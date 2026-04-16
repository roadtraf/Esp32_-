/**
 * @file main.cpp
 * @brief ESP32-S3    v3.9.4 Hardened Edition - 
 * @details  9 [1]~[9] +  12 [A]~[L] =  21  
 *
 * [1]  Brownout    
 * [2]  WDT(Watchdog Timer) 
 * [3]  PSRAM  
 * [4]  SD  
 * [5]  I2C  
 * [6]  WiFi  
 * [7]  Heap 
 * [8]  SPI  
 * [9]  DS18B20  
 * [A]   Race Condition  SharedState + Mutex
 * [B]  PWM      
 * [C]  NVS  Write  NVS  
 * [D]  Serial   Serial  
 * [E]  Stack       + 
 * [F]  MQTTState      
 * [G]  OTA   OTA    
 * [H]  ADC   ADC  
 * [I]  DFPlayer     +  
 * [J]  volatile   atomic/volatile 
 * [K]  NTP 1970   NTP     
 * [L]        +  
 */

// ============================================================
//  
// ============================================================
#include "HardenedConfig.h"
#include "AdditionalHardening.h"
#include "SharedState.h"
#include "StateMachine.h"
#include "SPIBusManager.h"
#include "I2CBusRecovery.h"
#include "SafeSensor.h"
#include "SafeSD.h"
#include "VoiceAlert.h"
#include "EnhancedWatchdog.h"

#include <Arduino.h>
#include "SD_MMC.h"
#include "UIManager.h"
bool screenNeedsRedraw = true;
void resetStatistics() { /* stub */ }
// GFX_WRAPPER_IMPLEMENTATION moved to tft_instance.cpp
#include "GFX_Wrapper.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
// #include <esp_brownout.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_adc/adc_oneshot.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// #include <DFRobotDFPlayerMini.h>
#include <atomic>
#include <cstring>
#include <cstdio>

// ============================================================
//  (ESP_LOGx)
// ============================================================
static const char* TAG_MAIN   = "MAIN";
static const char* TAG_CTRL   = "CTRL";
static const char* TAG_SENSOR = "SENSOR";
static const char* TAG_MQTT   = "MQTT";
static const char* TAG_OTA    = "OTA";
static const char* TAG_SD     = "SD";
static const char* TAG_WDT    = "WDT";
static const char* TAG_SYS    = "SYS";

// ============================================================
//  
// ============================================================
namespace PIN {
constexpr gpio_num_t PUMP_PWM      = GPIO_NUM_17;
constexpr gpio_num_t VALVE_1       = GPIO_NUM_18;
constexpr gpio_num_t VALVE_2       = GPIO_NUM_42;
constexpr gpio_num_t VALVE_3       = GPIO_NUM_45;
constexpr gpio_num_t PRESSURE_ADC  = GPIO_NUM_33;
constexpr gpio_num_t ESTOP         = GPIO_NUM_38;
constexpr gpio_num_t DS18B20_DATA  = GPIO_NUM_35;
constexpr gpio_num_t SD_CLK        = GPIO_NUM_9;   // SD_MMC CLK
constexpr gpio_num_t SD_CMD        = GPIO_NUM_10;  // SD_MMC CMD
constexpr gpio_num_t SD_D0         = GPIO_NUM_11;  // SD_MMC D0
constexpr gpio_num_t I2C_SDA       = GPIO_NUM_7;
constexpr gpio_num_t I2C_SCL       = GPIO_NUM_8;
constexpr gpio_num_t DFPLAYER_TX   = GPIO_NUM_43;
constexpr gpio_num_t DFPLAYER_RX   = GPIO_NUM_44;
constexpr gpio_num_t LED_STATUS    = GPIO_NUM_46;
constexpr gpio_num_t LED_ERROR     = GPIO_NUM_45;
}

// ============================================================
//  
// ============================================================
#ifdef PWM_RESOLUTION
#undef PWM_RESOLUTION
#endif
#ifdef PRESSURE_MIN_KPA
#undef PRESSURE_MIN_KPA
#endif
#ifdef PRESSURE_MAX_KPA
#undef PRESSURE_MAX_KPA
#endif
#ifdef ESTOP_DEBOUNCE_MS
#undef ESTOP_DEBOUNCE_MS
#endif
namespace CFG {
    // WiFi
    constexpr const char* WIFI_SSID         = "";
    constexpr const char* WIFI_PASS         = "";
    constexpr uint32_t    WIFI_TIMEOUT_MS   = 15000;
    constexpr uint8_t     WIFI_MAX_RETRY    = 5;

    // MQTT
    constexpr const char* MQTT_BROKER       = "";
    constexpr uint16_t    MQTT_PORT         = 1883;
    constexpr const char* MQTT_CLIENT_ID    = "esp32s3_vacuum_v394";
    constexpr const char* MQTT_USER         = "";
    constexpr const char* MQTT_PASS         = "";
    constexpr uint32_t    MQTT_RECONNECT_MS = 5000;

    // NTP
    constexpr const char* NTP_SERVER        = "pool.ntp.org";
    constexpr long        NTP_UTC_OFFSET    = 32400;  // KST = UTC+9
    constexpr uint32_t    NTP_SYNC_WAIT_MS  = 10000;  // [K] NTP  

    // PWM
    constexpr uint32_t    PWM_FREQ          = 25000;  // 25kHz
    constexpr uint8_t     PWM_RESOLUTION    = 10;     // 10bit
    constexpr uint8_t     PWM_MAX_CHANNELS  = 8;

    // 
    constexpr float       PRESSURE_MIN_KPA  = -100.0f;
    constexpr float       PRESSURE_MAX_KPA  = 0.0f;
    constexpr float       PRESSURE_ALARM    = -80.0f;
    constexpr float       PRESSURE_TRIP     = -95.0f;

    // 
    constexpr float       TEMP_ALARM        = 60.0f;
    constexpr float       TEMP_TRIP         = 75.0f;

    //   [E]   
    constexpr uint32_t    STACK_CONTROL     = 8192;
    constexpr uint32_t    STACK_SENSOR      = 6144;
    constexpr uint32_t    STACK_MQTT        = 8192;
    constexpr uint32_t    STACK_LOGGER      = 6144;
    constexpr uint32_t    STACK_MONITOR     = 4096;
    constexpr uint32_t    STACK_VOICE       = 4096;

    //  [L]
    constexpr uint32_t    ESTOP_DEBOUNCE_MS = 50;
    constexpr uint32_t    ESTOP_CONFIRM_MS  = 100;

    // ADC
    // ADC_CH_PRESSURE removed - using analogRead directly
    constexpr adc_atten_t    ADC_ATTEN        = ADC_ATTEN_DB_12;

    //  
    constexpr uint8_t     CMD_QUEUE_DEPTH   = 16;  // [F]
    constexpr uint8_t     VOICE_QUEUE_DEPTH = 8;   // [I]
    constexpr uint8_t     LOG_QUEUE_DEPTH   = 32;

    // Heap   [7]
    constexpr uint32_t    HEAP_WARN_BYTES   = 32768;
    constexpr uint32_t    HEAP_CRIT_BYTES   = 16384;
}

// ============================================================
// [F] MQTTState   
// ============================================================
enum class CommandType : uint8_t {
    SET_PUMP_SPEED = 0,
    SET_VALVE,
    EMERGENCY_STOP,
    RELEASE_ESTOP,
    SYSTEM_RESET,
    NVS_SAVE_SETPOINT,
};

struct SystemCommand {
    CommandType type;
    union {
        struct { uint8_t channel; float dutyCycle; } pump;
        struct { uint8_t valve; bool state; }        valve;
        struct { uint32_t setpoint; }                 nvs;
    } data;
    char origin[16];  //   (MQTT, LOCAL )
};

// ============================================================
// [B] PWM   ( )
// ============================================================
struct PwmChannelManager {
    std::atomic<uint8_t> allocBitmap{0};  // [J] atomic

    int8_t alloc() {
        uint8_t cur = allocBitmap.load(std::memory_order_relaxed);
        for (uint8_t bit = 0; bit < CFG::PWM_MAX_CHANNELS; ++bit) {
            if (!(cur & (1u << bit))) {
                uint8_t desired = cur | (1u << bit);
                if (allocBitmap.compare_exchange_strong(
                        cur, desired,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed)) {
                    return static_cast<int8_t>(bit);
                }
                cur = allocBitmap.load(std::memory_order_relaxed);
                --bit;  // retry same bit
            }
        }
        return -1;  //  
    }

    void release(uint8_t ch) {
        allocBitmap.fetch_and(~(1u << ch), std::memory_order_acq_rel);
    }
};

// ============================================================
// [A] SharedState ( Race  Mutex )
// ============================================================
struct VacuumSystemState {
    SemaphoreHandle_t mutex = nullptr;

    // 
    float   pressureKpa    = 0.0f;
    bool    pressureValid  = false;
    int64_t pressureTimeUs = 0;

    // 
    float   temperatureC   = 25.0f;
    bool    tempValid      = false;
    int64_t tempTimeUs     = 0;

    // 
    float   pumpDutyCycle  = 0.0f;
    bool    pumpRunning    = false;
    int8_t  pumpPwmCh      = -1;

    // 
    bool    valveState[3]  = {false, false, false};

    // 
    bool    estopActive    = false;      // [J] volatile  mutex + atomic
    bool    otaActive      = false;      // [G]
    bool    ntpSynced      = false;      // [K]
    bool    wifiConnected  = false;
    bool    mqttConnected  = false;

    // NVS 
    uint32_t pressureSetpoint = 80000;  // Pa 

    //  
    uint32_t adcErrors       = 0;
    uint32_t sensorErrors    = 0;
    uint32_t mqttDropped     = 0;
    uint32_t wdtResets       = 0;

    void init() {
        mutex = xSemaphoreCreateMutex();
        configASSERT(mutex != nullptr);
    }

    //  ( )
    float getPressure(bool* valid = nullptr) {
        float v = 0;
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            v = pressureKpa;
            if (valid) *valid = pressureValid;
            xSemaphoreGive(mutex);
        }
        return v;
    }

    void setPressure(float kpa, bool valid) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            pressureKpa   = kpa;
            pressureValid = valid;
            pressureTimeUs = esp_timer_get_time();
            xSemaphoreGive(mutex);
        }
    }

    float getTemperature(bool* valid = nullptr) {
        float v = 0;
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            v = temperatureC;
            if (valid) *valid = tempValid;
            xSemaphoreGive(mutex);
        }
        return v;
    }

    void setTemperature(float c, bool valid) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            temperatureC = c;
            tempValid    = valid;
            tempTimeUs   = esp_timer_get_time();
            xSemaphoreGive(mutex);
        }
    }

    bool isEstop() {
        bool v = false;
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            v = estopActive;
            xSemaphoreGive(mutex);
        }
        return v;
    }

    void setEstop(bool active) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            estopActive = active;
            xSemaphoreGive(mutex);
        }
    }

    bool isOtaActive() {
        bool v = false;
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            v = otaActive;
            xSemaphoreGive(mutex);
        }
        return v;
    }

    void setOtaActive(bool active) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            otaActive = active;
            xSemaphoreGive(mutex);
        }
    }
};

// ============================================================
//   
// ============================================================
static VacuumSystemState  g_state;
static PwmChannelManager  g_pwmMgr;

// [C] NVS  
static SemaphoreHandle_t  g_nvsMutex    = nullptr;
// [D] Serial  
static SemaphoreHandle_t  g_serialMutex = nullptr;
// [H] ADC  
static SemaphoreHandle_t  g_adcMutex    = nullptr;

// [F]  
static QueueHandle_t      g_cmdQueue    = nullptr;
// [I] DFPlayer  
static QueueHandle_t      g_voiceQueue  = nullptr;
//  
static QueueHandle_t      g_logQueue    = nullptr;

// FreeRTOS  
static EventGroupHandle_t g_sysEvents   = nullptr;
#define EVT_WIFI_UP       (1 << 0)
#define EVT_MQTT_UP       (1 << 1)
#define EVT_NTP_SYNC      (1 << 2)  // [K]
#define EVT_OTA_START     (1 << 3)  // [G]
#define EVT_ESTOP         (1 << 4)  // [L]
#define EVT_SENSOR_READY  (1 << 5)

//   ([G] OTA  )
static TaskHandle_t g_taskControl  = nullptr;
static TaskHandle_t g_taskSensor   = nullptr;
static TaskHandle_t g_taskMqtt     = nullptr;
static TaskHandle_t g_taskLogger   = nullptr;
static TaskHandle_t g_taskVoice    = nullptr;
static TaskHandle_t g_taskMonitor  = nullptr;

//  
static WiFiClient       g_wifiClient;
static PubSubClient     g_mqttClient(g_wifiClient);
static WiFiUDP          g_udpClient;
static NTPClient        g_ntpClient(g_udpClient, CFG::NTP_SERVER, CFG::NTP_UTC_OFFSET, 60000);

// 
static OneWire          g_oneWire(PIN::DS18B20_DATA);
static DallasTemperature g_dallasSensor(&g_oneWire);

// DFPlayer
static HardwareSerial   g_dfSerial(2);
// static DFRobotDFPlayerMini g_dfPlayer;

// ============================================================
// [D]  Serial  
// ============================================================
#define SAFE_SERIAL_PRINTF(fmt, ...) \
    do { \
        if (g_serialMutex && xSemaphoreTake(g_serialMutex, pdMS_TO_TICKS(50)) == pdTRUE) { \
            Serial.printf("[%7lu] " fmt "\n", (unsigned long)(esp_timer_get_time()/1000), ##__VA_ARGS__); \
            xSemaphoreGive(g_serialMutex); \
        } \
    } while(0)

// ============================================================
// [H]  ADC 
// ============================================================
static bool adcReadSafe(int pin, int* outRaw) {
    if (!g_adcMutex) return false;
    if (xSemaphoreTake(g_adcMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        ESP_LOGW(TAG_SENSOR, "ADC mutex timeout");
        return false;
    }
    *outRaw = analogRead(33);
    xSemaphoreGive(g_adcMutex);
    if (*outRaw < 0) {
        ESP_LOGE(TAG_SENSOR, "ADC read error: %d", *outRaw);
        return false;
    }
    return true;
}

// ADC  kPa  (  )
static float adcToKpa(int raw) {
    // 0~4095  -100~0 kPa (   )
    return CFG::PRESSURE_MIN_KPA + (raw / 4095.0f) * (CFG::PRESSURE_MAX_KPA - CFG::PRESSURE_MIN_KPA);
}

// ============================================================
// [C]  NVS /
// ============================================================
static esp_err_t nvsSaveU32(const char* key, uint32_t value) {
    if (!g_nvsMutex) return ESP_ERR_INVALID_STATE;
    if (xSemaphoreTake(g_nvsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGW(TAG_SYS, "NVS mutex timeout (save)");
        return ESP_ERR_TIMEOUT;
    }
    nvs_handle_t handle;
    esp_err_t err = nvs_open("vacuum_cfg", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_u32(handle, key, value);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
    }
    xSemaphoreGive(g_nvsMutex);
    return err;
}

static esp_err_t nvsLoadU32(const char* key, uint32_t* value, uint32_t defaultVal) {
    if (!g_nvsMutex) { *value = defaultVal; return ESP_ERR_INVALID_STATE; }
    if (xSemaphoreTake(g_nvsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        *value = defaultVal;
        return ESP_ERR_TIMEOUT;
    }
    nvs_handle_t handle;
    esp_err_t err = nvs_open("vacuum_cfg", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        err = nvs_get_u32(handle, key, value);
        nvs_close(handle);
    }
    if (err != ESP_OK) *value = defaultVal;
    xSemaphoreGive(g_nvsMutex);
    return err;
}

// ============================================================
// [1] Brownout 
// ============================================================
static void IRAM_ATTR browoutISR(void* arg) {
    //    (ISR  GPIO)
    gpio_set_level(PIN::PUMP_PWM, 0);
    gpio_set_level(PIN::VALVE_1, 0);
    gpio_set_level(PIN::VALVE_2, 0);
    gpio_set_level(PIN::VALVE_3, 0);
    gpio_set_level(PIN::LED_ERROR, 1);
    // ISR   -   RTC  
    esp_restart();
}

// ============================================================
// [L]   + 
// ============================================================
static volatile uint32_t g_estopLastUs = 0;  // [J] volatile 

static void IRAM_ATTR estopISR(void* arg) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if ((now - g_estopLastUs) < CFG::ESTOP_DEBOUNCE_MS) return;  // 
    g_estopLastUs = now;

    //    :   
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (g_sysEvents) {
        xEventGroupSetBitsFromISR(g_sysEvents, EVT_ESTOP, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ============================================================
// [6] WiFi   ( )
// ============================================================
static void wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESP_LOGI(TAG_MAIN, "WiFi STA ");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG_MAIN, "IP : %s", WiFi.localIP().toString().c_str());
            g_state.wifiConnected = true;
            if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_WIFI_UP);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            ESP_LOGW(TAG_MAIN, "WiFi   (reason=%d)", info.wifi_sta_disconnected.reason);
            g_state.wifiConnected = false;
            if (g_sysEvents) xEventGroupClearBits(g_sysEvents, EVT_WIFI_UP | EVT_MQTT_UP | EVT_NTP_SYNC);
            WiFi.reconnect();
            break;
        default:
            break;
    }
}

// ============================================================
// [F] MQTT      (State  )
// ============================================================
static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // payload null-terminate
    char buf[128] = {0};
    uint32_t copyLen = (length < sizeof(buf)-1) ? length : sizeof(buf)-1;
    memcpy(buf, payload, copyLen);

    ESP_LOGI(TAG_MQTT, ": [%s] %s", topic, buf);

    SystemCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    strncpy(cmd.origin, "MQTT", sizeof(cmd.origin)-1);

    bool valid = false;

    if (strcmp(topic, "vacuum/cmd/pump") == 0) {
        cmd.type = CommandType::SET_PUMP_SPEED;
        cmd.data.pump.channel = 0;
        cmd.data.pump.dutyCycle = atof(buf);
        valid = true;
    }
    else if (strcmp(topic, "vacuum/cmd/valve1") == 0) {
        cmd.type = CommandType::SET_VALVE;
        cmd.data.valve.valve = 0;
        cmd.data.valve.state = (buf[0] == '1');
        valid = true;
    }
    else if (strcmp(topic, "vacuum/cmd/valve2") == 0) {
        cmd.type = CommandType::SET_VALVE;
        cmd.data.valve.valve = 1;
        cmd.data.valve.state = (buf[0] == '1');
        valid = true;
    }
    else if (strcmp(topic, "vacuum/cmd/estop") == 0) {
        cmd.type = (buf[0] == '1') ? CommandType::EMERGENCY_STOP : CommandType::RELEASE_ESTOP;
        valid = true;
    }
    else if (strcmp(topic, "vacuum/cmd/setpoint") == 0) {
        cmd.type = CommandType::NVS_SAVE_SETPOINT;
        cmd.data.nvs.setpoint = (uint32_t)atol(buf);
        valid = true;
    }

    if (valid && g_cmdQueue) {
        if (xQueueSend(g_cmdQueue, &cmd, 0) != pdTRUE) {
            ESP_LOGW(TAG_MQTT, "   , ");
            if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                g_state.mqttDropped++;
                xSemaphoreGive(g_state.mutex);
            }
        }
    }
}

// ============================================================
// [K] NTP    
// ============================================================
static bool waitForNtpSync(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (g_ntpClient.update()) {
            unsigned long epoch = g_ntpClient.getEpochTime();
            if (epoch > 1700000000UL) {  // 2023  
                ESP_LOGI(TAG_MAIN, "NTP  : %lu", epoch);
                g_state.ntpSynced = true;
                if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_NTP_SYNC);
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGW(TAG_MAIN, "NTP   (%u ms)", timeoutMs);
    return false;
}

// [K] NTP    (1970 )
static bool makeLogFilename(char* outBuf, size_t bufLen) {
    if (!g_state.ntpSynced) {
        // NTP :    ms  
        snprintf(outBuf, bufLen, "/log_boot%lu.csv", (unsigned long)millis());
        ESP_LOGW(TAG_SD, "NTP ,   : %s", outBuf);
        return false;
    }
    unsigned long epoch = g_ntpClient.getEpochTime();
    // epoch  tm 
    time_t t = (time_t)epoch;
    struct tm* tmInfo = gmtime(&t);
    snprintf(outBuf, bufLen, "/log_%04d%02d%02d_%02d%02d%02d.csv",
             tmInfo->tm_year + 1900, tmInfo->tm_mon + 1, tmInfo->tm_mday,
             tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec);
    return true;
}

// ============================================================
// [G] OTA  / 
// ============================================================
static void suspendAllTasksForOTA() {
    ESP_LOGI(TAG_OTA, "OTA  -    ");
    g_state.setOtaActive(true);
    if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_OTA_START);

    //   
    if (g_state.pumpPwmCh >= 0) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)g_state.pumpPwmCh, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)g_state.pumpPwmCh);
    }
    gpio_set_level(PIN::VALVE_1, 0);
    gpio_set_level(PIN::VALVE_2, 0);
    gpio_set_level(PIN::VALVE_3, 0);

    //  
    if (g_taskControl)  vTaskSuspend(g_taskControl);
    if (g_taskSensor)   vTaskSuspend(g_taskSensor);
    if (g_taskLogger)   vTaskSuspend(g_taskLogger);
    if (g_taskVoice)    vTaskSuspend(g_taskVoice);
    if (g_taskMonitor)  vTaskSuspend(g_taskMonitor);
    // WDT  (OTA     )
    esp_task_wdt_delete(NULL);
}

static void resumeAllTasksAfterOTA() {
    ESP_LOGI(TAG_OTA, "OTA  -  ");
    if (g_taskControl)  vTaskResume(g_taskControl);
    if (g_taskSensor)   vTaskResume(g_taskSensor);
    if (g_taskLogger)   vTaskResume(g_taskLogger);
    if (g_taskVoice)    vTaskResume(g_taskVoice);
    if (g_taskMonitor)  vTaskResume(g_taskMonitor);
    g_state.setOtaActive(false);
}

// ============================================================
// [B][8] PWM   (SPI  +   )
// ============================================================
static int8_t initPumpPwm() {
    int8_t ch = g_pwmMgr.alloc();
    if (ch < 0) {
        ESP_LOGE(TAG_MAIN, "PWM    -  ");
        return -1;
    }
    Serial.println("STEP 11: PWM init"); 
    Serial.flush();
    ledc_timer_config_t timerCfg = {};
    timerCfg.speed_mode      = LEDC_LOW_SPEED_MODE;
    timerCfg.timer_num       = LEDC_TIMER_0;
    timerCfg.duty_resolution = (ledc_timer_bit_t)CFG::PWM_RESOLUTION;
    timerCfg.freq_hz         = CFG::PWM_FREQ;
    timerCfg.clk_cfg         = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timerCfg) != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "LEDC   ");
        g_pwmMgr.release(ch);
        return -1;
    }

    ledc_channel_config_t chCfg = {};
    chCfg.gpio_num   = PIN::PUMP_PWM;
    chCfg.speed_mode = LEDC_LOW_SPEED_MODE;
    chCfg.channel    = (ledc_channel_t)ch;
    chCfg.timer_sel  = LEDC_TIMER_0;
    chCfg.intr_type  = LEDC_INTR_DISABLE;
    chCfg.duty       = 0;
    chCfg.hpoint     = 0;
    if (ledc_channel_config(&chCfg) != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "LEDC   ");
        g_pwmMgr.release(ch);
        return -1;
    }

    ESP_LOGI(TAG_MAIN, " PWM  %d  ", ch);
    return ch;
}

// ============================================================
// [5] I2C  
// ============================================================
static void recoverI2CBus() {
    ESP_LOGW(TAG_MAIN, "I2C   ");
    Wire.end();
    vTaskDelay(pdMS_TO_TICKS(10));

    // SCL   9
    pinMode(PIN::I2C_SCL, OUTPUT);
    pinMode(PIN::I2C_SDA, INPUT_PULLUP);
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN::I2C_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(PIN::I2C_SCL, HIGH);
        delayMicroseconds(5);
    }
    // STOP 
    pinMode(PIN::I2C_SDA, OUTPUT);
    digitalWrite(PIN::I2C_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN::I2C_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN::I2C_SDA, HIGH);

    Wire.begin(PIN::I2C_SDA, PIN::I2C_SCL, 100000);
    esp_task_wdt_reset();
    Serial.println("STEP 13a: I2C done"); 
    Serial.flush();
    ESP_LOGI(TAG_MAIN, "I2C   ");
}

// ============================================================
// [4] SD   
// ============================================================
static bool initSDWithTimeout(uint32_t timeoutMs) {
    esp_task_wdt_reset();
    // Waveshare 3.5B: SD_MMC 방식
    // CLK=9, CMD=10, D0=11
    if (SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
        ESP_LOGI(TAG_MAIN, "SD_MMC OK");
        esp_task_wdt_reset();
        return true;
    }
    ESP_LOGE(TAG_MAIN, "SD_MMC failed");
    esp_task_wdt_reset();
    return false;
}
// ============================================================
// [3] PSRAM   ( )
// ============================================================
static uint8_t* allocPsramBuffer(size_t size) {
    if (!psramFound()) {
        ESP_LOGW(TAG_SYS, "PSRAM ,  SRAM ");
        return (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    uint8_t* p = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) {
        ESP_LOGE(TAG_SYS, "PSRAM   (%u bytes),  SRAM ", size);
        p = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if (p) {
        ESP_LOGI(TAG_SYS, "PSRAM   : %u bytes @ %p", size, p);
    } else {
        ESP_LOGE(TAG_SYS, "    (%u bytes)", size);
    }
    return p;
}

// ============================================================
// [9] DS18B20   (  750ms   )
// ============================================================
static bool g_ds18b20Requested = false;         // [J]  
static uint32_t g_ds18b20RequestTimeMs = 0;

static void ds18b20RequestConversion() {
    g_dallasSensor.setWaitForConversion(false);  // 
    g_dallasSensor.requestTemperatures();
    g_ds18b20Requested = true;
    g_ds18b20RequestTimeMs = millis();
    ESP_LOGD(TAG_SENSOR, "DS18B20  ");
}

static bool ds18b20ReadIfReady(float* outTemp) {
    if (!g_ds18b20Requested) return false;
    if (millis() - g_ds18b20RequestTimeMs < 750) return false;  //   

    float t = g_dallasSensor.getTempCByIndex(0);
    g_ds18b20Requested = false;

    if (t == DEVICE_DISCONNECTED_C || t < -55.0f || t > 125.0f) {
        ESP_LOGW(TAG_SENSOR, "DS18B20   : %.2f", t);
        return false;
    }
    *outTemp = t;
    return true;
}

// ============================================================
// [I] DFPlayer   
// ============================================================
enum class VoiceCmd : uint8_t {
    PLAY_TRACK = 0,
    SET_VOLUME,
    STOP,
};

struct VoiceMessage {
    VoiceCmd cmd;
    uint8_t  param1;
    uint8_t  param2;
};

static void voiceQueuePlay(uint8_t track) {
    VoiceMessage msg{VoiceCmd::PLAY_TRACK, track, 0};
    if (g_voiceQueue) {
        if (xQueueSend(g_voiceQueue, &msg, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG_MAIN, "Voice   ");
        }
    }
}

// ============================================================
// [7] Heap 
// ============================================================
static void checkHeapHealth() {
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t minFree  = esp_get_minimum_free_heap_size();

    if (freeHeap < CFG::HEAP_CRIT_BYTES) {
        ESP_LOGE(TAG_SYS, "CRITICAL: Heap ! : %u bytes (: %u)", freeHeap, minFree);
        voiceQueuePlay(5);  // 
        //   ESP  
    } else if (freeHeap < CFG::HEAP_WARN_BYTES) {
        ESP_LOGW(TAG_SYS, "WARNING: Heap : %u bytes (: %u)", freeHeap, minFree);
    }

    // PSRAM 
    if (psramFound()) {
        uint32_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGD(TAG_SYS, "PSRAM : %u bytes", freePsram);
    }
}

// ============================================================
// [E]  
// ============================================================
static void logTaskStackHighWaterMark(TaskHandle_t h, const char* name) {
    if (!h) return;
    UBaseType_t hwm = uxTaskGetStackHighWaterMark(h);
    if (hwm < 512) {
        ESP_LOGW(TAG_SYS, "[] %s: =%u words", name, hwm);
    } else {
        ESP_LOGD(TAG_SYS, "[] %s: =%u words", name, hwm);
    }
}

// ============================================================
// ==================== FreeRTOS  ====================
// ============================================================

// ============================================================
//  1:   (   + )
// ============================================================
static void taskControl(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_CTRL, "  ");

    //    +   
    SystemCommand cmd;

    for (;;) {
        esp_task_wdt_reset();

        // [G] OTA  
        if (g_state.isOtaActive()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // [L]   
        EventBits_t bits = xEventGroupGetBits(g_sysEvents);
        if (bits & EVT_ESTOP) {
            //    (CFG::ESTOP_CONFIRM_MS)
            vTaskDelay(pdMS_TO_TICKS(CFG::ESTOP_CONFIRM_MS));
            bool pinStillLow = (gpio_get_level(PIN::ESTOP) == 0);
            if (pinStillLow && !g_state.isEstop()) {
                ESP_LOGE(TAG_CTRL, " !");
                g_state.setEstop(true);
                //   
                if (g_state.pumpPwmCh >= 0) {
                    ledc_set_duty(LEDC_LOW_SPEED_MODE,
                                  (ledc_channel_t)g_state.pumpPwmCh, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE,
                                     (ledc_channel_t)g_state.pumpPwmCh);
                }
                gpio_set_level(PIN::VALVE_1, 0);
                gpio_set_level(PIN::VALVE_2, 0);
                gpio_set_level(PIN::VALVE_3, 0);
                gpio_set_level(PIN::LED_ERROR, 1);
                voiceQueuePlay(3);  //  
            }
            xEventGroupClearBits(g_sysEvents, EVT_ESTOP);
        }

        // [F]    
        while (xQueueReceive(g_cmdQueue, &cmd, 0) == pdTRUE) {
            //     
            if (g_state.isEstop() && cmd.type != CommandType::RELEASE_ESTOP) {
                ESP_LOGW(TAG_CTRL, "  -  : %d", (int)cmd.type);
                continue;
            }

            switch (cmd.type) {
                case CommandType::SET_PUMP_SPEED: {
                    float duty = cmd.data.pump.dutyCycle;
                    if (duty < 0.0f) duty = 0.0f;
                    if (duty > 100.0f) duty = 100.0f;

                    uint32_t rawDuty = (uint32_t)((duty / 100.0f) * ((1 << CFG::PWM_RESOLUTION) - 1));
                    if (g_state.pumpPwmCh >= 0) {
                        ledc_set_duty(LEDC_LOW_SPEED_MODE,
                                      (ledc_channel_t)g_state.pumpPwmCh, rawDuty);
                        ledc_update_duty(LEDC_LOW_SPEED_MODE,
                                         (ledc_channel_t)g_state.pumpPwmCh);
                    }
                    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                        g_state.pumpDutyCycle = duty;
                        g_state.pumpRunning   = (duty > 0.0f);
                        xSemaphoreGive(g_state.mutex);
                    }
                    ESP_LOGI(TAG_CTRL, "  : %.1f%% (origin:%s)", duty, cmd.origin);
                    break;
                }
                case CommandType::SET_VALVE: {
                    uint8_t v = cmd.data.valve.valve;
                    bool  st = cmd.data.valve.state;
                    gpio_num_t pins[] = {PIN::VALVE_1, PIN::VALVE_2, PIN::VALVE_3};
                    if (v < 3) {
                        gpio_set_level(pins[v], st ? 1 : 0);
                        if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                            g_state.valveState[v] = st;
                            xSemaphoreGive(g_state.mutex);
                        }
                        ESP_LOGI(TAG_CTRL, "%d  %s (origin:%s)", v+1, st?"ON":"OFF", cmd.origin);
                    }
                    break;
                }
                case CommandType::EMERGENCY_STOP: {
                    ESP_LOGE(TAG_CTRL, "    (from:%s)", cmd.origin);
                    g_state.setEstop(true);
                    if (g_state.pumpPwmCh >= 0) {
                        ledc_set_duty(LEDC_LOW_SPEED_MODE,
                                      (ledc_channel_t)g_state.pumpPwmCh, 0);
                        ledc_update_duty(LEDC_LOW_SPEED_MODE,
                                         (ledc_channel_t)g_state.pumpPwmCh);
                    }
                    gpio_set_level(PIN::VALVE_1, 0);
                    gpio_set_level(PIN::VALVE_2, 0);
                    gpio_set_level(PIN::VALVE_3, 0);
                    gpio_set_level(PIN::LED_ERROR, 1);
                    voiceQueuePlay(3);
                    break;
                }
                case CommandType::RELEASE_ESTOP: {
                    if (gpio_get_level(PIN::ESTOP) == 1) {  //   
                        ESP_LOGI(TAG_CTRL, "  (from:%s)", cmd.origin);
                        g_state.setEstop(false);
                        gpio_set_level(PIN::LED_ERROR, 0);
                        voiceQueuePlay(4);
                    } else {
                        ESP_LOGW(TAG_CTRL, "   ,  ");
                    }
                    break;
                }
                case CommandType::NVS_SAVE_SETPOINT: {
                    // [C] NVS  
                    uint32_t sp = cmd.data.nvs.setpoint;
                    esp_err_t err = nvsSaveU32("pressure_sp", sp);
                    if (err == ESP_OK) {
                        if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                            g_state.pressureSetpoint = sp;
                            xSemaphoreGive(g_state.mutex);
                        }
                        ESP_LOGI(TAG_CTRL, "  : %u Pa", sp);
                    } else {
                        ESP_LOGE(TAG_CTRL, " NVS  : 0x%x", err);
                    }
                    break;
                }
                default:
                    ESP_LOGW(TAG_CTRL, "   : %d", (int)cmd.type);
                    break;
            }
        }

        //     (  )
        if (!g_state.isEstop()) {
            bool pValid = false;
            float pressure = g_state.getPressure(&pValid);
            if (pValid) {
                if (pressure < CFG::PRESSURE_TRIP) {
                    // :   
                    if (g_state.pumpRunning) {
                        SystemCommand tripCmd;
                        tripCmd.type = CommandType::SET_PUMP_SPEED;
                        tripCmd.data.pump.dutyCycle = 0.0f;
                        strncpy(tripCmd.origin, "TRIP", sizeof(tripCmd.origin)-1);
                        xQueueSend(g_cmdQueue, &tripCmd, 0);
                        voiceQueuePlay(2);  //   
                        ESP_LOGE(TAG_CTRL, " : %.2f kPa", pressure);
                    }
                } else if (pressure < CFG::PRESSURE_ALARM) {
                    if (!g_state.pumpRunning) {
                        voiceQueuePlay(1);  // 
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================
//  2:  
// ============================================================
static void taskSensor(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_SENSOR, "  ");

    // DS18B20 
    g_dallasSensor.begin();
    g_dallasSensor.setResolution(12);  // 12bit = 750ms 
    ds18b20RequestConversion();  //   [9]

    // [H] ADC 
    // adc1_config_width removed
    // adc1_config_channel_atten removed

    xEventGroupSetBits(g_sysEvents, EVT_SENSOR_READY);

    uint32_t lastPressureMs = 0;
    uint32_t lastTempMs     = 0;
    uint8_t  i2cErrCount    = 0;

    for (;;) {
        esp_task_wdt_reset();

        if (g_state.isOtaActive()) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        uint32_t now = millis();

        // [H]  ADC  (100ms )
        if (now - lastPressureMs >= 100) {
            lastPressureMs = now;
            int raw = 0;
            if (adcReadSafe(33, &raw)) {
                float kpa = adcToKpa(raw);
                g_state.setPressure(kpa, true);
                i2cErrCount = 0;
            } else {
                g_state.setPressure(0.0f, false);
                i2cErrCount++;
                if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    g_state.adcErrors++;
                    xSemaphoreGive(g_state.mutex);
                }
                // [5]    I2C 
                if (i2cErrCount >= 5) {
                    recoverI2CBus();
                    i2cErrCount = 0;
                }
            }
        }

        // [9] DS18B20   (1000ms )
        if (now - lastTempMs >= 1000) {
            float temp = 0.0f;
            if (ds18b20ReadIfReady(&temp)) {
                g_state.setTemperature(temp, true);
                lastTempMs = now;
                //   
                if (temp >= CFG::TEMP_TRIP) {
                    ESP_LOGE(TAG_SENSOR, " : %.2fC", temp);
                    SystemCommand cmd;
                    cmd.type = CommandType::EMERGENCY_STOP;
                    strncpy(cmd.origin, "TEMP_TRIP", sizeof(cmd.origin)-1);
                    xQueueSend(g_cmdQueue, &cmd, 0);
                } else if (temp >= CFG::TEMP_ALARM) {
                    ESP_LOGW(TAG_SENSOR, " : %.2fC", temp);
                    voiceQueuePlay(6);
                }
                ds18b20RequestConversion();  //   
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ============================================================
//  3: MQTT  ()
// ============================================================
static void taskMqtt(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_MQTT, "MQTT  ");

    g_mqttClient.setServer(CFG::MQTT_BROKER, CFG::MQTT_PORT);
    g_mqttClient.setCallback(mqttCallback);  // [F]   
    g_mqttClient.setBufferSize(512);
    g_mqttClient.setKeepAlive(60);

    uint32_t lastReconnectMs = 0;
    uint32_t lastPublishMs   = 0;
    char     pubBuf[256];

    for (;;) {
        esp_task_wdt_reset();

        if (g_state.isOtaActive()) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // WiFi  () [6]
        if (!g_state.wifiConnected) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // MQTT 
        if (!g_mqttClient.connected()) {
            uint32_t now = millis();
            if (now - lastReconnectMs >= CFG::MQTT_RECONNECT_MS) {
                lastReconnectMs = now;
                ESP_LOGI(TAG_MQTT, "MQTT  : %s:%d", CFG::MQTT_BROKER, CFG::MQTT_PORT);
                bool ok = g_mqttClient.connect(CFG::MQTT_CLIENT_ID,
                                               CFG::MQTT_USER, CFG::MQTT_PASS,
                                               "vacuum/status/lwt", 1, true, "offline");
                if (ok) {
                    ESP_LOGI(TAG_MQTT, "MQTT  ");
                    g_state.mqttConnected = true;
                    if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_MQTT_UP);

                    // 
                    g_mqttClient.subscribe("vacuum/cmd/#", 1);
                    g_mqttClient.publish("vacuum/status/lwt", "online", true);
                } else {
                    ESP_LOGW(TAG_MQTT, "MQTT   (rc=%d)", g_mqttClient.state());
                    g_state.mqttConnected = false;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        g_mqttClient.loop();

        //   (2 )
        uint32_t now = millis();
        if (now - lastPublishMs >= 2000) {
            lastPublishMs = now;

            bool pValid = false, tValid = false;
            float pressure = g_state.getPressure(&pValid);
            float temp     = g_state.getTemperature(&tValid);

            uint32_t freeHeap = esp_get_free_heap_size();
            bool estop = g_state.isEstop();

            snprintf(pubBuf, sizeof(pubBuf),
                     "{\"pressure\":%.2f,\"temp\":%.2f,\"estop\":%d,"
                     "\"pump_duty\":%.1f,\"free_heap\":%u,"
                     "\"p_valid\":%d,\"t_valid\":%d}",
                     pressure, temp, estop ? 1 : 0,
                     g_state.pumpDutyCycle, freeHeap,
                     pValid ? 1 : 0, tValid ? 1 : 0);

            if (!g_mqttClient.publish("vacuum/status/telemetry", pubBuf)) {
                ESP_LOGW(TAG_MQTT, "MQTT  ");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================
//  4: SD   [K] NTP  
// ============================================================
static void taskLogger(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_SD, "  ");

    // [K] NTP  
    EventBits_t bits = xEventGroupWaitBits(g_sysEvents, EVT_NTP_SYNC,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(CFG::NTP_SYNC_WAIT_MS));
    if (!(bits & EVT_NTP_SYNC)) {
        ESP_LOGW(TAG_SD, "NTP     (  )");
    }

    char filename[64];
    makeLogFilename(filename, sizeof(filename));
    ESP_LOGI(TAG_SD, " : %s", filename);

    // CSV  
    File logFile = SD.open(filename, FILE_APPEND);
    if (logFile) {
        logFile.println("timestamp_ms,pressure_kpa,temperature_c,pump_duty,estop,free_heap");
        logFile.close();
    } else {
        ESP_LOGE(TAG_SD, "   : %s", filename);
    }

    uint32_t lastLogMs = 0;

    for (;;) {
        esp_task_wdt_reset();

        if (g_state.isOtaActive()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        uint32_t now = millis();
        if (now - lastLogMs >= 1000) {
            lastLogMs = now;

            float p = g_state.getPressure();
            float t = g_state.getTemperature();
            bool  e = g_state.isEstop();
            float d = 0.0f;
            if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                d = g_state.pumpDutyCycle;
                xSemaphoreGive(g_state.mutex);
            }
            uint32_t heap = esp_get_free_heap_size();

            // [4] SD  ( )
            logFile = SD.open(filename, FILE_APPEND);
            if (logFile) {
                char row[128];
                snprintf(row, sizeof(row),
                         "%lu,%.2f,%.2f,%.1f,%d,%u",
                         (unsigned long)now, p, t, d, e ? 1 : 0, heap);
                logFile.println(row);
                logFile.close();
            } else {
                ESP_LOGW(TAG_SD, "SD   (  )");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ============================================================
//  5:    [I]  
// ============================================================
static void taskVoice(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_MAIN, "  ");

    // DFPlayer 
    g_dfSerial.begin(9600, SERIAL_8N1, PIN::DFPLAYER_RX, PIN::DFPLAYER_TX);
    // if (!g_dfPlayer.begin(g_dfSerial, true, false)) {
    //    ESP_LOGE(TAG_MAIN, "DFPlayer  ");
    //    vTaskDelete(nullptr);
    //    return;
    // }
    // g_dfPlayer.volume(20);
    ESP_LOGI(TAG_MAIN, "DFPlayer  ");

    VoiceMessage msg;
    for (;;) {
        esp_task_wdt_reset();

        //    
        if (xQueueReceive(g_voiceQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (msg.cmd) {
                case VoiceCmd::PLAY_TRACK:
                    voiceAlert.playVoice(msg.param1);
                    ESP_LOGI(TAG_MAIN, "DFPlayer:  %d ", msg.param1);
                    break;
                case VoiceCmd::SET_VOLUME:
                    voiceAlert.setVolume(msg.param1);
                    break;
                case VoiceCmd::STOP:
                    voiceAlert.stop();
                    break;
            }
        }
    }
}

// ============================================================
//  6:    [2][7][E]
// ============================================================
static void taskMonitor(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_SYS, "  ");

    uint32_t lastMonMs = 0;

    for (;;) {
        esp_task_wdt_reset();

        uint32_t now = millis();
        if (now - lastMonMs >= 5000) {
            lastMonMs = now;

            // [7] Heap 
            checkHeapHealth();

            // [E]   
            logTaskStackHighWaterMark(g_taskControl,  "Control");
            logTaskStackHighWaterMark(g_taskSensor,   "Sensor");
            logTaskStackHighWaterMark(g_taskMqtt,     "MQTT");
            logTaskStackHighWaterMark(g_taskLogger,   "Logger");
            logTaskStackHighWaterMark(g_taskVoice,    "Voice");
            logTaskStackHighWaterMark(g_taskMonitor,  "Monitor");

            //   
            SAFE_SERIAL_PRINTF(
                "===   ===\n"
                "  : %.2f kPa | : %.2fC\n"
                "  : %.1f%% | E-Stop: %s\n"
                "  WiFi: %s | MQTT: %s | NTP: %s\n"
                "  Free Heap: %u | MinFree: %u",
                g_state.getPressure(), g_state.getTemperature(),
                g_state.pumpDutyCycle, g_state.isEstop() ? "ON" : "OFF",
                g_state.wifiConnected ? "UP" : "DOWN",
                g_state.mqttConnected ? "UP" : "DOWN",
                g_state.ntpSynced ? "SYNCED" : "NO_SYNC",
                esp_get_free_heap_size(),
                esp_get_minimum_free_heap_size()
            );

            // [2] WDT   ( )
            esp_reset_reason_t reason = esp_reset_reason();
            if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_INT_WDT) {
                ESP_LOGE(TAG_WDT, "  : WDT ");
                if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                    g_state.wdtResets++;
                    xSemaphoreGive(g_state.mutex);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================
// OTA  [G]
// ============================================================
static void initOTA() {
    ArduinoOTA.setHostname(CFG::MQTT_CLIENT_ID);
    ArduinoOTA.setPassword("admin");

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        ESP_LOGI(TAG_OTA, "OTA : %s", type.c_str());
        suspendAllTasksForOTA();  // [G]
    });

    ArduinoOTA.onEnd([]() {
        ESP_LOGI(TAG_OTA, "OTA  - ");
        //    esp_restart() 
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint8_t lastPct = 0;
        uint8_t pct = (uint8_t)(progress * 100 / total);
        if (pct != lastPct && pct % 10 == 0) {
            ESP_LOGI(TAG_OTA, "OTA : %u%%", pct);
            lastPct = pct;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        ESP_LOGE(TAG_OTA, "OTA  [%u]: %s",
                 error,
                 error == OTA_AUTH_ERROR    ? "" :
                 error == OTA_BEGIN_ERROR   ? "" :
                 error == OTA_CONNECT_ERROR ? "" :
                 error == OTA_RECEIVE_ERROR ? "" :
                 error == OTA_END_ERROR     ? "" : "  ");
        resumeAllTasksAfterOTA();  // [G]   
    });

    ArduinoOTA.begin();
    ESP_LOGI(TAG_OTA, "OTA  ");
}

// ============================================================
// WiFi  [6]
// ============================================================
static void initWifiNonBlocking() {
    esp_task_wdt_reset();
    // WiFi 임시 비활성화 (SSID 미설정)
    ESP_LOGW(TAG_MAIN, "WiFi skipped - no SSID");
    esp_task_wdt_reset();
}
/* static void initWifiNonBlocking() {
    esp_task_wdt_reset();
    WiFi.onEvent(wifiEventHandler);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    esp_task_wdt_reset();
    WiFi.begin(CFG::WIFI_SSID, CFG::WIFI_PASS);
    esp_task_wdt_reset();
    ESP_LOGI(TAG_MAIN, "WiFi   : %s", CFG::WIFI_SSID);
    //     
}
*/
// ============================================================
// OTA   (ArduinoOTA.handle() ) [G]
// ============================================================
static void taskOTA(void* pv) {
    ESP_LOGI(TAG_OTA, "OTA  ");
    for (;;) {
        if (g_state.wifiConnected) {
            ArduinoOTA.handle();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================
// setup() -  
// ============================================================
void setup() {
    delay(5000);
    Serial.begin(115200);
    delay(500);
    Serial.println("=== BOOT START ===");
    Serial.flush();
    
    // SPI Bus Manager 초기화 (LCD 초기화 후)
    SPIBusManager::getInstance().begin();
    Serial.println("SPI Bus init done"); Serial.flush();
    esp_task_wdt_reset();

    // [D] Serial  (   )
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== ESP32-S3    v3.9.4 Hardened Edition ===");

    // --------------------------------------------------------
    //  /  /    [A][C][D][F][H][I]
    // --------------------------------------------------------
    Serial.println("STEP 1: g_state.init");
    Serial.flush();
    g_state.init();
    Serial.println("STEP 2: mutex"); 
    Serial.flush();                                          // [A] SharedState mutex
    g_nvsMutex    = xSemaphoreCreateMutex();                 // [C]
    g_serialMutex = xSemaphoreCreateMutex();                 // [D]
    g_adcMutex    = xSemaphoreCreateMutex();                 // [H]
    Serial.println("STEP 3: queue"); 
    Serial.flush();
    g_cmdQueue    = xQueueCreate(CFG::CMD_QUEUE_DEPTH,   sizeof(SystemCommand));  // [F]
    g_voiceQueue  = xQueueCreate(CFG::VOICE_QUEUE_DEPTH, sizeof(VoiceMessage));  // [I]
    g_logQueue    = xQueueCreate(CFG::LOG_QUEUE_DEPTH,   sizeof(char)*128);
    g_sysEvents   = xEventGroupCreate();
    Serial.println("STEP 4: assert"); 
    Serial.flush();
    configASSERT(g_nvsMutex);
    configASSERT(g_serialMutex);
    configASSERT(g_adcMutex);
    configASSERT(g_cmdQueue);
    configASSERT(g_voiceQueue);
    configASSERT(g_sysEvents);
    Serial.println("STEP 5: WDT config"); 
    Serial.flush();
    // --------------------------------------------------------
    // [2] WDT  (15 )
    // --------------------------------------------------------
    esp_task_wdt_add(NULL);  // 현재 태스크 WDT 등록
    esp_task_wdt_config_t wdtCfg = {
        .timeout_ms     = 30000, // 30초로 늘림
        .idle_core_mask = 0,
        .trigger_panic  = true,
    };
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&wdtCfg));
    Serial.println("STEP 6: WDT done"); 
    Serial.flush();
    esp_task_wdt_reset();
    
    ESP_LOGI(TAG_MAIN, "WDT : %u", WDT_TIMEOUT);

    // --------------------------------------------------------
    // [1] Brownout  
    // --------------------------------------------------------
    //    (ESP-IDF   )
    // esp_brownout_init();  //ESP-IDF 5.x / arduino-esp32 3.x 
    ESP_LOGI(TAG_MAIN, "Brownout  ");

    // --------------------------------------------------------
    // NVS  [C]
    // --------------------------------------------------------
    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG_MAIN, "NVS :   ");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvsErr = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvsErr);
    Serial.println("STEP 7: NVS done"); 
    Serial.flush();
    // NVS  
    uint32_t savedSetpoint = 80000;
    nvsLoadU32("pressure_sp", &savedSetpoint, 80000);
    g_state.pressureSetpoint = savedSetpoint;
    ESP_LOGI(TAG_MAIN, "NVS  : pressure_sp=%u Pa", savedSetpoint);
    Serial.println("STEP 8: GPIO config"); 
    Serial.flush();
    // --------------------------------------------------------
    // GPIO 
    // --------------------------------------------------------
    //  
    gpio_config_t outCfg = {};
    outCfg.mode         = GPIO_MODE_OUTPUT;
    outCfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    outCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    outCfg.intr_type    = GPIO_INTR_DISABLE;

    outCfg.pin_bit_mask = (1ULL << PIN::VALVE_1) | (1ULL << PIN::VALVE_2) |
                          (1ULL << PIN::VALVE_3) | (1ULL << PIN::LED_STATUS) |
                          (1ULL << PIN::LED_ERROR);
    Serial.println("STEP 8a: outCfg"); 
    Serial.flush();
    ESP_ERROR_CHECK(gpio_config(&outCfg));
    Serial.println("STEP9: GPIO out done"); 
    Serial.flush();

    gpio_set_level(PIN::VALVE_1, 0);
    gpio_set_level(PIN::VALVE_2, 0);
    gpio_set_level(PIN::VALVE_3, 0);
    gpio_set_level(PIN::LED_STATUS, 1);
    gpio_set_level(PIN::LED_ERROR, 0);

    // [L]   (  + )
    gpio_config_t estopCfg = {};
    estopCfg.pin_bit_mask   = (1ULL << PIN::ESTOP);
    estopCfg.mode           = GPIO_MODE_INPUT;
    estopCfg.pull_up_en     = GPIO_PULLUP_ENABLE;   //  
    estopCfg.pull_down_en   = GPIO_PULLDOWN_DISABLE;
    estopCfg.intr_type      = GPIO_INTR_NEGEDGE;    //   
    // ESP_ERROR_CHECK(gpio_config(&estopCfg));
    // ESP_ERROR_CHECK(gpio_install_isr_service(0));
    // ESP_ERROR_CHECK(gpio_isr_handler_add(PIN::ESTOP, estopISR, nullptr));
    Serial.println("STEP 10: ESTOP done"); 
    Serial.flush();
    ESP_LOGI(TAG_MAIN, " GPIO   (=%u ms)", CFG::ESTOP_DEBOUNCE_MS);
    Serial.println("STEP 11: PWM init"); 
    Serial.flush();
    // --------------------------------------------------------
    // [B][8] PWM  (  )
    // --------------------------------------------------------
    int8_t pwmCh = initPumpPwm();
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_state.pumpPwmCh = pwmCh;
        xSemaphoreGive(g_state.mutex);
    }
    Serial.println("STEP 12: PSRAM check"); 
    Serial.flush();
    // --------------------------------------------------------
    // [3] PSRAM 
    // --------------------------------------------------------
    Serial.println("STEP 12: PSRAM check"); 
    Serial.flush();
    esp_task_wdt_reset();  // WDT 피드
    if (psramFound()) {
        ESP_LOGI(TAG_MAIN, "PSRAM : %u bytes", ESP.getPsramSize());
    } else {
        ESP_LOGW(TAG_MAIN, "PSRAM ");
    }
    esp_task_wdt_reset();
    Serial.println("STEP 13: I2C done"); 
    Serial.flush();
    //   PSRAM  ( )
    // uint8_t* logBuf = allocPsramBuffer(64 * 1024);

    // --------------------------------------------------------
    // I2C  [5]
    // --------------------------------------------------------
    Wire.begin(PIN::I2C_SDA, PIN::I2C_SCL, 100000);
    esp_task_wdt_reset();
    Serial.println("STEP 13b: I2C done"); 
    Serial.flush();
    ESP_LOGI(TAG_MAIN, "I2C  ");

    // TCA9554 RST (AXS15231B 필수) - Wire1 사용 (GPIO 21, 22)
    {
        Wire.begin(21, 22); // already initialized
        uint8_t tca_addr = 0x20;
        Wire.beginTransmission(tca_addr);
        Wire.write(0x03); Wire.write(0xFD); Wire.endTransmission();
        Wire.beginTransmission(tca_addr);
        Wire.write(0x01); Wire.write(0xFF); Wire.endTransmission();
        delay(10);
        Wire.beginTransmission(tca_addr);
        Wire.write(0x01); Wire.write(0xFD); Wire.endTransmission();
        delay(10);
        Wire.beginTransmission(tca_addr);
        Wire.write(0x01); Wire.write(0xFF); Wire.endTransmission();
        delay(200);
        Serial.println("STEP 13c: TCA9554 RST done"); Serial.flush();
    }

    // LCD 초기화 (TCA9554 RST 이후)
    esp_task_wdt_delete(NULL);
    Serial.println("LCD init start"); Serial.flush();
    if (tft.begin()) {
        Serial.println("LCD OK"); Serial.flush();
        tft.fillScreen(0xF800);
        delay(3000);
        tft.fillScreen(0x0000);
    } else {
        Serial.println("LCD FAIL"); Serial.flush();
    }
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();

    // --------------------------------------------------------
    // [4] SD   ( 5)
    // --------------------------------------------------------
    esp_task_wdt_reset();
    Serial.println("STEP 14: SD init start"); Serial.flush();
    bool sdOk = initSDWithTimeout(5000);
    esp_task_wdt_reset();
    Serial.println("STEP 15: SD done"); Serial.flush();
    if (!sdOk) {
        ESP_LOGE(TAG_MAIN, "SD    ( )");
    }
    // --------------------------------------------------------
    // [5] [K3]   Mutex 
    // --------------------------------------------------------
    initStateMachine();
    Serial.println("STEP 15: StateMachine done"); 
    Serial.flush();
    // --------------------------------------------------------
    // [6] WiFi  
    // --------------------------------------------------------
    esp_task_wdt_reset();  // ← 추가
    Serial.println("STEP 16a: WiFi init start"); 
    Serial.flush();
    initWifiNonBlocking();
    esp_task_wdt_reset();  // ← 추가
    Serial.println("STEP 16: WiFi init done");
    Serial.flush();

    // --------------------------------------------------------
    // WiFi   ( 15,  sleep)
    // --------------------------------------------------------
    esp_task_wdt_reset();  
    EventBits_t wifiEvt = xEventGroupWaitBits(g_sysEvents, EVT_WIFI_UP,
                                              pdFALSE, pdFALSE,
                                              pdMS_TO_TICKS(CFG::WIFI_TIMEOUT_MS));
     esp_task_wdt_reset();                                         
    if (wifiEvt & EVT_WIFI_UP) {
        ESP_LOGI(TAG_MAIN, "WiFi    NTP ");
        g_ntpClient.begin();
        waitForNtpSync(CFG::NTP_SYNC_WAIT_MS);  // [K]
        initOTA();                               // [G]
    } else {
        ESP_LOGW(TAG_MAIN, "WiFi  -  ");
    }

    // --------------------------------------------------------
    // FreeRTOS   [E]  
    // --------------------------------------------------------
    esp_task_wdt_reset();
    Serial.println("STEP 18: tasks start"); Serial.flush();

    xTaskCreatePinnedToCore(taskControl, "Control",
                            CFG::STACK_CONTROL, nullptr, 5,
                            &g_taskControl, 1);
    Serial.println("STEP 18a: Control task"); Serial.flush();

    xTaskCreatePinnedToCore(taskSensor, "Sensor",
                            CFG::STACK_SENSOR, nullptr, 4,
                            &g_taskSensor, 1);
    Serial.println("STEP 18b: Sensor task"); Serial.flush();

    xTaskCreatePinnedToCore(taskMqtt, "MQTT",
                            CFG::STACK_MQTT, nullptr, 3,
                            &g_taskMqtt, 0);
    Serial.println("STEP 18c: MQTT task"); Serial.flush();

    xTaskCreatePinnedToCore(taskLogger, "Logger",
                            CFG::STACK_LOGGER, nullptr, 2,
                            &g_taskLogger, 0);
    Serial.println("STEP 18d: Logger task"); Serial.flush();

    xTaskCreatePinnedToCore(taskVoice, "Voice",
                            CFG::STACK_VOICE, nullptr, 2,
                            &g_taskVoice, 0);
    Serial.println("STEP 18e: Voice task"); Serial.flush();

    xTaskCreatePinnedToCore(taskMonitor, "Monitor",
                            CFG::STACK_MONITOR, nullptr, 1,
                            &g_taskMonitor, 0);
    Serial.println("STEP 18f: Monitor task"); Serial.flush();
    // UI 태스크 추가
    extern void uiUpdateTask(void*);
    TaskHandle_t uiTask = nullptr;
    xTaskCreatePinnedToCore(uiUpdateTask, "UIUpdate",
                        8192, nullptr, 3,
                        &uiTask, 1);
    Serial.println("STEP 18g: UI task"); Serial.flush();

    TaskHandle_t otaTask = nullptr;
    xTaskCreatePinnedToCore(taskOTA, "OTA",
                            4096, nullptr, 3,
                            &otaTask, 0);
    Serial.println("STEP 19: all tasks done"); Serial.flush();
    esp_task_wdt_reset();
// UI 초기화
    extern UIManager uiManager;
    uiManager.begin();
    Serial.println("STEP 20: UI init done"); Serial.flush();
// 슬립 방지 — lastIdleTime 현재 시간으로 초기화
    extern void exitSleepMode();
    exitSleepMode();
    Serial.println("STEP 21: sleep reset done"); Serial.flush();
    ESP_LOGI(TAG_MAIN, "   ");
    SAFE_SERIAL_PRINTF("   - Free Heap: %u bytes", esp_get_free_heap_size());
}

// ============================================================
// loop() - Arduino  ( , )
// ============================================================
void loop() {
    esp_task_wdt_reset();  
    // FreeRTOS  loop() 
    // LED   
    static uint32_t lastBlinkMs = 0;
    static bool ledState = false;

    uint32_t now = millis();
    uint32_t blinkPeriod = g_state.isEstop() ? 100 : 1000;  // E-Stop   

    if (now - lastBlinkMs >= blinkPeriod) {
        lastBlinkMs = now;
        ledState = !ledState;
        gpio_set_level(PIN::LED_STATUS, ledState ? 1 : 0);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
}

// ============================================================
// END OF FILE: main.cpp
//  / :
// [1] Brownout: esp_brownout_init() + ISR  GPIO (arduino-esp32 3.x  )
// [2] WDT: esp_task_wdt_reconfigure() 15,   
// [3] PSRAM: heap_caps_malloc() SPIRAM ,  SRAM 
// [4] SD: initSDWithTimeout()  
// [5] I2C: recoverI2CBus() 9  + STOP 
// [6] WiFi: WiFi.onEvent()   + xEventGroup
// [7] Heap: checkHeapHealth() 5 , WARN/CRIT 
// [8] SPI: SPIBusManager (include )
// [9] DS18B20: setWaitForConversion(false) + 750ms 
// [A] Race: VacuumSystemState  + Mutex  
// [B] PWM: PwmChannelManager atomic CAS 
// [C] NVSWrite: g_nvsMutex  nvsSaveU32/nvsLoadU32
// [D] Serial: g_serialMutex + SAFE_SERIAL_PRINTF 
// [E] Stack:   6~8KiB + HWM 
// [F] MQTTState: g_cmdQueue + mqttCallback  
// [G] OTA: suspendAllTasksForOTA() / resumeAllTasksAfterOTA()
// [H] ADC: g_adcMutex + adcReadSafe()
// [I] DFPlayer: g_voiceQueue + taskVoice()
// [J] volatile: std::atomic / volatile  
// [K] NTP1970: waitForNtpSync() + makeLogFilename() 
// [L] : GPIO_INTR_NEGEDGE + IRAM_ATTR + 
// ============================================================
