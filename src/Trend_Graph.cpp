/* ================================================================
 *  Trend_Graph.cpp       (FreeRTOS Task +  )
 *  ESP32-S3   
 *
 *  
 *                                                      
 *     1.   /     (LovyanGFX)           
 *     2.     (Auto  Fixed )                   
 *     3.  (1  2  4)  /                              
 *     4.   ( )                            
 *     5.    SD                    
 *  
 *
 *    SD   
 *
 *   Main Task (Core 1)                  SD Task (Core 0)
 *                     
 *   [EXPORT ]
 *        
 *        
 *   exportGraphToSDAsync()
 *        
 *         SDMessage(OPEN)    file = SD.open()
 *                                      file.println(header)
 *        
 *         CSV  char buf[4096]
 *            
 *           pos  3500 
 *         SDMessage(DATA)    file.write(buf, len)
 *           ()
 *        
 *         
 *         SDMessage(DATA)    file.write(buf, len)
 *        
 *         SDMessage(CLOSE)   file.close()
 *                                        sdDone = true
 *        
 *   "Exporting" UI    
 *        
 *   loop()  checkSDWriteStatus()
 *                   sdDone 
 *                   / UI + 
 *  
 *
 *   
 *     SDMessage: union { filename[64], buf[4096] }
 *         sizeof  4104 B  (OPEN DATA   )
 *       10    ~40 KB  (heap )
 *       buf[4096] exportGraphToSDAsync()  
 *             ,   
 *  ================================================================ */

#include "Config.h"                 // enum, struct, extern  + PIN_BUZZER
#include "GFX_Wrapper.hpp"
#include "Lang.h"                   // L(), printL(), 
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/*  CURRENT_THRESHOLD_CRITICAL Config.h #define .
 *       . PIN_BUZZER Config.h . */

/* 
 *   
 *   */
static constexpr uint16_t MAX_POINTS      = 100;
static constexpr uint16_t SAMPLE_INTERVAL = 100;   /* ms */

static constexpr uint16_t GX    = 60;              /*    */
static constexpr uint16_t GY    = 50;
static constexpr uint16_t GW    = 380;
static constexpr uint16_t GH    = 80;
static constexpr uint16_t G_GAP = 20;              /*    */

static constexpr float DEF_P_MIN = -100.0f;
static constexpr float DEF_P_MAX =    0.0f;
static constexpr float DEF_C_MIN =    0.0f;
static constexpr float DEF_C_MAX =    5.0f;

/* 
 *   
 *   */
static constexpr uint16_t BUF_SIZE  = 4096;   /*             */
static constexpr uint16_t BUF_FLUSH = 3500;   /* flush              */
static constexpr uint16_t BUF_GUARD =   48;   /*   (25) +    */
static constexpr uint8_t  Q_DEPTH   =   10;   /*                    */
static constexpr uint16_t TASK_STACK = 8192;  /* SD Task   (8 KB)     */

/* 
 *    
 *   */
struct GraphData {
  float    pressure [MAX_POINTS];
  float    current  [MAX_POINTS];
  uint32_t timestamp[MAX_POINTS];
  uint16_t pointCount;
  uint16_t writeIndex;
  bool     bufferFull;

  float pressureMin, pressureMax;     /*    */
  float currentMin ,  currentMax;
  bool  autoScale;                    /* true =   */

  uint8_t  zoomLevel;                 /* 1 | 2 | 4 */
  int16_t  panOffset;

  bool     animated;
  uint16_t animationProgress;
  bool     capturing;
  uint32_t captureStartTime;
};

GraphData graphData;                  /*   (  extern ) */
static char g_csvBuffer[4096];        /* CSV   */

/* 
 *  SD      union OPEN DATA 
 *
 *    OPEN  : type=0, filename 
 *    DATA  : type=1, buf + len 
 *    CLOSE : type=2,   
 *   */
enum SDMsgType : uint8_t { SD_OPEN = 0, SD_DATA = 1, SD_CLOSE = 2 };

struct SDMessage {
  SDMsgType type;
  uint16_t  len;                      /* DATA   */
  union {
    char    filename[64];             /* OPEN  */
    uint8_t buf[BUF_SIZE];            /* DATA  */
  };
};

/*  / Task  */
static QueueHandle_t sdQueue      = nullptr;
static TaskHandle_t  sdTaskHandle = nullptr;

/*     (volatile  SD Task  Main Task) */
static volatile bool sdBusy       = false;   /*    */
static volatile bool sdDone       = false;   /*      */
static volatile bool sdSuccess    = false;   /*      */
static volatile char sdStatusMsg[128] = {};  /* UI   */

/* ================================================================
 *  SD Write Task    Core 0 
 *   OPEN  DATA(n)  CLOSE   
 *  ============================================================== */
static void sdWriteTask(void * /*unused*/) {
  SDMessage msg;
  File      file;                     /* Task    */

  Serial.printf("[SD Task] Core %d \n", (int)xPortGetCoreID());

  for (;;) {
    /*    () */
    if (xQueueReceive(sdQueue, &msg, portMAX_DELAY) != pdTRUE)
      continue;

    switch (msg.type) {
      /*  OPEN  */
      case SD_OPEN: {
        sdBusy = true;

        if (!SD.begin()) {
          Serial.println("[SD Task] SD  ");
          snprintf((char*)sdStatusMsg, sizeof(sdStatusMsg), "SD Card Error!");
          sdSuccess = false;  sdBusy = false;  sdDone = true;
          break;
        }
        /* /graph    */
        if (!SD.exists("/graph")) SD.mkdir("/graph");

        file = SD.open(msg.filename, FILE_WRITE);
        if (!file) {
          Serial.printf("[SD Task]   : %s\n", msg.filename);
          snprintf((char*)sdStatusMsg, sizeof(sdStatusMsg), "File Error!");
          sdSuccess = false;  sdBusy = false;  sdDone = true;
          break;
        }
        /* CSV    */
        file.println("Time(ms),Pressure(kPa),Current(A)");
        Serial.printf("[SD Task] OPEN  %s\n", msg.filename);
        break;
      }

      /*  DATA  */
      case SD_DATA: {
        if (file) {
          file.write(msg.buf, msg.len);
          Serial.printf("[SD Task] WRITE %u bytes\n", (unsigned)msg.len);
        }
        break;
      }

      /*  CLOSE  */
      case SD_CLOSE: {
        if (file) file.close();
        Serial.println("[SD Task] CLOSE ");
        snprintf((char*)sdStatusMsg, sizeof(sdStatusMsg), "Export complete!");
        sdSuccess = true;
        sdBusy    = false;
        sdDone    = true;             /* Main Task   */
        break;
      }
    }
  }
}

/* ================================================================
 *  initAsyncSD()    setup()    
 *    + Task  (Core 0)
 *  ============================================================== */
void initAsyncSD() {
  sdQueue = xQueueCreate(Q_DEPTH, sizeof(SDMessage));
  if (!sdQueue) { Serial.println("[SD]   !"); return; }

  BaseType_t rc = xTaskCreatePinnedToCore(
      sdWriteTask,
      "SDWriteTask",
      TASK_STACK,
      nullptr,
      1,              /*  (  UI  ) */
      &sdTaskHandle,
      0               /* Core 0  */
  );
  if (rc != pdPASS) { Serial.println("[SD] Task  !"); return; }

  Serial.printf("[SD]     (=%d, =%d)\n",
               (int)Q_DEPTH, (int)BUF_SIZE);
}

/* ================================================================
 *  exportGraphToSDAsync()    EXPORT  
 *
 *    ""  
 *
 *   :
 *     OPEN   push
 *     CSV  buf[4096]  
 *        pos  BUF_FLUSH(3500)  DATA  push, pos = 0
 *        pos > BUF_SIZE - BUF_GUARD  overflow  flush
 *      (pos > 0)   DATA push
 *     CLOSE  push
 *     UI "Exporting"    
 *
 *  Main Task   .
 *   SD  Core 0 sdWriteTask() .
 *  ============================================================== */
void exportGraphToSDAsync() {
  if (!sdQueue) { Serial.println("[SD]  "); return; }

  /*     UI  */
  if (sdBusy) {
    tft.fillRect(150, 140, 180, 40, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.setTextSize(1);
    printL(158, 155, TREND_WRITING);
    vTaskDelay(pdMS_TO_TICKS(800));
    screenNeedsRedraw = true;
    return;
  }

  /*        */
  if (graphData.pointCount == 0) {
    tft.fillRect(130, 140, 220, 40, TFT_ORANGE);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.setTextSize(1);
    printL(150, 155, TREND_NODATA);
    vTaskDelay(pdMS_TO_TICKS(1000));
    screenNeedsRedraw = true;
    Serial.println("[SD] EXPORT    0 pts");
    return;
  }

  /*   OPEN  */
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
      Serial.println("[SD] OPEN   ");
      return;
    }
  }

  /*   CSV   +  flush  */
  /* v3.9.1:    ( 4KB ) */
  {
    char*    buf = g_csvBuffer;       /*     */
    int      pos = 0;
    uint16_t si  = graphData.bufferFull ? graphData.writeIndex : 0;

    for (uint16_t i = 0; i < graphData.pointCount; i++) {
      uint16_t idx = (si + i) % MAX_POINTS;

      /* overflow :         flush */
      if (pos > (int)(BUF_SIZE - BUF_GUARD)) {
        SDMessage m;
        m.type = SD_DATA;
        m.len  = (uint16_t)pos;
        memcpy(m.buf, buf, pos);
        xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
        pos = 0;
      }

      /* CSV    */
      pos += snprintf(buf + pos, BUF_SIZE - pos,
          "%lu,%.2f,%.2f\n",
          (unsigned long)graphData.timestamp[idx],
          graphData.pressure[idx],
          graphData.current[idx]);

      /*     flush (    SD I/O  ) */
      if (pos >= BUF_FLUSH) {
        SDMessage m;
        m.type = SD_DATA;
        m.len  = (uint16_t)pos;
        memcpy(m.buf, buf, pos);
        xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
        pos = 0;
      }
    }

    /*    flush  */
    if (pos > 0) {
      SDMessage m;
      m.type = SD_DATA;
      m.len  = (uint16_t)pos;
      memcpy(m.buf, buf, pos);
      xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
    }
  }                                   /*     */

  /*   CLOSE  */
  {
    SDMessage m;
    memset(&m, 0, sizeof(m));
    m.type = SD_CLOSE;
    xQueueSend(sdQueue, &m, pdMS_TO_TICKS(200));
  }

  /*   UI   */
  tft.fillRect(130, 140, 220, 40, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(1);
  printL(145, 150, TREND_EXPORTING);
  tft.setCursor(145, 165);  tft.printf("%d points", graphData.pointCount);

  Serial.printf("[SD]     %d pts\n",
                (int)graphData.pointCount);
}

/* ================================================================
 *  checkSDWriteStatus()    loop()    
 *  sdDone   / UI + 
 *  UI    (delay  )
 *  ============================================================== */
static volatile uint32_t sdMsgShowUntil = 0;   /* UI    (millis) */

void checkSDWriteStatus() {
  uint32_t now = millis();

  /*         */
  if (sdMsgShowUntil > 0 && now < sdMsgShowUntil) return;

  /*         */
  if (sdMsgShowUntil > 0 && now >= sdMsgShowUntil) {
    sdMsgShowUntil  = 0;
    screenNeedsRedraw = true;
    return;
  }

  /*       */
  if (!sdDone) return;
  sdDone = false;                     /*    */

  if (sdSuccess) {
    tft.fillRect(130, 140, 220, 40, TFT_GREEN);
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.setTextSize(1);
    printL(140, 150, TREND_SUCCESS);
    tft.setCursor(140, 165);  tft.printf("%s", (const char*)sdStatusMsg);

    /*    2   ( ~200 ms,    ) */
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

    /*    1   */
    digitalWrite(PIN_BUZZER, HIGH); vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(PIN_BUZZER, LOW);
  }

  sdMsgShowUntil = now + 2000;        /*  2   (delay ) */
}

/* ================================================================
 *       setup()     
 *  ============================================================== */
void initGraphData() {
  memset(&graphData, 0, sizeof(graphData));

  graphData.pressureMin = DEF_P_MIN;
  graphData.pressureMax = DEF_P_MAX;
  graphData.currentMin  = DEF_C_MIN;
  graphData.currentMax  = DEF_C_MAX;
  graphData.autoScale   = true;
  graphData.zoomLevel   = 1;

  Serial.println("[]  ");
}

/* ================================================================
 *       loop() 100 ms  
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

  // if (graphData.autoScale) autoScale();  // 
}

/* ================================================================
 *    1   
 *       min/max  (10 % )
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
    if (p < pMin) pMin = p;           /*   min/max  */
    if (p > pMax) pMax = p;
    if (c > cMax) cMax = c;
    if (c < cMin) cMin = c;
  }

  /*    (10 %)   pMin  pMax   */
  float pr = pMax - pMin;
  if (pr > 0.0f) { pMin -= pr * 0.1f;  pMax += pr * 0.1f; }
  else           { pMin -= 5.0f;  pMax += 5.0f; }   /*   5 kPa */

  /*    */
  float cr = cMax - cMin;
  if (cr > 0.0f) { cMax += cr * 0.1f;  cMin -= cr * 0.1f;  if (cMin < 0) cMin = 0; }
  else           { cMax = cMin + 1.0f; }

  /*  :  =   (  ) 
   *   pressureMin = pMin (  ,  )
   *   pressureMax = pMax (  ,  )
   *   drawPressureGraph  mapVal(val, pMin, pMax, 0, GH)  
   *   pMiny=0(), pMaxy=GH()    */
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
 *    2   / 
 *    1  2  4  1 
 *  ============================================================== */
static void applyZoom(uint16_t &x1, uint16_t &x2,
                      uint16_t baseX, uint16_t baseW) {
  if (graphData.zoomLevel == 1) return;
  int16_t c = (int16_t)(baseX + baseW / 2);
  int16_t nx1 = c + ((int16_t)x1 - c) * graphData.zoomLevel + graphData.panOffset;
  int16_t nx2 = c + ((int16_t)x2 - c) * graphData.zoomLevel + graphData.panOffset;

  /*        */
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
    /*    */
    if      (graphData.zoomLevel == 1) graphData.zoomLevel = 2;
    else if (graphData.zoomLevel == 2) graphData.zoomLevel = 4;
    else { graphData.zoomLevel = 1;  graphData.panOffset = 0; }
    screenNeedsRedraw = true;
    lastTap = 0;                      /*     */
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
 *    4  
 *  ANIM      (2 )
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
 *   
 *  ============================================================== */
static inline float mapVal(float v,
                           float iMin, float iMax,
                           float oMin, float oMax) {
  return (v - iMin) * (oMax - oMin) / (iMax - iMin) + oMin;
}

/* ================================================================
 *    
 *  ============================================================== */
static void drawPressureGraph() {
  /*  */
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  printL(GX - 50, GY - 15, TREND_PRESS_LABEL);

  /*  */
  tft.drawRect(GX, GY, GW, GH, TFT_WHITE);

  /* Y  +  (6: 0~5) */
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

  /* X  +   (7: 0~6) */
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

  /*   */
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

      /*     */
      tft.drawLine(x1, y1,     x2, y2,     TFT_CYAN);
      tft.drawLine(x1, y1 + 1, x2, y2 + 1, TFT_CYAN);

      /* 10   */
      if (i % 10 == 0) {
        tft.fillCircle(x2, y2, 2, TFT_WHITE);
        tft.drawCircle(x2, y2, 3, TFT_CYAN);
      }
    }
  }

  /*    (PID  ) */
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

  /*    ( ) */
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
 *    
 *  ============================================================== */
static void drawCurrentGraph() {
  const uint16_t x = GX;
  const uint16_t y = GY + GH + G_GAP + 20;   /*    */

  /*  */
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  printL(x - 50, y - 15, TREND_CURR_LABEL);

  /*  */
  tft.drawRect(x, y, GW, GH, TFT_WHITE);

  /* Y  +  */
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

  /* X  +   */
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

  /*   */
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

  /*   ( ) */
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

  /*    */
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
 *  
 *  ============================================================== */
static void drawLegend() {
  uint16_t lx = GX;
  uint16_t ly = GY + (GH + G_GAP + 20) * 2 + GH + 8;

  tft.setTextSize(1);

  /*    */
  tft.drawLine(lx, ly, lx + 15, ly, TFT_CYAN);
  tft.drawLine(lx, ly + 1, lx + 15, ly + 1, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  printL(lx + 20, ly - 3, LEGEND_PRESS);

  /*    */
  tft.drawLine(lx + 90, ly, lx + 105, ly, TFT_YELLOW);
  tft.drawLine(lx + 90, ly + 1, lx + 105, ly + 1, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  printL(lx + 110, ly - 3, LEGEND_CURR);

  /* /    */
  for (uint16_t i = lx + 180; i < lx + 195; i += 3)
    tft.drawPixel(i, ly, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  printL(lx + 200, ly - 3, TREND_TARGET_LIMIT);

  /* Auto / Fixed  */
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(lx + 310, ly - 3);
  tft.print(graphData.autoScale ? "[Auto]" : "[Fixed]");
}

/* ================================================================
 *       ANIM | SCALE | EXPORT | BACK
 *  ============================================================== */
static void drawGraphControls() {
  constexpr uint16_t bY = 5, bW = 55, bH = 25, sp = 5;
  uint16_t bX;

  /* BACK ( ) */
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

  /* ANIM ( ) */
  bX -= (bW + sp);
  tft.fillRect(bX, bY, bW, bH, TFT_DARKGREY);
  tft.drawRect(bX, bY, bW, bH, TFT_MAGENTA);
  tft.setTextColor(TFT_MAGENTA, TFT_DARKGREY);
  printL(bX + 12, bY + 9, BTN_ANIM);
}

/* ================================================================
 *        UI_Screens.cpp updateUI() 
 *  ============================================================== */
void drawTrendGraph() {
  tft.fillScreen(TFT_BLACK);

  /*  +  */
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
 *       UI_Screens.cpp handleTouch() 
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

  /* EXPORT      */
  bX -= (bW + sp);
  if (x >= bX && x <= bX + bW && y >= bY && y <= bY + bH) {
    exportGraphToSDAsync();
    return;
  }

  /* SCALE (Auto  Fixed ) */
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

  /*       */
  if (x >= GX && x <= GX + GW)
    handleZoom(x, y);
}
