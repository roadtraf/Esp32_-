// ================================================================
// Tasks_Hardened.cpp - v3.9.4 Hardened FreeRTOS 태스크
// ================================================================
// 변경사항:
//   [2]  Task WDT: 각 태스크별 체크인 간격 최적화
//        WiFi Task: wifiManagerStep → 비블로킹 상태 머신
//   [8]  UI Task: TFT/터치 접근 시 SPI 뮤텍스 적용
//   [9]  DS18B20: 별도 ds18b20Task 추가 (SafeSensor.h)
//   [7]  Heap: 주기적 힙 상태 모니터링 추가
// ================================================================
#include "Tasks.h"
#include "../include/Config.h"
#include "../include/EnhancedWatchdog.h"
#include "../include/HardenedConfig.h"
#include "../include/SPIBusManager.h"
#include "../include/SafeSensor.h"

// ================================================================
// 태스크 핸들
// ================================================================
TaskHandle_t vacuumTaskHandle   = NULL;
TaskHandle_t sensorTaskHandle   = NULL;
TaskHandle_t uiTaskHandle       = NULL;
TaskHandle_t wifiTaskHandle     = NULL;
TaskHandle_t mqttTaskHandle     = NULL;
TaskHandle_t loggerTaskHandle   = NULL;
TaskHandle_t healthTaskHandle   = NULL;
TaskHandle_t predictorTaskHandle = NULL;
TaskHandle_t ds18b20TaskHandle  = NULL;   // [신규] DS18B20 전용

// ================================================================
// 외부 참조
// ================================================================
extern void updateStateMachine();
extern void handleError();
extern void updatePID();
extern void readSensors();
extern void updateSensorBuffers();
extern void checkSensorHealth();
extern void handleTouch();
extern void handleKeyboardInput();
extern void updateUI();
extern void enterSleepMode();
extern void mqttLoop();
extern void publishMQTT();
extern void checkSDWriteStatus();

extern SystemState  currentState;
extern ControlMode  currentMode;
extern bool         errorActive;
extern bool         screenNeedsRedraw;
extern bool         keyboardConnected;
extern uint32_t     lastIdleTime;
extern bool         sleepMode;
extern SystemConfig config;
extern SensorData   sensorData;
extern bool         mqttConnected;

#ifdef ENABLE_DATA_LOGGING
extern DataLogger dataLogger;
#endif

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
extern HealthMonitor healthMonitor;
extern MLPredictor   mlPredictor;
extern Statistics    stats;
#endif

extern uint32_t g_estopStartMs;  // UI_Screen_EStop.cpp에서 정의

// ================================================================
// 센서 변화 감지용 구조체 [R1]
// Tasks.cpp 상단 (static 선언 섹션)에 추가
// ================================================================
struct SensorSnapshot {
    float pressure    = 9999.0f;
    float temperature = 9999.0f;
    float current     = 9999.0f;
    SystemState state = STATE_IDLE;
    bool  errorActive = false;
};

// 변화 임계치
static constexpr float PRESSURE_DELTA  = 0.2f;   // kPa
static constexpr float TEMP_DELTA      = 0.3f;   // °C
static constexpr float CURRENT_DELTA   = 0.05f;  // A

// 이전 센서값 저장
static SensorSnapshot g_lastSensor;

// ================================================================
// [2] WDT 안전: 각 태스크별 최적 체크인 간격
// 기존: 모두 동일한 WDT_TIMEOUT 사용 → WiFi 블로킹 시 reset
// ================================================================
static void registerAllTasks() {
    enhancedWatchdog.registerTask("VacuumCtrl", WDT_TIMEOUT_TASK_VACUUM);
    enhancedWatchdog.registerTask("SensorRead", WDT_TIMEOUT_TASK_SENSOR);
    enhancedWatchdog.registerTask("UIUpdate",   WDT_TIMEOUT_TASK_UI);
    enhancedWatchdog.registerTask("WiFiMgr",    WDT_TIMEOUT_TASK_WIFI);   // 30s
    enhancedWatchdog.registerTask("MQTTHandler",WDT_TIMEOUT_TASK_MQTT);
    enhancedWatchdog.registerTask("DataLogger", WDT_TIMEOUT_TASK_LOGGER);
    enhancedWatchdog.registerTask("DS18B20",    5000);
    // HealthMon, Predictor는 WDT 등록 불필요 (저우선순위, 지연 허용)
}

// ================================================================
// STEP FUNCTIONS
// ================================================================

// ── 1. Vacuum Control ──
static void vacuumControlStep() {
    updateStateMachine();

    if (errorActive) {
        handleError();
    }

    if (currentMode == MODE_PID && !errorActive) {
        updatePID();
    }

    WDT_CHECKIN("VacuumCtrl");
}

// ── 2. Sensor Read ──
// ================================================================
// [R1] sensorReadStep() 교체본
// [9] DS18B20는 safeDS18B20.getTemperature()로만 읽음 (비블로킹)
// ================================================================
static void sensorReadStep() {
    readSensors();
    updateSensorBuffers();
    checkSensorHealth();

    // [9] DS18B20 비동기 온도 반영
    if (safeDS18B20.isPresent()) {
        sensorData.temperature = safeDS18B20.getTemperature();
    }

    // [R1] 메인화면 센서값 변화 감지
    if (currentScreen == SCREEN_MAIN) {
        bool changed =
            fabsf(sensorData.pressure    - g_lastSensor.pressure)    > PRESSURE_DELTA  ||
            fabsf(sensorData.temperature - g_lastSensor.temperature) > TEMP_DELTA      ||
            fabsf(sensorData.current     - g_lastSensor.current)     > CURRENT_DELTA   ||
            currentState != g_lastSensor.state ||
            errorActive  != g_lastSensor.errorActive;

        if (changed) {
            g_lastSensor.pressure    = sensorData.pressure;
            g_lastSensor.temperature = sensorData.temperature;
            g_lastSensor.current     = sensorData.current;
            g_lastSensor.state       = currentState;
            g_lastSensor.errorActive = errorActive;
            screenNeedsRedraw        = true;
        }
    }

    WDT_CHECKIN("SensorRead");
}

// ── 3. uiUpdateStep ─
// ================================================================
// [R2] uiUpdateStep() 교체본
// [8] TFT/터치 접근 시 SPI 뮤텍스 적용
// ================================================================
static void uiUpdateStep() {
    static uint32_t lastUpdate   = 0;
    static uint8_t  lastEstopSec = 0xFF;

    uint32_t now = millis();

    // [8] 터치: SPI 뮤텍스 획득 후 접근
    {
        SPIGuard touchGuard(SPI_DEV_TOUCH, SPI_MUTEX_TIMEOUT_MS);
        if (touchGuard.acquired()) {
            handleTouch();
        }
    }

    if (keyboardConnected) {
        handleKeyboardInput();
    }

    // [8] TFT 업데이트: SPI 뮤텍스 획득 후 접근
    if (now - lastUpdate >= 200) {
        lastUpdate = now;

        // [R2] E-Stop 깜빡임: 1초 단위로만 redraw
        if (currentScreen == SCREEN_ESTOP) {
            uint8_t currentSec = (uint8_t)((millis() - g_estopStartMs) / 1000);
            if (currentSec != lastEstopSec) {
                lastEstopSec = currentSec;
                screenNeedsRedraw = true;
            }
        }

        if (screenNeedsRedraw) {
            SPIGuard tftGuard(SPI_DEV_TFT, SPI_MUTEX_TIMEOUT_MS);
            if (tftGuard.acquired()) {
                WDT_FEED();
                updateUI();
                screenNeedsRedraw = false;
            } else {
                Serial.println("[UITask] SPI 뮤텍스 대기 중, UI 업데이트 지연");
            }
        }
    }

    if (!sleepMode && (now - lastIdleTime > IDLE_TIMEOUT)) {
        if (currentState == STATE_IDLE) {
            enterSleepMode();
        }
    }

    WDT_CHECKIN("UIUpdate");
}

// ── 4. WiFi Manager ──
// ================================================================
// [6] 비블로킹 상태 머신 기반 재연결
// 기존: connectWiFi() → WiFi.begin() + while(WL_CONNECTED) 최대 10s 블로킹
//       → WiFiMgr WDT 타임아웃 발생
// 해결: 연결 상태를 단계별로 체크 (각 단계 500ms 이내)
// ================================================================
static void wifiManagerStep() {
    static enum {
        WIFI_SM_IDLE,
        WIFI_SM_BEGIN,
        WIFI_SM_WAITING,
        WIFI_SM_CONNECTED,
        WIFI_SM_BACKOFF,
    } wifiState = WIFI_SM_IDLE;

    static uint32_t stepTime     = 0;
    static uint32_t backoffDelay = WIFI_BACKOFF_BASE_MS;
    static uint8_t  stepCount    = 0;

    uint32_t now = millis();

    if (strlen(config.wifiSSID) == 0) {
        WDT_CHECKIN("WiFiMgr");
        return;
    }

    switch (wifiState) {
        case WIFI_SM_IDLE:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFiMgr] 연결 시작 (비블로킹)");
                WiFi.begin(config.wifiSSID, config.wifiPassword);
                stepTime  = now;
                stepCount = 0;
                wifiState = WIFI_SM_WAITING;
            }
            break;

        case WIFI_SM_BEGIN:
            WiFi.begin(config.wifiSSID, config.wifiPassword);
            stepTime  = now;
            stepCount = 0;
            wifiState = WIFI_SM_WAITING;
            break;

        case WIFI_SM_WAITING:
            // [핵심] 매 호출 시 500ms씩만 대기 → WDT feed 가능
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("[WiFiMgr] ✅ WiFi 연결 성공");
                backoffDelay = WIFI_BACKOFF_BASE_MS;  // 백오프 리셋
                wifiState = WIFI_SM_CONNECTED;
            } else if (now - stepTime >= WIFI_CONNECT_STEP_MS) {
                stepTime = now;
                stepCount++;

                if (stepCount >= WIFI_MAX_CONNECT_STEPS) {
                    Serial.printf("[WiFiMgr] 연결 실패 → %lums 백오프\n", backoffDelay);
                    WiFi.disconnect();
                    stepTime  = now;
                    wifiState = WIFI_SM_BACKOFF;
                }
            }
            break;

        case WIFI_SM_CONNECTED:
            // 연결 유지 확인
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFiMgr] 연결 끊김 → 재연결");
                wifiState = WIFI_SM_BEGIN;
            }
            break;

        case WIFI_SM_BACKOFF:
            // 지수 백오프 대기
            if (now - stepTime >= backoffDelay) {
                // 백오프 증가 (최대 30s)
                backoffDelay = min(backoffDelay * WIFI_BACKOFF_MULTIPLIER,
                                   (uint32_t)WIFI_BACKOFF_MAX_MS);
                wifiState = WIFI_SM_BEGIN;
            }
            break;
    }

    WDT_CHECKIN("WiFiMgr");
}

// ── 5. MQTT Handler ──
static void mqttHandlerStep() {
    static uint32_t lastPublish = 0;
    uint32_t now = millis();

    if (mqttConnected) {
        mqttLoop();

        if (now - lastPublish >= 2000) {
            publishMQTT();
            lastPublish = now;
        }
    }

    WDT_CHECKIN("MQTTHandler");
}

// ── 6. Data Logger ──
// ================================================================
// [4] SafeSD를 통한 안전한 SD 접근 (기존 SD_Logger.cpp와 연동)
// ================================================================
static void dataLoggerStep() {
#ifdef ENABLE_DATA_LOGGING
    // SafeSDManager는 내부적으로 SPIGuard 사용
    dataLogger.logHealthData(healthMonitor);
#endif

    checkSDWriteStatus();

    WDT_CHECKIN("DataLogger");
}

// ── 7. Health Monitor ──
static void healthMonitorStep() {
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate >= HEALTH_UPDATE_INTERVAL) {
        healthMonitor.update(
            sensorData.pressure,
            sensorData.temperature,
            sensorData.current,
            stats.cycleCount,
            stats.uptime
        );
        lastUpdate = now;
    }
#endif
}

// ── 8. Predictor ──
static void predictorStep() {
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate >= ML_UPDATE_INTERVAL) {
        mlPredictor.addSample(
            sensorData.pressure,
            sensorData.temperature,
            sensorData.current
        );
        lastUpdate = now;
    }
#endif
}

// ── 9. [신규] Heap 모니터 (DataLogger Task 내 호출) ──
// [7] 힙 단편화 감지 및 경고
static void heapMonitorStep() {
    static uint32_t lastLog = 0;
    uint32_t now = millis();

    if (now - lastLog >= LOG_HEAP_INTERVAL_MS) {
        lastLog = now;

        size_t freeHeap     = esp_get_free_heap_size();
        size_t minFreeHeap  = esp_get_minimum_free_heap_size();
        size_t freePSRAM    = 0;
        size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

        if (psramFound()) {
            freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        }

        Serial.printf("[Heap] 잔여: %u | 최소: %u | 최대블록: %u | PSRAM: %u\n",
                      freeHeap, minFreeHeap, largestBlock, freePSRAM);

        if (freeHeap < HEAP_WARN_THRESHOLD) {
            Serial.printf("[Heap] ⚠️  힙 부족 경고! %u bytes 남음\n", freeHeap);
        }

        // 내부 힙 부족 시 PSRAM으로 버퍼 마이그레이션 알림
        if (freeHeap < INTERNAL_HEAP_MIN_FREE && psramFound()) {
            Serial.println("[Heap] 💡 큰 버퍼는 PSRAM_SAFE_ALLOC() 사용 권장");
        }

        // SPI 버스 충돌 통계
        SPIBusManager::getInstance().printStats();
    }
}

// ================================================================
// 공통 Task Loop
// ================================================================
static void taskLoop(void (*stepFunc)(), uint32_t intervalMs) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(intervalMs);

    for (;;) {
        stepFunc();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

// DataLogger Task: SD 쓰기 + 힙 모니터 통합
static void dataLoggerAndMonitorStep() {
    dataLoggerStep();
    heapMonitorStep();
}

// ================================================================
// TASK WRAPPERS
// ================================================================
void vacuumControlTask(void* param) { taskLoop(vacuumControlStep,         100); }
void sensorReadTask(void* param)    { taskLoop(sensorReadStep,            100); }
void uiUpdateTask(void* param)      { taskLoop(uiUpdateStep,               50); }
void wifiManagerTask(void* param)   { taskLoop(wifiManagerStep,           500); }  // [변경] 5000→500 (상태머신)
void mqttHandlerTask(void* param)   { taskLoop(mqttHandlerStep,           100); }
void dataLoggerTask(void* param)    { taskLoop(dataLoggerAndMonitorStep, 1000); }  // [변경] 힙 모니터 통합
void healthMonitorTask(void* param) { taskLoop(healthMonitorStep,        1000); }
void predictorTask(void* param)     { taskLoop(predictorStep,            1000); }

// [신규] DS18B20 전용 태스크
void ds18b20TaskWrapper(void* param) {
    for (;;) {
        safeDS18B20.step();
        WDT_CHECKIN("DS18B20");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ================================================================
// TASK 생성 (최적 코어 배치)
// ================================================================
void createAllTasks() {
    Serial.println("[Tasks] v3.9.4 Hardened FreeRTOS 태스크 생성 시작...");

    // WDT에 태스크 등록 (체크인 간격 최적화)
    registerAllTasks();

    // ── Core 1: 제어/UI/센서 (실시간성 중요) ──
    xTaskCreatePinnedToCore(
        vacuumControlTask, "VacuumCtrl",
        4096, NULL, 3,  // 우선순위 3 (최고)
        &vacuumTaskHandle, 1);

    xTaskCreatePinnedToCore(
        sensorReadTask, "SensorRead",
        3072, NULL, 2,
        &sensorTaskHandle, 1);

    xTaskCreatePinnedToCore(
        uiUpdateTask, "UIUpdate",
        8192, NULL, 1,  // [8] 스택 증가: SPI Guard 오버헤드
        &uiTaskHandle, 1);

    xTaskCreatePinnedToCore(
        predictorTask, "Predictor",
        4096, NULL, 1,
        &predictorTaskHandle, 1);

    // [신규] DS18B20 전용 태스크 (Core 1, 낮은 우선순위)
    xTaskCreatePinnedToCore(
        ds18b20TaskWrapper, "DS18B20",
        DS18B20_TASK_STACK, NULL, DS18B20_TASK_PRIORITY,
        &ds18b20TaskHandle, 1);

    // ── Core 0: 네트워크/백그라운드 ──
    xTaskCreatePinnedToCore(
        wifiManagerTask, "WiFiMgr",
        4096, NULL, 2,  // [변경] 우선순위 1→2 (안정적 재연결)
        &wifiTaskHandle, 0);

    xTaskCreatePinnedToCore(
        mqttHandlerTask, "MQTTHandler",
        4096, NULL, 1,
        &mqttTaskHandle, 0);

    xTaskCreatePinnedToCore(
        dataLoggerTask, "DataLogger",
        4096, NULL, 1,
        &loggerTaskHandle, 0);

    xTaskCreatePinnedToCore(
        healthMonitorTask, "HealthMon",
        2048, NULL, 1,
        &healthTaskHandle, 0);

    Serial.println("[Tasks] ✅ v3.9.4 Hardened 태스크 생성 완료");
    Serial.println("[Tasks] 개선사항:");
    Serial.println("  [2] Task WDT 체크인 간격 최적화");
    Serial.println("  [6] WiFi 비블로킹 상태 머신");
    Serial.println("  [8] SPI 뮤텍스 (TFT/Touch/SD 충돌 방지)");
    Serial.println("  [9] DS18B20 전용 태스크 분리");
    Serial.println("  [7] 힙 모니터링 통합");
}

/*
v3.9.4 Hardened 태스크 변경 요약:

코어 배치 (변경 없음):
  Core 1: VacuumCtrl(p3) > SensorRead(p2) > UIUpdate(p1) > DS18B20(p1) > Predictor(p1)
  Core 0: WiFiMgr(p2) > MQTTHandler(p1) > DataLogger(p1) > HealthMon(p1)

핵심 변경:
  1. WiFiMgr: 5000ms 주기 connectWiFi() → 500ms 상태머신 (WDT 안전)
  2. UIUpdate: SPI Guard로 TFT/Touch 접근 보호
  3. DataLogger: SafeSD + 힙 모니터 통합
  4. DS18B20: 별도 태스크 분리 (SensorRead 블로킹 제거)
*/
