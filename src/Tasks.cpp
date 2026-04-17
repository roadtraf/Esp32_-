// ================================================================
// Tasks_Hardened.cpp - v3.9.4 Hardened FreeRTOS 
// ================================================================
// :
//   [2]  Task WDT:     
//        WiFi Task: wifiManagerStep    
//   [8]  UI Task: TFT/   SPI  
//   [9]  DS18B20:  ds18b20Task  (SafeSensor.h)
//   [7]  Heap:     
// ================================================================
#include "Tasks.h"
#include "Config.h"
#include "DataLogger.h"
#include "EnhancedWatchdog.h"
#include "HardenedConfig.h"
#include "SPIBusManager.h"
#include "SafeSensor.h"

// ================================================================
//  
// ================================================================
TaskHandle_t vacuumTaskHandle   = NULL;
TaskHandle_t sensorTaskHandle   = NULL;
TaskHandle_t uiTaskHandle       = NULL;
TaskHandle_t wifiTaskHandle     = NULL;
TaskHandle_t mqttTaskHandle     = NULL;
TaskHandle_t loggerTaskHandle   = NULL;
TaskHandle_t healthTaskHandle   = NULL;
TaskHandle_t predictorTaskHandle = NULL;
TaskHandle_t ds18b20TaskHandle  = NULL;   // [] DS18B20 

// ================================================================
//  
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

#endif

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
extern HealthMonitor healthMonitor;
// extern MLPredictor mlPredictor;  // MLPredictor 
extern Statistics    stats;
#endif

extern uint32_t g_estopStartMs;  // UI_Screen_EStop.cpp 

// ================================================================
//     [R1]
// Tasks.cpp  (static  ) 
// ================================================================
struct SensorSnapshot {
    float pressure    = 9999.0f;
    float temperature = 9999.0f;
    float current     = 9999.0f;
    SystemState state = STATE_IDLE;
    bool  errorActive = false;
};

//  
static constexpr float PRESSURE_DELTA  = 0.2f;   // kPa
static constexpr float TEMP_DELTA      = 0.3f;   // C
static constexpr float CURRENT_DELTA   = 0.05f;  // A

//   
static SensorSnapshot g_lastSensor;

// ================================================================
// [2] WDT :     
// :   WDT_TIMEOUT   WiFi   reset
// ================================================================
static void registerAllTasks() {
    enhancedWatchdog.registerTask("VacuumCtrl", WDT_TIMEOUT_TASK_VACUUM);
    enhancedWatchdog.registerTask("SensorRead", WDT_TIMEOUT_TASK_SENSOR);
    enhancedWatchdog.registerTask("UIUpdate",   WDT_TIMEOUT_TASK_UI);
    enhancedWatchdog.registerTask("WiFiMgr",    WDT_TIMEOUT_TASK_WIFI);   // 30s
    enhancedWatchdog.registerTask("MQTTHandler",WDT_TIMEOUT_TASK_MQTT);
    enhancedWatchdog.registerTask("DataLogger", WDT_TIMEOUT_TASK_LOGGER);
    enhancedWatchdog.registerTask("DS18B20",    5000);
    // HealthMon, Predictor WDT   (,  )
}

// ================================================================
// STEP FUNCTIONS
// ================================================================

//  1. Vacuum Control 
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

//  2. Sensor Read 
// ================================================================
// [R1] sensorReadStep() 
// [9] DS18B20 safeDS18B20.getTemperature()  ()
// ================================================================
static void sensorReadStep() {
    readSensors();
    updateSensorBuffers();
    checkSensorHealth();

    // [9] DS18B20   
    if (safeDS18B20.isPresent()) {
        sensorData.temperature = safeDS18B20.getTemperature();
    }

    // [R1]    
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

//  3. uiUpdateStep 
// ================================================================
// [R2] uiUpdateStep() 
// [8] TFT/   SPI  
// ================================================================
static void uiUpdateStep() {
    static uint32_t lastUpdate   = 0;
    static uint8_t  lastEstopSec = 0xFF;

    uint32_t now = millis();

    // [8] : SPI    
    {
        SPIGuard touchGuard(SPI_DEV_TOUCH, SPI_MUTEX_TIMEOUT_MS);
        if (touchGuard.acquired()) {
            handleTouch();
        }
    }

    if (keyboardConnected) {
        handleKeyboardInput();
    }

    // [8] TFT : SPI    
    if (now - lastUpdate >= 200) {
        lastUpdate = now;

        // [R2] E-Stop : 1  redraw
        if (currentScreen == SCREEN_ESTOP) {
            uint8_t currentSec = (uint8_t)((millis() - g_estopStartMs) / 1000);
            if (currentSec != lastEstopSec) {
                lastEstopSec = currentSec;
                screenNeedsRedraw = true;
            }
        }

       WDT_FEED();
       updateUI();
       WDT_FEED();
       // tft.flush(); // disabled
       WDT_FEED();
       screenNeedsRedraw = false;
        if (currentState == STATE_IDLE) {
           // enterSleepMode();
        }
    }

    WDT_CHECKIN("UIUpdate");
}

//  4. WiFi Manager 
// ================================================================
// [6]     
// : connectWiFi()  WiFi.begin() + while(WL_CONNECTED)  10s 
//        WiFiMgr WDT  
// :     (  500ms )
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
                Serial.println("[WiFiMgr]   ()");
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
            // []    500ms   WDT feed 
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("[WiFiMgr]  WiFi  ");
                backoffDelay = WIFI_BACKOFF_BASE_MS;  //  
                wifiState = WIFI_SM_CONNECTED;
            } else if (now - stepTime >= WIFI_CONNECT_STEP_MS) {
                stepTime = now;
                stepCount++;

                if (stepCount >= WIFI_MAX_CONNECT_STEPS) {
                    Serial.printf("[WiFiMgr]    %lums \n", backoffDelay);
                    WiFi.disconnect();
                    stepTime  = now;
                    wifiState = WIFI_SM_BACKOFF;
                }
            }
            break;

        case WIFI_SM_CONNECTED:
            //   
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFiMgr]    ");
                wifiState = WIFI_SM_BEGIN;
            }
            break;

        case WIFI_SM_BACKOFF:
            //   
            if (now - stepTime >= backoffDelay) {
                //   ( 30s)
                backoffDelay = min(backoffDelay * WIFI_BACKOFF_MULTIPLIER,
                                   (uint32_t)WIFI_BACKOFF_MAX_MS);
                wifiState = WIFI_SM_BEGIN;
            }
            break;
    }

    WDT_CHECKIN("WiFiMgr");
}

//  5. MQTT Handler 
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

//  6. Data Logger 
// ================================================================
// [4] SafeSD   SD  ( SD_Logger.cpp )
// ================================================================
static void dataLoggerStep() {
#ifdef ENABLE_DATA_LOGGING
    // SafeSDManager  SPIGuard 
    dataLogger.logHealthData(healthMonitor);
#endif

    checkSDWriteStatus();

    WDT_CHECKIN("DataLogger");
}

//  7. Health Monitor 
static void healthMonitorStep() {
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate >= 5000) {
        healthMonitor.update(
            sensorData.pressure,
            sensorData.temperature,
            sensorData.current,
            stats.totalCycles,
            stats.uptime
        );
        lastUpdate = now;
    }
#endif
}

//  8. Predictor 
static void predictorStep() {
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate >= 10000) {
        // mlPredictor call disabled  // 
        lastUpdate = now;
    }
#endif
}

//  9. [] Heap  (DataLogger Task  ) 
// [7]     
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

        Serial.printf("[Heap] : %u | : %u | : %u | PSRAM: %u\n",
                      freeHeap, minFreeHeap, largestBlock, freePSRAM);

        if (freeHeap < HEAP_WARN_THRESHOLD) {
            Serial.printf("[Heap]     ! %u bytes \n", freeHeap);
        }

        //     PSRAM   
        if (freeHeap < INTERNAL_HEAP_MIN_FREE && psramFound()) {
            Serial.println("[Heap]    PSRAM_SAFE_ALLOC()  ");
        }

        // SPI   
        SPIBusManager::getInstance().printStats();
    }
}

// ================================================================
//  Task Loop
// ================================================================
static void taskLoop(void (*stepFunc)(), uint32_t intervalMs) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(intervalMs);

    for (;;) {
        stepFunc();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

// DataLogger Task: SD  +   
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
void wifiManagerTask(void* param)   { taskLoop(wifiManagerStep,           500); }  // [] 5000500 ()
void mqttHandlerTask(void* param)   { taskLoop(mqttHandlerStep,           100); }
void dataLoggerTask(void* param)    { taskLoop(dataLoggerAndMonitorStep, 1000); }  // []   
void healthMonitorTask(void* param) { taskLoop(healthMonitorStep,        1000); }
void predictorTask(void* param)     { taskLoop(predictorStep,            1000); }

// [] DS18B20  
void ds18b20TaskWrapper(void* param) {
    for (;;) {
        safeDS18B20.step();
        WDT_CHECKIN("DS18B20");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ================================================================
// TASK  (  )
// ================================================================
void createAllTasks() {
    Serial.println("[Tasks] v3.9.4 Hardened FreeRTOS   ...");

    // WDT   (  )
    registerAllTasks();

    //  Core 1: /UI/ ( ) 
    xTaskCreatePinnedToCore(
        vacuumControlTask, "VacuumCtrl",
        4096, NULL, 3,  //  3 ()
        &vacuumTaskHandle, 1);

    xTaskCreatePinnedToCore(
        sensorReadTask, "SensorRead",
        3072, NULL, 2,
        &sensorTaskHandle, 1);

    xTaskCreatePinnedToCore(
        uiUpdateTask, "UIUpdate",
        8192, NULL, 1,  // [8]  : SPI Guard 
        &uiTaskHandle, 1);

    xTaskCreatePinnedToCore(
        predictorTask, "Predictor",
        4096, NULL, 1,
        &predictorTaskHandle, 1);

    // [] DS18B20   (Core 1,  )
    xTaskCreatePinnedToCore(
        ds18b20TaskWrapper, "DS18B20",
        DS18B20_TASK_STACK, NULL, DS18B20_TASK_PRIORITY,
        &ds18b20TaskHandle, 1);

    //  Core 0: / 
    xTaskCreatePinnedToCore(
        wifiManagerTask, "WiFiMgr",
        4096, NULL, 2,  // []  12 ( )
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

    Serial.println("[Tasks]  v3.9.4 Hardened   ");
    Serial.println("[Tasks] :");
    Serial.println("  [2] Task WDT   ");
    Serial.println("  [6] WiFi   ");
    Serial.println("  [8] SPI  (TFT/Touch/SD  )");
    Serial.println("  [9] DS18B20   ");
    Serial.println("  [7]   ");
}

/*
v3.9.4 Hardened   :

  ( ):
  Core 1: VacuumCtrl(p3) > SensorRead(p2) > UIUpdate(p1) > DS18B20(p1) > Predictor(p1)
  Core 0: WiFiMgr(p2) > MQTTHandler(p1) > DataLogger(p1) > HealthMon(p1)

 :
  1. WiFiMgr: 5000ms  connectWiFi()  500ms  (WDT )
  2. UIUpdate: SPI Guard TFT/Touch  
  3. DataLogger: SafeSD +   
  4. DS18B20:    (SensorRead  )
*/
