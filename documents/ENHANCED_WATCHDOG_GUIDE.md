# 🛡️ Enhanced Watchdog 통합 가이드 (Phase 3-1)

## 📋 개요

Enhanced Watchdog는 ESP32-S3 진공 제어 시스템의 안정성을 크게 향상시키는 모니터링 시스템입니다.

### ✨ 주요 기능

1. **태스크별 모니터링**
   - 각 FreeRTOS 태스크의 상태 추적
   - 데드락/정지 자동 감지
   - 실시간 성능 모니터링

2. **자동 복구**
   - 데드락 감지 시 자동 재시작
   - 재시작 원인 분석 및 저장
   - 부팅 시 이전 재시작 정보 표시

3. **진단 도구**
   - 실시간 상태 출력
   - 태스크별 상세 정보
   - 재시작 히스토리

---

## 🚀 빠른 시작 (5분 안에 적용)

### Step 1: 파일 추가 (1분)

```bash
# 프로젝트 디렉토리로 이동
cd your_project_directory

# 헤더 파일 복사
cp EnhancedWatchdog.h include/

# 소스 파일 복사
cp EnhancedWatchdog.cpp src/
```

### Step 2: main.cpp 수정 (3분)

#### 1. 헤더 include
```cpp
// main.cpp 상단에 추가
#include "EnhancedWatchdog.h"
```

#### 2. setup() 초기화
```cpp
void setup() {
    Serial.begin(115200);
    
    // ✅ Enhanced Watchdog 초기화 추가
    enhancedWatchdog.begin(10);  // 10초 타임아웃
    
    // ✅ 태스크 등록
    enhancedWatchdog.registerTask("VacuumCtrl", 5000);   // 5초마다 체크인
    enhancedWatchdog.registerTask("SensorRead", 1000);   // 1초마다
    enhancedWatchdog.registerTask("UIUpdate", 100);      // 100ms마다
    enhancedWatchdog.registerTask("WiFiMgr", 10000);     // 10초마다
    
    // 나머지 기존 초기화 코드...
}
```

#### 3. loop() 업데이트 추가
```cpp
void loop() {
    // ✅ Watchdog 업데이트 (필수!)
    enhancedWatchdog.update();
    
    // 나머지 기존 코드...
}
```

### Step 3: FreeRTOS 태스크 수정 (1분)

각 태스크에 체크인 추가:

```cpp
void vacuumControlTask(void* param) {
    while (1) {
        // 기존 작업...
        
        // ✅ Watchdog 체크인 추가
        WDT_CHECKIN("VacuumCtrl");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void sensorReadTask(void* param) {
    while (1) {
        readSensors();
        
        // ✅ Watchdog 체크인 추가
        WDT_CHECKIN("SensorRead");
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 다른 태스크들도 동일하게...
```

### Step 4: 컴파일 & 테스트
```bash
pio run --target upload
pio device monitor
```

---

## 📊 실행 결과 예시

### 정상 부팅 시
```
[EnhancedWDT] 초기화 시작...
[EnhancedWDT] 초기화 완료 (타임아웃: 10초)
[EnhancedWDT] ✅ 태스크 등록: VacuumCtrl (간격: 5000ms)
[EnhancedWDT] ✅ 태스크 등록: SensorRead (간격: 1000ms)
[EnhancedWDT] ✅ 태스크 등록: UIUpdate (간격: 100ms)
[EnhancedWDT] ✅ 태스크 등록: WiFiMgr (간격: 10000ms)
```

### 이전 재시작 감지 시
```
[EnhancedWDT] ⚠️ 이전 재시작 감지:

╔═══════════════════════════════════════╗
║       재시작 정보                     ║
╠═══════════════════════════════════════╣
║ 원인: 데드락 감지                    ║
║ 시간: 120 초 전                      ║
║ 태스크: SensorRead                   ║
║ 총 재시작 횟수: 3회                  ║
╚═══════════════════════════════════════╝
```

### 데드락 감지 시
```
╔═══════════════════════════════════════╗
║   ⚠️  데드락 감지! ⚠️                ║
╠═══════════════════════════════════════╣
║ 태스크: SensorRead                   ║
║ 미응답: 3회 연속                     ║
║ 마지막 체크인: 15342 ms 전           ║
║                                       ║
║ 5초 후 자동 재시작합니다...          ║
╚═══════════════════════════════════════╝
```

---

## 🎮 시리얼 명령어

시리얼 모니터에서 다음 명령어 사용 가능:

```bash
# Watchdog 전체 상태 보기
wdt_status

# 특정 태스크 상세 정보
wdt_task VacuumCtrl

# 재시작 히스토리
wdt_history

# 강제 재시작 (테스트용)
wdt_restart
```

### 출력 예시: wdt_status
```
╔═══════════════════════════════════════╗
║     Enhanced Watchdog 상태            ║
╠═══════════════════════════════════════╣
║ 활성화: 예                            ║
║ 가동 시간: 1234 초                    ║
║ 등록 태스크: 4개                      ║
║ 전체 상태: ✅ 정상                    ║
╠═══════════════════════════════════════╣
║ VacuumCtrl       ✅ 정상 (234ms)      ║
║ SensorRead       ✅ 정상 (12ms)       ║
║ UIUpdate         ✅ 정상 (45ms)       ║
║ WiFiMgr          ⚠️  느림 (12500ms)   ║
╚═══════════════════════════════════════╝
```

---

## 🎨 선택 기능: UI 화면 추가

### 1. Config.h에 화면 추가
```cpp
enum ScreenType {
    // ... 기존 화면들 ...
    
    SCREEN_WATCHDOG_STATUS,  // ✅ 추가
};
```

### 2. 화면 구현 (UI_Screen_WatchdogStatus.cpp)
```cpp
#include "../include/UIComponents.h"
#include "../include/Config.h"
#include "EnhancedWatchdog.h"

using namespace UIComponents;
using namespace UITheme;

void drawWatchdogStatusScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    // 헤더
    drawHeader("시스템 모니터");
    
    int16_t y = HEADER_HEIGHT + SPACING_MD;
    
    // 전체 상태 카드
    CardConfig statusCard = {
        .x = SPACING_SM,
        .y = y,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 60,
        .bgColor = enhancedWatchdog.isHealthy() ? COLOR_SUCCESS : COLOR_DANGER
    };
    drawCard(statusCard);
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING);
    tft.print(enhancedWatchdog.isHealthy() ? "정상" : "경고");
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 22);
    tft.printf("가동: %lu초", enhancedWatchdog.getUptimeSeconds());
    
    y += statusCard.h + SPACING_SM;
    
    // 태스크 상태 목록
    const char* taskNames[] = {"VacuumCtrl", "SensorRead", "UIUpdate", "WiFiMgr"};
    
    for (int i = 0; i < 4; i++) {
        TaskInfo* task = enhancedWatchdog.getTaskInfo(taskNames[i]);
        if (!task) continue;
        
        CardConfig taskCard = {
            .x = SPACING_SM,
            .y = y,
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = 40,
            .bgColor = COLOR_BG_CARD
        };
        drawCard(taskCard);
        
        // 태스크 이름
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(taskCard.x + CARD_PADDING, taskCard.y + CARD_PADDING);
        tft.print(task->name);
        
        // 상태 배지
        const char* statusText;
        BadgeType badgeType;
        
        switch (task->status) {
            case TASK_HEALTHY:
                statusText = "정상";
                badgeType = BADGE_SUCCESS;
                break;
            case TASK_SLOW:
                statusText = "느림";
                badgeType = BADGE_WARNING;
                break;
            case TASK_STALLED:
                statusText = "정지";
                badgeType = BADGE_DANGER;
                break;
            default:
                statusText = "미확인";
                badgeType = BADGE_INFO;
                break;
        }
        
        drawBadge(taskCard.x + taskCard.w - 60, taskCard.y + CARD_PADDING, 
                  statusText, badgeType);
        
        y += taskCard.h + 4;
    }
    
    // 네비게이션
    NavButton navButtons[] = {
        {"뒤로", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

void handleWatchdogStatusTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    if (y >= navY) {
        currentScreen = SCREEN_SETTINGS;
        screenNeedsRedraw = true;
    }
}
```

### 3. Settings 화면에서 접근 추가
```cpp
// Settings 메뉴에 추가
{"시스템", "모니터", COLOR_INFO, SCREEN_WATCHDOG_STATUS, true},
```

---

## 🔧 고급 설정

### 타임아웃 조정
```cpp
// 더 짧은 타임아웃 (빠른 반응)
enhancedWatchdog.begin(5);   // 5초

// 더 긴 타임아웃 (안정성 우선)
enhancedWatchdog.begin(30);  // 30초
```

### 태스크 체크인 간격 조정
```cpp
// 빠른 태스크 (100ms)
enhancedWatchdog.registerTask("FastTask", 100);

// 느린 태스크 (1분)
enhancedWatchdog.registerTask("SlowTask", 60000);
```

### 일시적 비활성화
```cpp
// OTA 업데이트 중에는 비활성화
enhancedWatchdog.disable();
performOTAUpdate();
enhancedWatchdog.enable();
```

---

## ⚠️ 주의사항

### 1. 체크인 간격 설정
- 너무 짧으면: 오탐지 발생 가능
- 너무 길면: 문제 감지가 늦어짐
- **권장**: 실제 태스크 실행 주기의 2배

### 2. 필수 체크인
각 태스크는 **반드시** 주기적으로 체크인해야 합니다:
```cpp
// ❌ 잘못된 예
void task() {
    while (1) {
        doWork();
        // 체크인 없음 - 데드락 오판!
        vTaskDelay(100);
    }
}

// ✅ 올바른 예
void task() {
    while (1) {
        doWork();
        WDT_CHECKIN("TaskName");  // ← 필수!
        vTaskDelay(100);
    }
}
```

### 3. 긴 작업 처리
```cpp
// 긴 작업 중에는 중간에도 체크인
void longTask() {
    for (int i = 0; i < 1000; i++) {
        heavyWork(i);
        
        if (i % 100 == 0) {
            WDT_CHECKIN("LongTask");  // 100번마다
        }
    }
}
```

---

## 📊 성능 영향

### 메모리 사용
- RAM: ~1KB (태스크 정보 저장)
- Flash: ~8KB (코드)

### CPU 사용
- 체크인: < 1μs
- 업데이트: ~100μs (1초마다)
- 영향: **무시할 수 있는 수준**

---

## 🐛 문제 해결

### Q1: "태스크 등록 실패: 최대 개수 초과"
```cpp
// EnhancedWatchdog.h에서 MAX_TASK_MONITORS 증가
#define MAX_TASK_MONITORS   12  // 8 → 12로 증가
```

### Q2: 오탐지 (정상인데 데드락으로 판정)
```cpp
// 체크인 간격을 더 길게 설정
enhancedWatchdog.registerTask("SlowTask", 15000);  // 10초 → 15초
```

### Q3: 재시작 루프
```cpp
// setup()에서 재시작 원인 확인
RestartInfo info = enhancedWatchdog.getLastRestartInfo();
if (info.restartCount > 5) {
    // 5번 이상 재시작 시 안전 모드 진입
    enterSafeMode();
}
```

---

## 🎯 다음 단계

Enhanced Watchdog 적용 후:

1. ✅ **Config Backup/Restore** 추가
2. ✅ **Safe Mode** 구현
3. ✅ **Network Resilience** 개선

각 기능은 독립적으로 추가 가능합니다.

---

## 📝 체크리스트

Phase 3-1 Enhanced Watchdog 통합:

- [ ] EnhancedWatchdog.h 파일 추가
- [ ] EnhancedWatchdog.cpp 파일 추가
- [ ] main.cpp에 헤더 include
- [ ] setup()에 초기화 코드 추가
- [ ] loop()에 update() 추가
- [ ] 각 FreeRTOS 태스크에 체크인 추가
- [ ] 컴파일 및 업로드
- [ ] 시리얼 모니터로 동작 확인
- [ ] wdt_status 명령어 테스트
- [ ] 강제 재시작 테스트 (wdt_restart)

**예상 소요 시간**: 10-15분

---

**작성일**: 2026-02-15  
**버전**: Phase 3-1 Enhanced Watchdog v1.0  
**다음**: Config Backup/Restore 구현
