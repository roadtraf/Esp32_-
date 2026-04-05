/* ================================================================
 *  Trend_Graph.cpp   –  최종본  (FreeRTOS Task + 버퍼링 통합)
 *  ESP32-S3 진공 제어 시스템
 *
 *  ┌────────────────────────────────────────────────────────────┐
 *  │  포함 기능                                                 │
 *  │   1. 실시간 압력 / 전류 라인 그래프  (LovyanGFX)           │
 *  │   2. 자동 스케일링   (Auto ↔ Fixed 토글)                   │
 *  │   3. 줌 (1× → 2× → 4×)  /  팬                            │
 *  │   4. 애니메이션  (점진적 그리기)                            │
 *  │   5. 버퍼링 기반 비동기 SD 내보내기  ← 핵심                │
 *  └────────────────────────────────────────────────────────────┘
 *
 *  ─── 버퍼링 SD 쓰기 흐름 ────────────────────────────────────
 *
 *   Main Task (Core 1)                  SD Task (Core 0)
 *   ──────────────────                  ─────────────────
 *   [EXPORT 클릭]
 *        │
 *        ▼
 *   exportGraphToSDAsync()
 *        │
 *        ├─ SDMessage(OPEN) → 큐 ─────▶ file = SD.open()
 *        │                              file.println(header)
 *        │
 *        ├─ CSV 행을 char buf[4096]에
 *        │   순차 조립
 *        │   pos ≥ 3500 이면
 *        ├─ SDMessage(DATA) → 큐 ─────▶ file.write(buf, len)
 *        │   (반복)
 *        │
 *        ├─ 잔여분
 *        ├─ SDMessage(DATA) → 큐 ─────▶ file.write(buf, len)
 *        │
 *        └─ SDMessage(CLOSE)→ 큐 ─────▶ file.close()
 *                                        sdDone = true
 *        ▼
 *   "Exporting…" UI 표시 후 즉시 반환
 *        │
 *   loop() ──▶ checkSDWriteStatus()
 *                   sdDone 감지
 *                   성공/실패 UI + 부저
 *  ────────────────────────────────────────────────────────────
 *
 *  메모리 설계
 *    • SDMessage: union { filename[64], buf[4096] }
 *        → sizeof ≈ 4104 B  (OPEN과 DATA가 같은 공간 공유)
 *    • 큐 깊이 10  →  ~40 KB  (heap 충분)
 *    • 조립 버퍼 buf[4096]은 exportGraphToSDAsync() 스택 변수
 *        → 함수 반환 시 자동 해제, 장시간 점유 없음
 *  ================================================================ */

#include "Config.h"                 // enum, struct, extern 전역변수 + PIN_BUZZER
#include "LovyanGFX_Config.hpp"
#include "Lang.h"                   // L(), printL(), 다국어
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/* ★ CURRENT_THRESHOLD_CRITICAL는 Config.h의 #define로 공유됨.
 *    여기서는 별도 상수 불필요. PIN_BUZZER도 Config.h에서 공유됨. */

/* ────────────────────────────────────────────────────────────────
 *  그래프 상수
 *  ──────────────────────────────────────────────────────────── */
static constexpr uint16_t MAX_POINTS      = 100;
static constexpr uint16_t SAMPLE_INTERVAL = 100;   /* ms */

static constexpr uint16_t GX    = 60;              /* 그래프 영역 좌표 */
static constexpr uint16_t GY    = 50;
static constexpr uint16_t GW    = 380;
static constexpr uint16_t GH    = 80;
static constexpr uint16_t G_GAP = 20;              /* 그래프 사이 간격 */

static constexpr float DEF_P_MIN = -100.0f;
static constexpr float DEF_P_MAX =    0.0f;
static constexpr float DEF_C_MIN =    0.0f;
static constexpr float DEF_C_MAX =    5.0f;

/* ────────────────────────────────────────────────────────────────
 *  버퍼링 상수
 *  ──────────────────────────────────────────────────────────── */
static constexpr uint16_t BUF_SIZE  = 4096;   /* 조립 버퍼 크기          */
static constexpr uint16_t BUF_FLUSH = 3500;   /* flush 임계점             */
static constexpr uint16_t BUF_GUARD =   48;   /* 한 행 최대(25) + 여유   */
static constexpr uint8_t  Q_DEPTH   =   10;   /* 큐 깊이                  */
static constexpr uint16_t TASK_STACK = 8192;  /* SD Task 스택  (8 KB)     */

/* ────────────────────────────────────────────────────────────────
 *  그래프 데이터 구조체
 *  ──────────────────────────────────────────────────────────── */
struct GraphData {
  float    pressure [MAX_POINTS];
  float    current  [MAX_POINTS];
  uint32_t timestamp[MAX_POINTS];
  uint16_t pointCount;
  uint16_t writeIndex;
  bool     bufferFull;

  float pressureMin, pressureMax;     /* 현재 스케일 범위 */
  float currentMin ,  currentMax;
  bool  autoScale;                    /* true = 자동 스케일 */

  uint8_t  zoomLevel;                 /* 1 | 2 | 4 */
  int16_t  panOffset;

  bool     animated;
  uint16_t animationProgress;
  static char g_csvBuffer[BUF_SIZE];   /* v3.9.1 Phase 1: 정적 CSV 조립 버퍼 */
  bool     capturing;
  uint32_t captureStartTime;
};

GraphData graphData;                  /* 전역 원본 (다른 파일에서 extern 가능) */

/* ────────────────────────────────────────────────────────────────
 *  SD 큐 메시지  –  union으로 OPEN과 DATA 공유
 *
 *    OPEN  : type=0, filename 사용
 *    DATA  : type=1, buf + len 사용
 *    CLOSE : type=2, 유효 필드 없음
 *  ──────────────────────────────────────────────────────────── */
enum SDMsgType : uint8_t { SD_OPEN = 0, SD_DATA = 1, SD_CLOSE = 2 };

struct SDMessage {
  SDMsgType type;
  uint16_t  len;                      /* DATA용 유효 길이 */
  union {
    char    filename[64];             /* OPEN용  */
    uint8_t buf[BUF_SIZE];            /* DATA용  */
  };
};

/* 큐 / Task 핸들 */
static QueueHandle_t sdQueue      = nullptr;
static TaskHandle_t  sdTaskHandle = nullptr;

/* 공유 상태 플래그  (volatile – SD Task ↔ Main Task) */
static volatile bool sdBusy       = false;   /* 작업 진행 중 */
static volatile bool sdDone       = false;   /* 완료 신호    */
static volatile bool sdSuccess    = false;   /* 성공 여부    */
static volatile char sdStatusMsg[128] = {};  /* UI 표시용 문자열 */

/* ================================================================
 *  SD Write Task  –  Core 0 전용
 *  큐에서 OPEN → DATA(n회) → CLOSE 를 순차 처리
 *  ============================================================== */
static void sdWriteTask(void * /*unused*/) {
  SDMessage msg;
  File      file;                     /* Task 지역 파일 핸들 */

  Serial.printf("[SD Task] Core %d 시작\n", (int)xPortGetCoreID());

  for (;;) {
    /* 큐에서 메시지 대기 (무한) */
    if (xQueueReceive(sdQueue, &msg, portMAX_DELAY) != pdTRUE)
      continue;

    switch (msg.type) {
      /* ── OPEN ─────────────────────────────────────────── */
      case SD_OPEN: {
        sdBusy = true;

        if (!SD.begin()) {
          Serial.println("[SD Task] SD 카드 없음");
          snprintf((char*)sdStatusMsg, sizeof(sdStatusMsg), "SD Card Error!");
          sdSuccess = false;  sdBusy = false;  sdDone = true;
          break;
        }
        /* /graph 폴더 없으면 생성 */
        if (!SD.exists("/graph")) SD.mkdir("/graph");

        file = SD.open(msg.filename, FILE_WRITE);
        if (!file) {
          Serial.printf("[SD Task] 파일 생성 실패: %s\n", msg.filename);
          snprintf((char*)sdStatusMsg, sizeof(sdStatusMsg), "File Error!");
          sdSuccess = false;  sdBusy = false;  sdDone = true;
          break;
        }
        /* CSV 헤더 즉시 쓰기 */
        file.println("Time(ms),Pressure(kPa),Current(A)");
        Serial.printf("[SD Task] OPEN → %s\n", msg.filename);
        break;
      }

      /* ── DATA ─────────────────────────────────────────── */
      case SD_DATA: {
        if (file) {
          file.write(msg.buf, msg.len);
          Serial.printf("[SD Task] WRITE %u bytes\n", (unsigned)msg.len);
        }
        break;
      }

      /* ── CLOSE ────────────────────────────────────────── */
      case SD_CLOSE: {
        if (file) file.close();
        Serial.println("[SD Task] CLOSE 완료");
        snprintf((char*)sdStatusMsg, sizeof(sdStatusMsg), "Export complete!");
        sdSuccess = true;
        sdBusy    = false;
        sdDone    = true;             /* Main Task에 완료 신호 */
        break;
      }
    }
  }
}

/* ================================================================
 *  initAsyncSD()  –  setup() 끝에서 한 번 호출
 *  큐 생성 + Task 생성 (Core 0)
 *  ============================================================== */
void initAsyncSD() {
  sdQueue = xQueueCreate(Q_DEPTH, sizeof(SDMessage));
  if (!sdQueue) { Serial.println("[SD] 큐 생성 실패!"); return; }

  BaseType_t rc = xTaskCreatePinnedToCore(
      sdWriteTask,
      "SDWriteTask",
      TASK_STACK,
      nullptr,
      1,              /* 우선순위 (낮음 – UI가 더 중요) */
      &sdTaskHandle,
      0               /* Core 0 고정 */
  );
  if (rc != pdPASS) { Serial.println("[SD] Task 생성 실패!"); return; }

  Serial.printf("[SD] 비동기 초기화 완료  (큐=%d, 버퍼=%d)\n",
               (int)Q_DEPTH, (int)BUF_SIZE);
}

/* ================================================================
 *  exportGraphToSDAsync()  –  EXPORT 버튼에서 호출
 *
 *  ★ 이것이 "버퍼링" 핵심 함수
 *
 *  단계 순서:
 *    ① OPEN 메시지 큐에 push
 *    ② CSV 행을 buf[4096]에 순차 조립
 *       • pos ≥ BUF_FLUSH(3500) → DATA 메시지 push, pos = 0
 *       • pos > BUF_SIZE - BUF_GUARD → overflow 방지 flush
 *    ③ 잔여 (pos > 0) → 마지막 DATA push
 *    ④ CLOSE 메시지 push
 *    ⑤ UI에 "Exporting…" 표시 후 즉시 반환
 *
 *  Main Task는 거의 멈추지 않음.
 *  실제 SD 쓰기는 Core 0의 sdWriteTask()가 처리.
 *  ============================================================== */
void exportGraphToSDAsync() {
  if (!sdQueue) { Serial.println("[SD] 초기화되지 않음"); return; }

  /* 이미 쓰기 진행 중이면 UI 알림만 */
  if (sdBusy) {
    tft.fillRect(150, 140, 180, 40, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.setTextSize(1);
    printL(158, 155, TREND_WRITING);
    vTaskDelay(pdMS_TO_TICKS(800));
    screenNeedsRedraw = true;
    return;
  }

  /* ★ 데이터가 없으면 빈 파일을 만들지 않음 */
  if (graphData.pointCount == 0) {
    tft.fillRect(130, 140, 220, 40, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.setTextSize(1);
    printL(150, 155, TREND_NODATA);
    vTaskDelay(pdMS_TO_TICKS(1000));
    screenNeedsRedraw = true;
    Serial.println("[SD] EXPORT 취소 — 데이터 0 pts");
    return;
  }

  /* ─── ① OPEN ─────────────────────────────────────────── */
  sdDone = false;
  {
    SDMessage m;
    memset(&m, 0, sizeof(m));
    m.type = SD_OPEN;

    time_t  now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    snprintf(m.filename, sizeof(m.filename),
        "/graph/g_%04d%02d%02d_%02d%02d%02d_c%u.csv",
        t.tm_year+1900, t.tm_mon+1, t.tm_mday,
        t.tm_hour, t.tm_min, t.tm_sec,
        (unsigned)stats.totalCycles);

    if (xQueueSend(sdQueue, &m, 0) != pdTRUE) {
      Serial.println("[SD] OPEN 큐 전송 실패");
      return;
    }
  }

  /* ─── ② CSV 행 조립 + 버퍼링 flush ────────────────────── */
  /* v3.9.1: 정적 버퍼 사용 (스택 4KB 절약) */
  {
    char*    buf = g_csvBuffer;       /* ★ 정적 버퍼 포인터 */
    int      pos = 0;
    uint16_t si  = graphData.bufferFull ? graphData.writeIndex : 0;

    for (uint16_t i = 0; i < graphData.pointCount; i++) {
      uint16_t idx = (si + i) % MAX_POINTS;

      /* overflow 방지: 남은 공간이 한 행 최대 길이보다 작으면 먼저 flush */
      if (pos > (int)(BUF_SIZE - BUF_GUARD)) {
        SDMessage m;
        m.type = SD_DATA;
        m.len  = (uint16_t)pos;
        memcpy(m.buf, buf, pos);
        xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
        pos = 0;
      }

      /* CSV 한 행 조립 */
      pos += snprintf(buf + pos, BUF_SIZE - pos,
          "%lu,%.2f,%.2f\n",
          (unsigned long)graphData.timestamp[idx],
          graphData.pressure[idx],
          graphData.current[idx]);

      /* ★ 임계점 도달 → flush (한 번의 큰 쓰기로 SD I/O 횟수 감소) */
      if (pos >= BUF_FLUSH) {
        SDMessage m;
        m.type = SD_DATA;
        m.len  = (uint16_t)pos;
        memcpy(m.buf, buf, pos);
        xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
        pos = 0;
      }
    }

    /* ─── ③ 잔여 flush ────────────────────────────────── */
    if (pos > 0) {
      SDMessage m;
      m.type = SD_DATA;
      m.len  = (uint16_t)pos;
      memcpy(m.buf, buf, pos);
      xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
    }
  }                                   /* 정적 버퍼는 계속 유지 */

  /* ─── ④ CLOSE ────────────────────────────────────────── */
  {
    SDMessage m;
    memset(&m, 0, sizeof(m));
    m.type = SD_CLOSE;
    xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
  }

  /* ─── ⑤ UI 피드백 ──────────────────────────────────── */
  tft.fillRect(130, 140, 220, 40, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(1);
  printL(145, 150, TREND_EXPORTING);
  tft.setCursor(145, 165);  tft.printf("%d points", graphData.pointCount);

  Serial.printf("[SD] 버퍼링 내보내기 시작 — %d pts\n",
                (int)graphData.pointCount);
}

/* ================================================================
 *  checkSDWriteStatus()  –  loop() 안에서 매 루프마다 호출
 *  sdDone 플래그를 감지하여 성공/실패 UI + 부저
 *  UI 표시는 타이머 기반 (delay 사용하지 않음)
 *  ============================================================== */
static volatile uint32_t sdMsgShowUntil = 0;   /* UI 표시 유효 시간 (millis) */

void checkSDWriteStatus() {
  uint32_t now = millis();

  /* ── 이전 표시가 아직 남아있으면 그대로 유지 ── */
  if (sdMsgShowUntil > 0 && now < sdMsgShowUntil) return;

  /* ── 표시 시간 만료 → 화면 갱신 ── */
  if (sdMsgShowUntil > 0 && now >= sdMsgShowUntil) {
    sdMsgShowUntil  = 0;
    screenNeedsRedraw = true;
    return;
  }

  /* ── 완료 신호 없으면 종료 ── */
  if (!sdDone) return;
  sdDone = false;                     /* 한 번만 처리 */

  if (sdSuccess) {
    tft.fillRect(130, 140, 220, 40, TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.setTextSize(1);
    printL(140, 150, TREND_SUCCESS);
    tft.setCursor(140, 165);  tft.printf("%s", (const char*)sdStatusMsg);

    /* 부저 – 성공 2회 짧은 소리 (총 ~200 ms, 실시간 제어 영향 최소) */
    digitalWrite(PIN_BUZZER, HIGH); vTaskDelay(pdMS_TO_TICKS(80));
    digitalWrite(PIN_BUZZER, LOW);  vTaskDelay(pdMS_TO_TICKS(40));
    digitalWrite(PIN_BUZZER, HIGH); vTaskDelay(pdMS_TO_TICKS(80));
    digitalWrite(PIN_BUZZER, LOW);
  } else {
    tft.fillRect(130, 140, 220, 40, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(1);
    printL(140, 150, TREND_FAILED);
    tft.setCursor(140, 165);  tft.printf("%s", (const char*)sdStatusMsg);

    /* 부저 – 실패 1회 짧은 소리 */
    digitalWrite(PIN_BUZZER, HIGH); vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(PIN_BUZZER, LOW);
  }

  sdMsgShowUntil = now + 2000;        /* ★ 2초간 표시 유지 (delay 없이) */
}

/* ================================================================
 *  그래프 초기화  –  setup()과 새 사이클 시작 시 호출
 *  ============================================================== */
void initGraphData() {
  memset(&graphData, 0, sizeof(graphData));

  graphData.pressureMin = DEF_P_MIN;
  graphData.pressureMax = DEF_P_MAX;
  graphData.currentMin  = DEF_C_MIN;
  graphData.currentMax  = DEF_C_MAX;
  graphData.autoScale   = true;
  graphData.zoomLevel   = 1;

  Serial.println("[그래프] 초기화 완료");
}

/* ================================================================
 *  포인트 추가  –  loop()에서 100 ms 간격으로 호출
 *  ============================================================== */
void addGraphPoint(float pressure, float current) {
  if (graphData.pointCount == 0 && !graphData.capturing) {
    graphData.capturing        = true;
    graphData.captureStartTime = millis();
  }

  graphData.pressure [graphData.writeIndex] = pressure;
  graphData.current  [graphData.writeIndex] = current;
  graphData.timestamp[graphData.writeIndex] = millis() - graphData.captureStartTime;

  if (++graphData.writeIndex >= MAX_POINTS) {
    graphData.writeIndex = 0;
    graphData.bufferFull = true;
  }
  graphData.pointCount = graphData.bufferFull ? MAX_POINTS : graphData.writeIndex;

  if (graphData.autoScale) autoScale();
}

/* ================================================================
 *  고급 기능 1 – 자동 스케일링
 *  데이터 전체를 한 번 순회하여 min/max 갱신 (±10 % 여유)
 *  ============================================================== */
void autoScale() {
  if (!graphData.pointCount) return;

  uint16_t si = graphData.bufferFull ? graphData.writeIndex : 0;

  float pMin, pMax, cMin, cMax;
  pMin = pMax = graphData.pressure[si % MAX_POINTS];
  cMin = cMax = graphData.current [si % MAX_POINTS];

  for (uint16_t i = 1; i < graphData.pointCount; i++) {
    uint16_t idx = (si + i) % MAX_POINTS;
    float p = graphData.pressure[idx];
    float c = graphData.current [idx];
    if (p < pMin) pMin = p;           /* ★ 표준 min/max 비교 */
    if (p > pMax) pMax = p;
    if (c > cMax) cMax = c;
    if (c < cMin) cMin = c;
  }

  /* 압력 범위 여유 (±10 %)  — pMin ≤ pMax 항상 성립 */
  float pr = pMax - pMin;
  if (pr > 0.0f) { pMin -= pr * 0.1f;  pMax += pr * 0.1f; }
  else           { pMin -= 5.0f;  pMax += 5.0f; }   /* 단일 값이면 ±5 kPa */

  /* 전류 범위 여유 */
  float cr = cMax - cMin;
  if (cr > 0.0f) { cMax += cr * 0.1f;  cMin -= cr * 0.1f;  if (cMin < 0) cMin = 0; }
  else           { cMax = cMin + 1.0f; }

  /* ★ 압력축: 아래 = 더 큰 진공(더 작은 값) 이므로
   *   pressureMin = pMin (가장 작은 값, 그래프 아래)
   *   pressureMax = pMax (가장 큰 값, 그래프 위)
   *   drawPressureGraph 에서 mapVal(val, pMin, pMax, 0, GH) 사용 시
   *   pMin→y=0(아래), pMax→y=GH(위) 로 올바르게 렌더링됨 */
  graphData.pressureMin = pMin;
  graphData.pressureMax = pMax;
  graphData.currentMin  = cMin;
  graphData.currentMax  = cMax;
}

void resetScale() {
  graphData.pressureMin = DEF_P_MIN;
  graphData.pressureMax = DEF_P_MAX;
  graphData.currentMin  = DEF_C_MIN;
  graphData.currentMax  = DEF_C_MAX;
  graphData.autoScale   = true;
}

/* ================================================================
 *  고급 기능 2 – 줌 / 팬
 *  더블 탭으로 1× → 2× → 4× → 1× 순환
 *  ============================================================== */
static void applyZoom(uint16_t &x1, uint16_t &x2,
                      uint16_t baseX, uint16_t baseW) {
  if (graphData.zoomLevel == 1) return;
  int16_t c = (int16_t)(baseX + baseW / 2);
  int16_t nx1 = c + ((int16_t)x1 - c) * graphData.zoomLevel + graphData.panOffset;
  int16_t nx2 = c + ((int16_t)x2 - c) * graphData.zoomLevel + graphData.panOffset;

  /* ★ 그래프 영역 밖으로 나간 좌표를 클리핑 */
  x1 = (uint16_t)constrain(nx1, (int16_t)baseX,            (int16_t)(baseX + baseW));
  x2 = (uint16_t)constrain(nx2, (int16_t)baseX,            (int16_t)(baseX + baseW));
}

void handleZoom(uint16_t x, uint16_t y) {
  static uint32_t lastTap = 0;
  static uint16_t ltX = 0, ltY = 0;
  uint32_t now = millis();

  if (now - lastTap < 300 &&
      abs((int16_t)x - (int16_t)ltX) < 20 &&
      abs((int16_t)y - (int16_t)ltY) < 20) {
    /* 더블 탭 감지 */
    if      (graphData.zoomLevel == 1) graphData.zoomLevel = 2;
    else if (graphData.zoomLevel == 2) graphData.zoomLevel = 4;
    else { graphData.zoomLevel = 1;  graphData.panOffset = 0; }
    screenNeedsRedraw = true;
    lastTap = 0;                      /* 다음 탭 카운터 리셋 */
  } else {
    lastTap = now;  ltX = x;  ltY = y;
  }
}

void handlePan(int16_t delta) {
  if (graphData.zoomLevel == 1) return;
  graphData.panOffset += delta;
  int16_t maxPan = (int16_t)(GW * (graphData.zoomLevel - 1) / 2);
  graphData.panOffset = constrain(graphData.panOffset, -maxPan, maxPan);
  screenNeedsRedraw = true;
}

/* ================================================================
 *  고급 기능 4 – 애니메이션
 *  ANIM 버튼 클릭 시 점진적 그리기 (2포인트씩 진행)
 *  ============================================================== */
void startAnimation() {
  graphData.animated          = true;
  graphData.animationProgress = 0;
}

void updateAnimation() {
  if (!graphData.animated) return;
  graphData.animationProgress += 2;
  if (graphData.animationProgress >= graphData.pointCount) {
    graphData.animationProgress = graphData.pointCount;
    graphData.animated = false;
  }
}

/* ================================================================
 *  내부 유틸
 *  ============================================================== */
static inline float mapVal(float v,
                           float iMin, float iMax,
                           float oMin, float oMax) {
  return (v - iMin) * (oMax - oMin) / (iMax - iMin) + oMin;
}

/* ================================================================
 *  압력 그래프 그리기
 *  ============================================================== */
static void drawPressureGraph() {
  /* 제목 */
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  printL(GX - 50, GY - 15, TREND_PRESS_LABEL);

  /* 테두리 */
  tft.drawRect(GX, GY, GW, GH, TFT_WHITE);

  /* Y축 그리드 + 라벨 (6단계: 0~5) */
  for (int i = 0; i <= 5; i++) {
    uint16_t yp  = GY + (uint16_t)(GH * i / 5);
    float    val = graphData.pressureMax -
                   (graphData.pressureMax - graphData.pressureMin) * i / 5.0f;

    for (uint16_t xp = GX; xp < GX + GW; xp += 5)
      tft.drawPixel(xp, yp, TFT_DARKGREY);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(GX - 42, yp - 4);
    tft.printf("%.0f", val);
  }

  /* X축 그리드 + 시간 라벨 (7단계: 0~6) */
  for (int i = 0; i <= 6; i++) {
    uint16_t xp = GX + (uint16_t)(GW * i / 6);
    for (uint16_t yp = GY; yp < GY + GH; yp += 5)
      tft.drawPixel(xp, yp, TFT_DARKGREY);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(xp - 8, GY + GH + 3);
    if (graphData.pointCount) {
      uint16_t li = (graphData.bufferFull
                   ? graphData.writeIndex
                   : graphData.pointCount - 1) % MAX_POINTS;
      tft.printf("%.1f", graphData.timestamp[li] / 1000.0f * i / 6.0f);
    } else {
      tft.printf("%d", i);
    }
  }

  /* 데이터 라인 */
  if (graphData.pointCount > 1) {
    uint16_t si  = graphData.bufferFull ? graphData.writeIndex : 0;
    uint16_t cnt = graphData.animated
                 ? graphData.animationProgress
                 : graphData.pointCount;

    for (uint16_t i = 1; i < cnt; i++) {
      uint16_t i1 = (si + i - 1) % MAX_POINTS;
      uint16_t i2 = (si + i    ) % MAX_POINTS;

      uint16_t x1 = GX + (uint16_t)((uint32_t)GW * (i - 1) / graphData.pointCount);
      uint16_t y1 = GY + GH - (uint16_t)mapVal(graphData.pressure[i1],
                     graphData.pressureMin, graphData.pressureMax, 0, GH);
      uint16_t x2 = GX + (uint16_t)((uint32_t)GW *  i      / graphData.pointCount);
      uint16_t y2 = GY + GH - (uint16_t)mapVal(graphData.pressure[i2],
                     graphData.pressureMin, graphData.pressureMax, 0, GH);

      applyZoom(x1, x2, GX, GW);
      y1 = constrain(y1, GY, GY + GH);
      y2 = constrain(y2, GY, GY + GH);

      /* 두 픽셀 두께 라인 */
      tft.drawLine(x1, y1,     x2, y2,     TFT_CYAN);
      tft.drawLine(x1, y1 + 1, x2, y2 + 1, TFT_CYAN);

      /* 10개마다 강조 원 */
      if (i % 10 == 0) {
        tft.fillCircle(x2, y2, 2, TFT_WHITE);
        tft.drawCircle(x2, y2, 3, TFT_CYAN);
      }
    }
  }

  /* 목표 압력 점선 (PID 모드일 때만) */
  if (currentMode == MODE_PID) {
    uint16_t ty = GY + GH - (uint16_t)mapVal(config.targetPressure,
                   graphData.pressureMin, graphData.pressureMax, 0, GH);
    if (ty >= GY && ty <= GY + GH) {
      for (uint16_t i = GX; i < GX + GW; i += 8)
        tft.drawLine(i, ty, i + 4, ty, TFT_RED);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(GX + GW + 3, ty - 4);
      tft.printf("%.0f", config.targetPressure);
    }
  }

  /* 현재 값 표시 (오른쪽 상단) */
  if (graphData.pointCount) {
    uint16_t li = graphData.bufferFull
               ? (graphData.writeIndex + MAX_POINTS - 1) % MAX_POINTS
               : graphData.pointCount - 1;
    tft.fillRect(GX + GW - 60, GY - 12, 60, 10, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(GX + GW - 58, GY - 12);
    tft.printf("%.1f kPa", graphData.pressure[li]);
  }
}

/* ================================================================
 *  전류 그래프 그리기
 *  ============================================================== */
static void drawCurrentGraph() {
  const uint16_t x = GX;
  const uint16_t y = GY + GH + G_GAP + 20;   /* 압력 그래프 아래 */

  /* 제목 */
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  printL(x - 50, y - 15, TREND_CURR_LABEL);

  /* 테두리 */
  tft.drawRect(x, y, GW, GH, TFT_WHITE);

  /* Y축 그리드 + 라벨 */
  for (int i = 0; i <= 5; i++) {
    uint16_t yp  = y + (uint16_t)(GH * i / 5);
    float    val = graphData.currentMax -
                   (graphData.currentMax - graphData.currentMin) * i / 5.0f;

    for (uint16_t xp = x; xp < x + GW; xp += 5)
      tft.drawPixel(xp, yp, TFT_DARKGREY);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(x - 32, yp - 4);
    tft.printf("%.1f", val);
  }

  /* X축 그리드 + 시간 라벨 */
  for (int i = 0; i <= 6; i++) {
    uint16_t xp = x + (uint16_t)(GW * i / 6);
    for (uint16_t yp = y; yp < y + GH; yp += 5)
      tft.drawPixel(xp, yp, TFT_DARKGREY);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(xp - 8, y + GH + 3);
    if (graphData.pointCount) {
      uint16_t li = (graphData.bufferFull
                   ? graphData.writeIndex
                   : graphData.pointCount - 1) % MAX_POINTS;
      tft.printf("%.1f", graphData.timestamp[li] / 1000.0f * i / 6.0f);
    } else {
      tft.printf("%d", i);
    }
  }

  /* 데이터 라인 */
  if (graphData.pointCount > 1) {
    uint16_t si  = graphData.bufferFull ? graphData.writeIndex : 0;
    uint16_t cnt = graphData.animated
                 ? graphData.animationProgress
                 : graphData.pointCount;

    for (uint16_t i = 1; i < cnt; i++) {
      uint16_t i1 = (si + i - 1) % MAX_POINTS;
      uint16_t i2 = (si + i    ) % MAX_POINTS;

      uint16_t x1 = x + (uint16_t)((uint32_t)GW * (i - 1) / graphData.pointCount);
      uint16_t y1 = y + GH - (uint16_t)mapVal(graphData.current[i1],
                     graphData.currentMin, graphData.currentMax, 0, GH);
      uint16_t x2 = x + (uint16_t)((uint32_t)GW *  i      / graphData.pointCount);
      uint16_t y2 = y + GH - (uint16_t)mapVal(graphData.current[i2],
                     graphData.currentMin, graphData.currentMax, 0, GH);

      applyZoom(x1, x2, x, GW);
      y1 = constrain(y1, y, y + GH);
      y2 = constrain(y2, y, y + GH);

      tft.drawLine(x1, y1,     x2, y2,     TFT_YELLOW);
      tft.drawLine(x1, y1 + 1, x2, y2 + 1, TFT_YELLOW);

      if (i % 10 == 0) {
        tft.fillCircle(x2, y2, 2, TFT_WHITE);
        tft.drawCircle(x2, y2, 3, TFT_YELLOW);
      }
    }
  }

  /* 임계값 점선 (항상 표시) */
  {
    uint16_t cy = y + GH - (uint16_t)mapVal(CURRENT_THRESHOLD_CRITICAL,
                   graphData.currentMin, graphData.currentMax, 0, GH);
    if (cy >= y && cy <= y + GH) {
      for (uint16_t i = x; i < x + GW; i += 8)
        tft.drawLine(i, cy, i + 4, cy, TFT_RED);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(x + GW + 3, cy - 4);
      tft.printf("%.1f", CURRENT_THRESHOLD_CRITICAL);
    }
  }

  /* 현재 값 표시 */
  if (graphData.pointCount) {
    uint16_t li = graphData.bufferFull
               ? (graphData.writeIndex + MAX_POINTS - 1) % MAX_POINTS
               : graphData.pointCount - 1;
    tft.fillRect(x + GW - 50, y - 12, 50, 10, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(x + GW - 48, y - 12);
    tft.printf("%.2f A", graphData.current[li]);
  }
}

/* ================================================================
 *  범례
 *  ============================================================== */
static void drawLegend() {
  uint16_t lx = GX;
  uint16_t ly = GY + (GH + G_GAP + 20) * 2 + GH + 8;

  tft.setTextSize(1);

  /* 압력 – 청록색 */
  tft.drawLine(lx, ly, lx + 15, ly, TFT_CYAN);
  tft.drawLine(lx, ly + 1, lx + 15, ly + 1, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  printL(lx + 20, ly - 3, LEGEND_PRESS);

  /* 전류 – 노란색 */
  tft.drawLine(lx + 90, ly, lx + 105, ly, TFT_YELLOW);
  tft.drawLine(lx + 90, ly + 1, lx + 105, ly + 1, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  printL(lx + 110, ly - 3, LEGEND_CURR);

  /* 목표/임계값 – 빨간 점선 */
  for (uint16_t i = lx + 180; i < lx + 195; i += 3)
    tft.drawPixel(i, ly, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  printL(lx + 200, ly - 3, TREND_TARGET_LIMIT);

  /* Auto / Fixed 상태 */
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(lx + 310, ly - 3);
  tft.print(graphData.autoScale ? "[Auto]" : "[Fixed]");
}

/* ================================================================
 *  컨트롤 버튼  –  ANIM | SCALE | EXPORT | BACK
 *  ============================================================== */
static void drawGraphControls() {
  constexpr uint16_t bY = 5, bW = 55, bH = 25, sp = 5;
  uint16_t bX;

  /* BACK (오른쪽 끝) */
  bX = 480 - bW - 5;
  tft.fillRect(bX, bY, bW, bH, TFT_DARKGREY);
  tft.drawRect(bX, bY, bW, bH, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_DARKGREY);
  tft.setTextSize(1);
  printL(bX + 12, bY + 9, BACK);

  /* EXPORT */
  bX -= (bW + sp);
  tft.fillRect(bX, bY, bW, bH, TFT_DARKGREY);
  tft.drawRect(bX, bY, bW, bH, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, TFT_DARKGREY);
  printL(bX + 6, bY + 9, BTN_EXPORT);

  /* SCALE */
  bX -= (bW + sp);
  tft.fillRect(bX, bY, bW, bH, TFT_DARKGREY);
  tft.drawRect(bX, bY, bW, bH, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_DARKGREY);
  printL(bX + 8, bY + 9, BTN_SCALE);

  /* ANIM (왼쪽 끝) */
  bX -= (bW + sp);
  tft.fillRect(bX, bY, bW, bH, TFT_DARKGREY);
  tft.drawRect(bX, bY, bW, bH, TFT_MAGENTA);
  tft.setTextColor(TFT_MAGENTA, TFT_DARKGREY);
  printL(bX + 12, bY + 9, BTN_ANIM);
}

/* ================================================================
 *  메인 그래프 화면  –  UI_Screens.cpp의 updateUI()에서 호출
 *  ============================================================== */
void drawTrendGraph() {
  tft.fillScreen(TFT_BLACK);

  /* 제목 + 정보 */
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  printL(10, 10, TITLE_TREND);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(200, 15);
  tft.printf("Pts:%d/%d", graphData.pointCount, MAX_POINTS);

  if (graphData.zoomLevel > 1) {
    tft.setCursor(330, 15);
    tft.printf("Z:%dx", graphData.zoomLevel);
  }

  drawPressureGraph();
  drawCurrentGraph();
  drawLegend();
  drawGraphControls();

  if (graphData.animated) updateAnimation();
}

/* ================================================================
 *  터치 핸들러  –  UI_Screens.cpp의 handleTouch()에서 호출
 *  ============================================================== */
void handleGraphTouch(uint16_t x, uint16_t y) {
  constexpr uint16_t bY = 5, bW = 55, bH = 25, sp = 5;
  uint16_t bX;

  /* BACK */
  bX = 480 - bW - 5;
  if (x >= bX && x <= bX + bW && y >= bY && y <= bY + bH) {
    currentScreen = SCREEN_SETTINGS;  screenNeedsRedraw = true;
    return;
  }

  /* EXPORT  ← 버퍼링 비동기 호출 */
  bX -= (bW + sp);
  if (x >= bX && x <= bX + bW && y >= bY && y <= bY + bH) {
    exportGraphToSDAsync();
    return;
  }

  /* SCALE (Auto ↔ Fixed 토글) */
  bX -= (bW + sp);
  if (x >= bX && x <= bX + bW && y >= bY && y <= bY + bH) {
    graphData.autoScale = !graphData.autoScale;
    if (!graphData.autoScale) resetScale();
    screenNeedsRedraw = true;
    return;
  }

  /* ANIM */
  bX -= (bW + sp);
  if (x >= bX && x <= bX + bW && y >= bY && y <= bY + bH) {
    if (!graphData.animated && graphData.pointCount) startAnimation();
    screenNeedsRedraw = true;
    return;
  }

  /* 그래프 영역 터치 → 줌 더블탭 */
  if (x >= GX && x <= GX + GW)
    handleZoom(x, y);
}
