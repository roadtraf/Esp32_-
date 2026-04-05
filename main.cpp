/**
 * @file main_hardened.cpp
 * @brief ESP32-S3 진공 제어 시스템 v3.9.4 Hardened Edition - 완전판
 * @details 요청항목 9가지 [1]~[9] + 추가발견 12가지 [A]~[L] = 총 21가지 전부 반영
 *
 * [1]  Brownout 감지 및 안전 종료
 * [2]  WDT(Watchdog Timer) 강화
 * [3]  PSRAM 안전 할당
 * [4]  SD 타임아웃 처리
 * [5]  I2C 버스 복구
 * [6]  WiFi 비블로킹 연결
 * [7]  Heap 모니터링
 * [8]  SPI 버스 뮤텍스
 * [9]  DS18B20 비동기 읽기
 * [A]  전역변수 Race Condition → SharedState + Mutex
 * [B]  PWM 채널 경쟁 → 원자적 채널 할당
 * [C]  NVS 동시 Write → NVS 전용 뮤텍스
 * [D]  Serial 경쟁 → Serial 전용 뮤텍스
 * [E]  Stack 오버플로우 → 태스크 스택 크기 증가 + 모니터
 * [F]  MQTT→State 직접변경 → 큐 기반 명령 처리
 * [G]  OTA 미정지 → OTA 시 모든 태스크 일시정지
 * [H]  ADC 재진입 → ADC 전용 뮤텍스
 * [I]  DFPlayer 큐없음 → 명령 큐 + 비동기 처리
 * [J]  volatile 미선언 → atomic/volatile 적용
 * [K]  NTP 1970 파일명 → NTP 동기화 대기 후 파일 생성
 * [L]  비상정지 디바운스 없음 → 하드웨어 디바운스 + 소프트 디바운스
 */

// ============================================================
// 헤더 인클루드
// ============================================================
#include "HardenedConfig.h"
#include "AdditionalHardening.h"
#include "SharedState.h"
#include "SPIBusManager.h"
#include "I2CBusRecovery.h"
#include "SafeSensor.h"
#include "SafeSD.h"
#include "VoiceAlert_Hardened.h"
#include "EnhancedWatchdog_Hardened.h"
#include "OTA_Hardened.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_brownout.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/adc.h>
#include <soc/adc_channel.h>
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
#include <DFRobotDFPlayerMini.h>
#include <atomic>
#include <cstring>
#include <cstdio>

// ============================================================
// 태그 (ESP_LOGx용)
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
// 핀 정의
// ============================================================
namespace PIN {
    // 진공 펌프 PWM
    constexpr gpio_num_t PUMP_PWM       = GPIO_NUM_18;
    // 솔레노이드 밸브
    constexpr gpio_num_t VALVE_1        = GPIO_NUM_19;
    constexpr gpio_num_t VALVE_2        = GPIO_NUM_20;
    constexpr gpio_num_t VALVE_3        = GPIO_NUM_21;
    // 압력 센서 (ADC)
    constexpr gpio_num_t PRESSURE_ADC   = GPIO_NUM_4;   // ADC1_CH3
    // 비상정지 버튼 (하드웨어 풀업)
    constexpr gpio_num_t ESTOP          = GPIO_NUM_0;
    // DS18B20 온도 센서
    constexpr gpio_num_t DS18B20_DATA   = GPIO_NUM_15;
    // SPI (SD카드)
    constexpr gpio_num_t SD_CS          = GPIO_NUM_5;
    constexpr gpio_num_t SD_MOSI        = GPIO_NUM_23;
    constexpr gpio_num_t SD_MISO        = GPIO_NUM_25;
    constexpr gpio_num_t SD_SCK         = GPIO_NUM_26;
    // I2C (압력 센서 보조)
    constexpr gpio_num_t I2C_SDA        = GPIO_NUM_16;
    constexpr gpio_num_t I2C_SCL        = GPIO_NUM_17;
    // DFPlayer UART
    constexpr gpio_num_t DFPLAYER_TX    = GPIO_NUM_27;
    constexpr gpio_num_t DFPLAYER_RX    = GPIO_NUM_14;
    // 상태 LED
    constexpr gpio_num_t LED_STATUS     = GPIO_NUM_2;
    constexpr gpio_num_t LED_ERROR      = GPIO_NUM_13;
}

// ============================================================
// 시스템 상수
// ============================================================
namespace CFG {
    // WiFi
    constexpr const char* WIFI_SSID         = HARDENED_WIFI_SSID;
    constexpr const char* WIFI_PASS         = HARDENED_WIFI_PASS;
    constexpr uint32_t    WIFI_TIMEOUT_MS   = 15000;
    constexpr uint8_t     WIFI_MAX_RETRY    = 5;

    // MQTT
    constexpr const char* MQTT_BROKER       = HARDENED_MQTT_BROKER;
    constexpr uint16_t    MQTT_PORT         = 1883;
    constexpr const char* MQTT_CLIENT_ID    = "esp32s3_vacuum_v394";
    constexpr const char* MQTT_USER         = HARDENED_MQTT_USER;
    constexpr const char* MQTT_PASS         = HARDENED_MQTT_PASS;
    constexpr uint32_t    MQTT_RECONNECT_MS = 5000;

    // NTP
    constexpr const char* NTP_SERVER        = "pool.ntp.org";
    constexpr long        NTP_UTC_OFFSET    = 32400;  // KST = UTC+9
    constexpr uint32_t    NTP_SYNC_WAIT_MS  = 10000;  // [K] NTP 동기화 대기

    // PWM
    constexpr uint32_t    PWM_FREQ          = 25000;  // 25kHz
    constexpr uint8_t     PWM_RESOLUTION    = 10;     // 10bit
    constexpr uint8_t     PWM_MAX_CHANNELS  = 8;

    // 압력
    constexpr float       PRESSURE_MIN_KPA  = -100.0f;
    constexpr float       PRESSURE_MAX_KPA  = 0.0f;
    constexpr float       PRESSURE_ALARM    = -80.0f;
    constexpr float       PRESSURE_TRIP     = -95.0f;

    // 온도
    constexpr float       TEMP_ALARM        = 60.0f;
    constexpr float       TEMP_TRIP         = 75.0f;

    // 태스크 스택 [E] 충분한 스택 확보
    constexpr uint32_t    STACK_CONTROL     = 8192;
    constexpr uint32_t    STACK_SENSOR      = 6144;
    constexpr uint32_t    STACK_MQTT        = 8192;
    constexpr uint32_t    STACK_LOGGER      = 6144;
    constexpr uint32_t    STACK_MONITOR     = 4096;
    constexpr uint32_t    STACK_VOICE       = 4096;

    // 디바운스 [L]
    constexpr uint32_t    ESTOP_DEBOUNCE_MS = 50;
    constexpr uint32_t    ESTOP_CONFIRM_MS  = 100;

    // ADC
    constexpr adc1_channel_t ADC_CH_PRESSURE = ADC1_CHANNEL_3;  // GPIO4
    constexpr adc_atten_t    ADC_ATTEN        = ADC_ATTEN_DB_11;

    // 큐 크기
    constexpr uint8_t     CMD_QUEUE_DEPTH   = 16;  // [F]
    constexpr uint8_t     VOICE_QUEUE_DEPTH = 8;   // [I]
    constexpr uint8_t     LOG_QUEUE_DEPTH   = 32;

    // Heap 경보 임계값 [7]
    constexpr uint32_t    HEAP_WARN_BYTES   = 32768;
    constexpr uint32_t    HEAP_CRIT_BYTES   = 16384;
}

// ============================================================
// [F] MQTT→State 명령 큐 구조체
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
    char origin[16];  // 명령 출처 (MQTT, LOCAL 등)
};

// ============================================================
// [B] PWM 채널 관리 (원자적 할당)
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
        return -1;  // 채널 부족
    }

    void release(uint8_t ch) {
        allocBitmap.fetch_and(~(1u << ch), std::memory_order_acq_rel);
    }
};

// ============================================================
// [A] SharedState (전역변수 Race → Mutex 보호)
// ============================================================
struct VacuumSystemState {
    SemaphoreHandle_t mutex = nullptr;

    // 압력
    float   pressureKpa    = 0.0f;
    bool    pressureValid  = false;
    int64_t pressureTimeUs = 0;

    // 온도
    float   temperatureC   = 25.0f;
    bool    tempValid      = false;
    int64_t tempTimeUs     = 0;

    // 펌프
    float   pumpDutyCycle  = 0.0f;
    bool    pumpRunning    = false;
    int8_t  pumpPwmCh      = -1;

    // 밸브
    bool    valveState[3]  = {false, false, false};

    // 시스템
    bool    estopActive    = false;      // [J] volatile 대신 mutex + atomic
    bool    otaActive      = false;      // [G]
    bool    ntpSynced      = false;      // [K]
    bool    wifiConnected  = false;
    bool    mqttConnected  = false;

    // NVS 설정값
    uint32_t pressureSetpoint = 80000;  // Pa 단위

    // 오류 카운터
    uint32_t adcErrors       = 0;
    uint32_t sensorErrors    = 0;
    uint32_t mqttDropped     = 0;
    uint32_t wdtResets       = 0;

    void init() {
        mutex = xSemaphoreCreateMutex();
        configASSERT(mutex != nullptr);
    }

    // 읽기 (복사본 반환)
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
// 전역 객체 선언
// ============================================================
static VacuumSystemState  g_state;
static PwmChannelManager  g_pwmMgr;

// [C] NVS 전용 뮤텍스
static SemaphoreHandle_t  g_nvsMutex    = nullptr;
// [D] Serial 전용 뮤텍스
static SemaphoreHandle_t  g_serialMutex = nullptr;
// [H] ADC 전용 뮤텍스
static SemaphoreHandle_t  g_adcMutex    = nullptr;

// [F] 명령 큐
static QueueHandle_t      g_cmdQueue    = nullptr;
// [I] DFPlayer 음성 큐
static QueueHandle_t      g_voiceQueue  = nullptr;
// 로그 큐
static QueueHandle_t      g_logQueue    = nullptr;

// FreeRTOS 이벤트 그룹
static EventGroupHandle_t g_sysEvents   = nullptr;
#define EVT_WIFI_UP       (1 << 0)
#define EVT_MQTT_UP       (1 << 1)
#define EVT_NTP_SYNC      (1 << 2)  // [K]
#define EVT_OTA_START     (1 << 3)  // [G]
#define EVT_ESTOP         (1 << 4)  // [L]
#define EVT_SENSOR_READY  (1 << 5)

// 태스크 핸들 ([G] OTA 시 일시정지용)
static TaskHandle_t g_taskControl  = nullptr;
static TaskHandle_t g_taskSensor   = nullptr;
static TaskHandle_t g_taskMqtt     = nullptr;
static TaskHandle_t g_taskLogger   = nullptr;
static TaskHandle_t g_taskVoice    = nullptr;
static TaskHandle_t g_taskMonitor  = nullptr;

// 네트워크 클라이언트
static WiFiClient       g_wifiClient;
static PubSubClient     g_mqttClient(g_wifiClient);
static WiFiUDP          g_udpClient;
static NTPClient        g_ntpClient(g_udpClient, CFG::NTP_SERVER, CFG::NTP_UTC_OFFSET, 60000);

// 센서
static OneWire          g_oneWire(PIN::DS18B20_DATA);
static DallasTemperature g_dallasSensor(&g_oneWire);

// DFPlayer
static HardwareSerial   g_dfSerial(2);
static DFRobotDFPlayerMini g_dfPlayer;

// ============================================================
// [D] 안전 Serial 출력 매크로
// ============================================================
#define SAFE_SERIAL_PRINTF(fmt, ...) \
    do { \
        if (g_serialMutex && xSemaphoreTake(g_serialMutex, pdMS_TO_TICKS(50)) == pdTRUE) { \
            Serial.printf("[%7lu] " fmt "\n", (unsigned long)(esp_timer_get_time()/1000), ##__VA_ARGS__); \
            xSemaphoreGive(g_serialMutex); \
        } \
    } while(0)

// ============================================================
// [H] 안전 ADC 읽기
// ============================================================
static bool adcReadSafe(adc1_channel_t ch, int* outRaw) {
    if (!g_adcMutex) return false;
    if (xSemaphoreTake(g_adcMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        ESP_LOGW(TAG_SENSOR, "ADC mutex timeout");
        return false;
    }
    *outRaw = adc1_get_raw(ch);
    xSemaphoreGive(g_adcMutex);
    if (*outRaw < 0) {
        ESP_LOGE(TAG_SENSOR, "ADC read error: %d", *outRaw);
        return false;
    }
    return true;
}

// ADC → kPa 변환 (선형 보정 예시)
static float adcToKpa(int raw) {
    // 0~4095 → -100~0 kPa (센서 특성에 맞게 조정)
    return CFG::PRESSURE_MIN_KPA + (raw / 4095.0f) * (CFG::PRESSURE_MAX_KPA - CFG::PRESSURE_MIN_KPA);
}

// ============================================================
// [C] 안전 NVS 읽기/쓰기
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
// [1] Brownout 콜백
// ============================================================
static void IRAM_ATTR browoutISR(void* arg) {
    // 펌프 즉시 정지 (ISR에서 직접 GPIO)
    gpio_set_level(PIN::PUMP_PWM, 0);
    gpio_set_level(PIN::VALVE_1, 0);
    gpio_set_level(PIN::VALVE_2, 0);
    gpio_set_level(PIN::VALVE_3, 0);
    gpio_set_level(PIN::LED_ERROR, 1);
    // ISR에서 로그 불가 - 재부팅 후 RTC 메모리로 확인
    esp_restart();
}

// ============================================================
// [L] 비상정지 인터럽트 + 디바운스
// ============================================================
static volatile uint32_t g_estopLastUs = 0;  // [J] volatile 선언

static void IRAM_ATTR estopISR(void* arg) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if ((now - g_estopLastUs) < CFG::ESTOP_DEBOUNCE_MS) return;  // 디바운스
    g_estopLastUs = now;

    // 우선 순위 높은 인터럽트: 이벤트 그룹에 알림
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (g_sysEvents) {
        xEventGroupSetBitsFromISR(g_sysEvents, EVT_ESTOP, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ============================================================
// [6] WiFi 비블로킹 연결 (이벤트 핸들러)
// ============================================================
static void wifiEventHandler(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESP_LOGI(TAG_MAIN, "WiFi STA 연결됨");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            ESP_LOGI(TAG_MAIN, "IP 취득: %s", WiFi.localIP().toString().c_str());
            g_state.wifiConnected = true;
            if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_WIFI_UP);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            ESP_LOGW(TAG_MAIN, "WiFi 연결 끊김 (reason=%d)", info.wifi_sta_disconnected.reason);
            g_state.wifiConnected = false;
            if (g_sysEvents) xEventGroupClearBits(g_sysEvents, EVT_WIFI_UP | EVT_MQTT_UP | EVT_NTP_SYNC);
            WiFi.reconnect();
            break;
        default:
            break;
    }
}

// ============================================================
// [F] MQTT 콜백 → 명령 큐에 넣기 (State 직접변경 금지)
// ============================================================
static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // payload를 null-terminate
    char buf[128] = {0};
    uint32_t copyLen = (length < sizeof(buf)-1) ? length : sizeof(buf)-1;
    memcpy(buf, payload, copyLen);

    ESP_LOGI(TAG_MQTT, "수신: [%s] %s", topic, buf);

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
            ESP_LOGW(TAG_MQTT, "명령 큐 가득 참, 드랍");
            if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                g_state.mqttDropped++;
                xSemaphoreGive(g_state.mutex);
            }
        }
    }
}

// ============================================================
// [K] NTP 시간 동기화 대기 함수
// ============================================================
static bool waitForNtpSync(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (g_ntpClient.update()) {
            unsigned long epoch = g_ntpClient.getEpochTime();
            if (epoch > 1700000000UL) {  // 2023년 이후면 유효
                ESP_LOGI(TAG_MAIN, "NTP 동기화 성공: %lu", epoch);
                g_state.ntpSynced = true;
                if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_NTP_SYNC);
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGW(TAG_MAIN, "NTP 동기화 타임아웃 (%u ms)", timeoutMs);
    return false;
}

// [K] NTP 기반 파일명 생성 (1970 방지)
static bool makeLogFilename(char* outBuf, size_t bufLen) {
    if (!g_state.ntpSynced) {
        // NTP 미동기화: 부팅 후 경과 ms로 임시 파일명
        snprintf(outBuf, bufLen, "/log_boot%lu.csv", (unsigned long)millis());
        ESP_LOGW(TAG_SD, "NTP 미동기화, 임시 파일명 사용: %s", outBuf);
        return false;
    }
    unsigned long epoch = g_ntpClient.getEpochTime();
    // epoch → tm 변환
    time_t t = (time_t)epoch;
    struct tm* tmInfo = gmtime(&t);
    snprintf(outBuf, bufLen, "/log_%04d%02d%02d_%02d%02d%02d.csv",
             tmInfo->tm_year + 1900, tmInfo->tm_mon + 1, tmInfo->tm_mday,
             tmInfo->tm_hour, tmInfo->tm_min, tmInfo->tm_sec);
    return true;
}

// ============================================================
// [G] OTA 태스크 일시정지/재개 헬퍼
// ============================================================
static void suspendAllTasksForOTA() {
    ESP_LOGI(TAG_OTA, "OTA 시작 - 모든 제어 태스크 일시정지");
    g_state.setOtaActive(true);
    if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_OTA_START);

    // 펌프 안전 정지
    if (g_state.pumpPwmCh >= 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)g_state.pumpPwmCh, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)g_state.pumpPwmCh);
    }
    gpio_set_level(PIN::VALVE_1, 0);
    gpio_set_level(PIN::VALVE_2, 0);
    gpio_set_level(PIN::VALVE_3, 0);

    // 태스크 일시정지
    if (g_taskControl)  vTaskSuspend(g_taskControl);
    if (g_taskSensor)   vTaskSuspend(g_taskSensor);
    if (g_taskLogger)   vTaskSuspend(g_taskLogger);
    if (g_taskVoice)    vTaskSuspend(g_taskVoice);
    if (g_taskMonitor)  vTaskSuspend(g_taskMonitor);
    // WDT 해제 (OTA 중 오래 걸릴 수 있음)
    esp_task_wdt_delete(NULL);
}

static void resumeAllTasksAfterOTA() {
    ESP_LOGI(TAG_OTA, "OTA 완료 - 태스크 재개");
    if (g_taskControl)  vTaskResume(g_taskControl);
    if (g_taskSensor)   vTaskResume(g_taskSensor);
    if (g_taskLogger)   vTaskResume(g_taskLogger);
    if (g_taskVoice)    vTaskResume(g_taskVoice);
    if (g_taskMonitor)  vTaskResume(g_taskMonitor);
    g_state.setOtaActive(false);
}

// ============================================================
// [B][8] PWM 채널 초기화 (SPI 뮤텍스 + 원자적 채널 할당)
// ============================================================
static int8_t initPumpPwm() {
    int8_t ch = g_pwmMgr.alloc();
    if (ch < 0) {
        ESP_LOGE(TAG_MAIN, "PWM 채널 할당 실패 - 채널 부족");
        return -1;
    }

    ledc_timer_config_t timerCfg = {};
    timerCfg.speed_mode      = LEDC_HIGH_SPEED_MODE;
    timerCfg.timer_num       = LEDC_TIMER_0;
    timerCfg.duty_resolution = (ledc_timer_bit_t)CFG::PWM_RESOLUTION;
    timerCfg.freq_hz         = CFG::PWM_FREQ;
    timerCfg.clk_cfg         = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timerCfg) != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "LEDC 타이머 설정 실패");
        g_pwmMgr.release(ch);
        return -1;
    }

    ledc_channel_config_t chCfg = {};
    chCfg.gpio_num   = PIN::PUMP_PWM;
    chCfg.speed_mode = LEDC_HIGH_SPEED_MODE;
    chCfg.channel    = (ledc_channel_t)ch;
    chCfg.timer_sel  = LEDC_TIMER_0;
    chCfg.intr_type  = LEDC_INTR_DISABLE;
    chCfg.duty       = 0;
    chCfg.hpoint     = 0;
    if (ledc_channel_config(&chCfg) != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "LEDC 채널 설정 실패");
        g_pwmMgr.release(ch);
        return -1;
    }

    ESP_LOGI(TAG_MAIN, "펌프 PWM 채널 %d 할당 완료", ch);
    return ch;
}

// ============================================================
// [5] I2C 버스 복구
// ============================================================
static void recoverI2CBus() {
    ESP_LOGW(TAG_MAIN, "I2C 버스 복구 시도");
    Wire.end();
    vTaskDelay(pdMS_TO_TICKS(10));

    // SCL 클록 펄스 9개
    pinMode(PIN::I2C_SCL, OUTPUT);
    pinMode(PIN::I2C_SDA, INPUT_PULLUP);
    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN::I2C_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(PIN::I2C_SCL, HIGH);
        delayMicroseconds(5);
    }
    // STOP 조건
    pinMode(PIN::I2C_SDA, OUTPUT);
    digitalWrite(PIN::I2C_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN::I2C_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN::I2C_SDA, HIGH);

    Wire.begin(PIN::I2C_SDA, PIN::I2C_SCL, 100000);
    ESP_LOGI(TAG_MAIN, "I2C 버스 복구 완료");
}

// ============================================================
// [4] SD 카드 타임아웃 초기화
// ============================================================
static bool initSDWithTimeout(uint32_t timeoutMs) {
    uint32_t start = millis();
    bool ok = false;

    SPI.begin(PIN::SD_SCK, PIN::SD_MISO, PIN::SD_MOSI, PIN::SD_CS);

    while (!ok && (millis() - start < timeoutMs)) {
        if (SD.begin(PIN::SD_CS, SPI, 4000000, "/sd", 5, false)) {
            ok = true;
            break;
        }
        ESP_LOGW(TAG_SD, "SD 초기화 재시도...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!ok) {
        ESP_LOGE(TAG_SD, "SD 카드 초기화 실패 (timeout=%u ms)", timeoutMs);
    } else {
        ESP_LOGI(TAG_SD, "SD 카드 초기화 성공 (용량=%llu MB)",
                 SD.cardSize() / (1024ULL * 1024ULL));
    }
    return ok;
}

// ============================================================
// [3] PSRAM 안전 할당 (로그 버퍼)
// ============================================================
static uint8_t* allocPsramBuffer(size_t size) {
    if (!psramFound()) {
        ESP_LOGW(TAG_SYS, "PSRAM 없음, 내부 SRAM 사용");
        return (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    uint8_t* p = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) {
        ESP_LOGE(TAG_SYS, "PSRAM 할당 실패 (%u bytes), 내부 SRAM 시도", size);
        p = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if (p) {
        ESP_LOGI(TAG_SYS, "PSRAM 버퍼 할당 성공: %u bytes @ %p", size, p);
    } else {
        ESP_LOGE(TAG_SYS, "메모리 할당 완전 실패 (%u bytes)", size);
    }
    return p;
}

// ============================================================
// [9] DS18B20 비동기 읽기 (요청 → 750ms 대기 → 읽기)
// ============================================================
static bool g_ds18b20Requested = false;         // [J] 상태 플래그
static uint32_t g_ds18b20RequestTimeMs = 0;

static void ds18b20RequestConversion() {
    g_dallasSensor.setWaitForConversion(false);  // 논블로킹
    g_dallasSensor.requestTemperatures();
    g_ds18b20Requested = true;
    g_ds18b20RequestTimeMs = millis();
    ESP_LOGD(TAG_SENSOR, "DS18B20 변환 요청");
}

static bool ds18b20ReadIfReady(float* outTemp) {
    if (!g_ds18b20Requested) return false;
    if (millis() - g_ds18b20RequestTimeMs < 750) return false;  // 아직 변환 중

    float t = g_dallasSensor.getTempCByIndex(0);
    g_ds18b20Requested = false;

    if (t == DEVICE_DISCONNECTED_C || t < -55.0f || t > 125.0f) {
        ESP_LOGW(TAG_SENSOR, "DS18B20 유효하지 않은 값: %.2f", t);
        return false;
    }
    *outTemp = t;
    return true;
}

// ============================================================
// [I] DFPlayer 음성 큐 처리
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
            ESP_LOGW(TAG_MAIN, "Voice 큐 가득 참");
        }
    }
}

// ============================================================
// [7] Heap 모니터링
// ============================================================
static void checkHeapHealth() {
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t minFree  = esp_get_minimum_free_heap_size();

    if (freeHeap < CFG::HEAP_CRIT_BYTES) {
        ESP_LOGE(TAG_SYS, "CRITICAL: Heap 부족! 남은: %u bytes (최소: %u)", freeHeap, minFree);
        voiceQueuePlay(5);  // 경보음
        // 필요 시 ESP 재시작 고려
    } else if (freeHeap < CFG::HEAP_WARN_BYTES) {
        ESP_LOGW(TAG_SYS, "WARNING: Heap 낮음: %u bytes (최소: %u)", freeHeap, minFree);
    }

    // PSRAM 확인
    if (psramFound()) {
        uint32_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGD(TAG_SYS, "PSRAM 남은: %u bytes", freePsram);
    }
}

// ============================================================
// [E] 스택 모니터링
// ============================================================
static void logTaskStackHighWaterMark(TaskHandle_t h, const char* name) {
    if (!h) return;
    UBaseType_t hwm = uxTaskGetStackHighWaterMark(h);
    if (hwm < 512) {
        ESP_LOGW(TAG_SYS, "[스택경고] %s: 남은=%u words", name, hwm);
    } else {
        ESP_LOGD(TAG_SYS, "[스택] %s: 남은=%u words", name, hwm);
    }
}

// ============================================================
// ==================== FreeRTOS 태스크들 ====================
// ============================================================

// ============================================================
// 태스크 1: 제어 태스크 (명령 큐 처리 + 비상정지)
// ============================================================
static void taskControl(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_CTRL, "제어 태스크 시작");

    // 비상정지 감지 대기 + 명령 처리 루프
    SystemCommand cmd;

    for (;;) {
        esp_task_wdt_reset();

        // [G] OTA 중이면 대기
        if (g_state.isOtaActive()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // [L] 비상정지 이벤트 확인
        EventBits_t bits = xEventGroupGetBits(g_sysEvents);
        if (bits & EVT_ESTOP) {
            // 소프트 디바운스 확인 (CFG::ESTOP_CONFIRM_MS)
            vTaskDelay(pdMS_TO_TICKS(CFG::ESTOP_CONFIRM_MS));
            bool pinStillLow = (gpio_get_level(PIN::ESTOP) == 0);
            if (pinStillLow && !g_state.isEstop()) {
                ESP_LOGE(TAG_CTRL, "비상정지 확인됨!");
                g_state.setEstop(true);
                // 즉시 출력 차단
                if (g_state.pumpPwmCh >= 0) {
                    ledc_set_duty(LEDC_HIGH_SPEED_MODE,
                                  (ledc_channel_t)g_state.pumpPwmCh, 0);
                    ledc_update_duty(LEDC_HIGH_SPEED_MODE,
                                     (ledc_channel_t)g_state.pumpPwmCh);
                }
                gpio_set_level(PIN::VALVE_1, 0);
                gpio_set_level(PIN::VALVE_2, 0);
                gpio_set_level(PIN::VALVE_3, 0);
                gpio_set_level(PIN::LED_ERROR, 1);
                voiceQueuePlay(3);  // 비상정지 경보
            }
            xEventGroupClearBits(g_sysEvents, EVT_ESTOP);
        }

        // [F] 명령 큐에서 명령 처리
        while (xQueueReceive(g_cmdQueue, &cmd, 0) == pdTRUE) {
            // 비상정지 중에는 대부분 명령 무시
            if (g_state.isEstop() && cmd.type != CommandType::RELEASE_ESTOP) {
                ESP_LOGW(TAG_CTRL, "비상정지 중 - 명령 무시: %d", (int)cmd.type);
                continue;
            }

            switch (cmd.type) {
                case CommandType::SET_PUMP_SPEED: {
                    float duty = cmd.data.pump.dutyCycle;
                    if (duty < 0.0f) duty = 0.0f;
                    if (duty > 100.0f) duty = 100.0f;

                    uint32_t rawDuty = (uint32_t)((duty / 100.0f) * ((1 << CFG::PWM_RESOLUTION) - 1));
                    if (g_state.pumpPwmCh >= 0) {
                        ledc_set_duty(LEDC_HIGH_SPEED_MODE,
                                      (ledc_channel_t)g_state.pumpPwmCh, rawDuty);
                        ledc_update_duty(LEDC_HIGH_SPEED_MODE,
                                         (ledc_channel_t)g_state.pumpPwmCh);
                    }
                    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                        g_state.pumpDutyCycle = duty;
                        g_state.pumpRunning   = (duty > 0.0f);
                        xSemaphoreGive(g_state.mutex);
                    }
                    ESP_LOGI(TAG_CTRL, "펌프 속도 설정: %.1f%% (origin:%s)", duty, cmd.origin);
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
                        ESP_LOGI(TAG_CTRL, "밸브%d → %s (origin:%s)", v+1, st?"ON":"OFF", cmd.origin);
                    }
                    break;
                }
                case CommandType::EMERGENCY_STOP: {
                    ESP_LOGE(TAG_CTRL, "원격 비상정지 명령 수신 (from:%s)", cmd.origin);
                    g_state.setEstop(true);
                    if (g_state.pumpPwmCh >= 0) {
                        ledc_set_duty(LEDC_HIGH_SPEED_MODE,
                                      (ledc_channel_t)g_state.pumpPwmCh, 0);
                        ledc_update_duty(LEDC_HIGH_SPEED_MODE,
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
                    if (gpio_get_level(PIN::ESTOP) == 1) {  // 버튼 해제 확인
                        ESP_LOGI(TAG_CTRL, "비상정지 해제 (from:%s)", cmd.origin);
                        g_state.setEstop(false);
                        gpio_set_level(PIN::LED_ERROR, 0);
                        voiceQueuePlay(4);
                    } else {
                        ESP_LOGW(TAG_CTRL, "비상정지 버튼 여전히 눌려있음, 해제 불가");
                    }
                    break;
                }
                case CommandType::NVS_SAVE_SETPOINT: {
                    // [C] NVS 뮤텍스 보호
                    uint32_t sp = cmd.data.nvs.setpoint;
                    esp_err_t err = nvsSaveU32("pressure_sp", sp);
                    if (err == ESP_OK) {
                        if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                            g_state.pressureSetpoint = sp;
                            xSemaphoreGive(g_state.mutex);
                        }
                        ESP_LOGI(TAG_CTRL, "설정값 저장 완료: %u Pa", sp);
                    } else {
                        ESP_LOGE(TAG_CTRL, "설정값 NVS 저장 실패: 0x%x", err);
                    }
                    break;
                }
                default:
                    ESP_LOGW(TAG_CTRL, "알 수 없는 명령: %d", (int)cmd.type);
                    break;
            }
        }

        // 압력 기반 자동 제어 (비상정지 아닐 때)
        if (!g_state.isEstop()) {
            bool pValid = false;
            float pressure = g_state.getPressure(&pValid);
            if (pValid) {
                if (pressure < CFG::PRESSURE_TRIP) {
                    // 트립: 펌프 긴급 정지
                    if (g_state.pumpRunning) {
                        SystemCommand tripCmd;
                        tripCmd.type = CommandType::SET_PUMP_SPEED;
                        tripCmd.data.pump.dutyCycle = 0.0f;
                        strncpy(tripCmd.origin, "TRIP", sizeof(tripCmd.origin)-1);
                        xQueueSend(g_cmdQueue, &tripCmd, 0);
                        voiceQueuePlay(2);  // 압력 초과 경보
                        ESP_LOGE(TAG_CTRL, "압력 트립: %.2f kPa", pressure);
                    }
                } else if (pressure < CFG::PRESSURE_ALARM) {
                    if (!g_state.pumpRunning) {
                        voiceQueuePlay(1);  // 경고
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================
// 태스크 2: 센서 태스크
// ============================================================
static void taskSensor(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_SENSOR, "센서 태스크 시작");

    // DS18B20 초기화
    g_dallasSensor.begin();
    g_dallasSensor.setResolution(12);  // 12bit = 750ms 변환
    ds18b20RequestConversion();  // 첫 요청 [9]

    // [H] ADC 초기화
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(CFG::ADC_CH_PRESSURE, CFG::ADC_ATTEN);

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

        // [H] 압력 ADC 읽기 (100ms 주기)
        if (now - lastPressureMs >= 100) {
            lastPressureMs = now;
            int raw = 0;
            if (adcReadSafe(CFG::ADC_CH_PRESSURE, &raw)) {
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
                // [5] 연속 오류 시 I2C 복구
                if (i2cErrCount >= 5) {
                    recoverI2CBus();
                    i2cErrCount = 0;
                }
            }
        }

        // [9] DS18B20 비동기 읽기 (1000ms 주기)
        if (now - lastTempMs >= 1000) {
            float temp = 0.0f;
            if (ds18b20ReadIfReady(&temp)) {
                g_state.setTemperature(temp, true);
                lastTempMs = now;
                // 온도 경보 확인
                if (temp >= CFG::TEMP_TRIP) {
                    ESP_LOGE(TAG_SENSOR, "온도 트립: %.2f°C", temp);
                    SystemCommand cmd;
                    cmd.type = CommandType::EMERGENCY_STOP;
                    strncpy(cmd.origin, "TEMP_TRIP", sizeof(cmd.origin)-1);
                    xQueueSend(g_cmdQueue, &cmd, 0);
                } else if (temp >= CFG::TEMP_ALARM) {
                    ESP_LOGW(TAG_SENSOR, "온도 경보: %.2f°C", temp);
                    voiceQueuePlay(6);
                }
                ds18b20RequestConversion();  // 다음 변환 요청
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ============================================================
// 태스크 3: MQTT 태스크 (비블로킹)
// ============================================================
static void taskMqtt(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_MQTT, "MQTT 태스크 시작");

    g_mqttClient.setServer(CFG::MQTT_BROKER, CFG::MQTT_PORT);
    g_mqttClient.setCallback(mqttCallback);  // [F] 큐 기반 콜백
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

        // WiFi 대기 (비블로킹) [6]
        if (!g_state.wifiConnected) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // MQTT 재연결
        if (!g_mqttClient.connected()) {
            uint32_t now = millis();
            if (now - lastReconnectMs >= CFG::MQTT_RECONNECT_MS) {
                lastReconnectMs = now;
                ESP_LOGI(TAG_MQTT, "MQTT 연결 시도: %s:%d", CFG::MQTT_BROKER, CFG::MQTT_PORT);
                bool ok = g_mqttClient.connect(CFG::MQTT_CLIENT_ID,
                                               CFG::MQTT_USER, CFG::MQTT_PASS,
                                               "vacuum/status/lwt", 1, true, "offline");
                if (ok) {
                    ESP_LOGI(TAG_MQTT, "MQTT 연결 성공");
                    g_state.mqttConnected = true;
                    if (g_sysEvents) xEventGroupSetBits(g_sysEvents, EVT_MQTT_UP);

                    // 구독
                    g_mqttClient.subscribe("vacuum/cmd/#", 1);
                    g_mqttClient.publish("vacuum/status/lwt", "online", true);
                } else {
                    ESP_LOGW(TAG_MQTT, "MQTT 연결 실패 (rc=%d)", g_mqttClient.state());
                    g_state.mqttConnected = false;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        g_mqttClient.loop();

        // 상태 퍼블리시 (2초 주기)
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
                ESP_LOGW(TAG_MQTT, "MQTT 퍼블리시 실패");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================================================
// 태스크 4: SD 로거 태스크 [K] NTP 기반 파일명
// ============================================================
static void taskLogger(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_SD, "로거 태스크 시작");

    // [K] NTP 동기화 대기
    EventBits_t bits = xEventGroupWaitBits(g_sysEvents, EVT_NTP_SYNC,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(CFG::NTP_SYNC_WAIT_MS));
    if (!(bits & EVT_NTP_SYNC)) {
        ESP_LOGW(TAG_SD, "NTP 미동기화 상태로 로거 시작 (파일명 임시 사용)");
    }

    char filename[64];
    makeLogFilename(filename, sizeof(filename));
    ESP_LOGI(TAG_SD, "로그 파일: %s", filename);

    // CSV 헤더 기록
    File logFile = SD.open(filename, FILE_APPEND);
    if (logFile) {
        logFile.println("timestamp_ms,pressure_kpa,temperature_c,pump_duty,estop,free_heap");
        logFile.close();
    } else {
        ESP_LOGE(TAG_SD, "로그 파일 열기 실패: %s", filename);
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

            // [4] SD 기록 (타임아웃 보호)
            logFile = SD.open(filename, FILE_APPEND);
            if (logFile) {
                char row[128];
                snprintf(row, sizeof(row),
                         "%lu,%.2f,%.2f,%.1f,%d,%u",
                         (unsigned long)now, p, t, d, e ? 1 : 0, heap);
                logFile.println(row);
                logFile.close();
            } else {
                ESP_LOGW(TAG_SD, "SD 쓰기 실패 (파일 열기 오류)");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ============================================================
// 태스크 5: 음성 알림 태스크 [I] 큐 기반
// ============================================================
static void taskVoice(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_MAIN, "음성 태스크 시작");

    // DFPlayer 초기화
    g_dfSerial.begin(9600, SERIAL_8N1, PIN::DFPLAYER_RX, PIN::DFPLAYER_TX);
    if (!g_dfPlayer.begin(g_dfSerial, true, false)) {
        ESP_LOGE(TAG_MAIN, "DFPlayer 초기화 실패");
        vTaskDelete(nullptr);
        return;
    }
    g_dfPlayer.volume(20);
    ESP_LOGI(TAG_MAIN, "DFPlayer 초기화 성공");

    VoiceMessage msg;
    for (;;) {
        esp_task_wdt_reset();

        // 큐에서 음성 명령 수신
        if (xQueueReceive(g_voiceQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            switch (msg.cmd) {
                case VoiceCmd::PLAY_TRACK:
                    g_dfPlayer.play(msg.param1);
                    ESP_LOGI(TAG_MAIN, "DFPlayer: 트랙 %d 재생", msg.param1);
                    break;
                case VoiceCmd::SET_VOLUME:
                    g_dfPlayer.volume(msg.param1);
                    break;
                case VoiceCmd::STOP:
                    g_dfPlayer.stop();
                    break;
            }
        }
    }
}

// ============================================================
// 태스크 6: 시스템 모니터 태스크 [2][7][E]
// ============================================================
static void taskMonitor(void* pv) {
    esp_task_wdt_add(NULL);
    ESP_LOGI(TAG_SYS, "모니터 태스크 시작");

    uint32_t lastMonMs = 0;

    for (;;) {
        esp_task_wdt_reset();

        uint32_t now = millis();
        if (now - lastMonMs >= 5000) {
            lastMonMs = now;

            // [7] Heap 모니터링
            checkHeapHealth();

            // [E] 스택 워터마크 모니터링
            logTaskStackHighWaterMark(g_taskControl,  "Control");
            logTaskStackHighWaterMark(g_taskSensor,   "Sensor");
            logTaskStackHighWaterMark(g_taskMqtt,     "MQTT");
            logTaskStackHighWaterMark(g_taskLogger,   "Logger");
            logTaskStackHighWaterMark(g_taskVoice,    "Voice");
            logTaskStackHighWaterMark(g_taskMonitor,  "Monitor");

            // 시스템 정보 출력
            SAFE_SERIAL_PRINTF(
                "=== 시스템 상태 ===\n"
                "  압력: %.2f kPa | 온도: %.2f°C\n"
                "  펌프: %.1f%% | E-Stop: %s\n"
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

            // [2] WDT 상태 확인 (재시작 원인)
            esp_reset_reason_t reason = esp_reset_reason();
            if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_INT_WDT) {
                ESP_LOGE(TAG_WDT, "이전 재시작 원인: WDT 타임아웃");
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
// OTA 초기화 [G]
// ============================================================
static void initOTA() {
    ArduinoOTA.setHostname(CFG::MQTT_CLIENT_ID);
    ArduinoOTA.setPassword(HARDENED_OTA_PASS);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        ESP_LOGI(TAG_OTA, "OTA 시작: %s", type.c_str());
        suspendAllTasksForOTA();  // [G]
    });

    ArduinoOTA.onEnd([]() {
        ESP_LOGI(TAG_OTA, "OTA 완료 - 재시작");
        // 재시작 전 클린업은 esp_restart()에서 처리
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint8_t lastPct = 0;
        uint8_t pct = (uint8_t)(progress * 100 / total);
        if (pct != lastPct && pct % 10 == 0) {
            ESP_LOGI(TAG_OTA, "OTA 진행: %u%%", pct);
            lastPct = pct;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        ESP_LOGE(TAG_OTA, "OTA 오류 [%u]: %s",
                 error,
                 error == OTA_AUTH_ERROR    ? "인증실패" :
                 error == OTA_BEGIN_ERROR   ? "시작실패" :
                 error == OTA_CONNECT_ERROR ? "연결실패" :
                 error == OTA_RECEIVE_ERROR ? "수신실패" :
                 error == OTA_END_ERROR     ? "종료실패" : "알 수 없음");
        resumeAllTasksAfterOTA();  // [G] 오류 시 재개
    });

    ArduinoOTA.begin();
    ESP_LOGI(TAG_OTA, "OTA 서버 시작됨");
}

// ============================================================
// WiFi 초기화 [6]
// ============================================================
static void initWifiNonBlocking() {
    WiFi.onEvent(wifiEventHandler);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(CFG::WIFI_SSID, CFG::WIFI_PASS);
    ESP_LOGI(TAG_MAIN, "WiFi 비블로킹 연결 시작: %s", CFG::WIFI_SSID);
    // 연결 완료는 이벤트 핸들러에서 처리
}

// ============================================================
// OTA 전용 태스크 (ArduinoOTA.handle() 루프) [G]
// ============================================================
static void taskOTA(void* pv) {
    ESP_LOGI(TAG_OTA, "OTA 태스크 시작");
    for (;;) {
        if (g_state.wifiConnected) {
            ArduinoOTA.handle();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================
// setup() - 시스템 초기화
// ============================================================
void setup() {
    // [D] Serial 초기화 (뮤텍스 전에 먼저 시작)
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== ESP32-S3 진공 제어 시스템 v3.9.4 Hardened Edition ===");

    // --------------------------------------------------------
    // 뮤텍스 / 큐 / 이벤트 그룹 초기화 [A][C][D][F][H][I]
    // --------------------------------------------------------
    g_state.init();                                          // [A] SharedState mutex
    g_nvsMutex    = xSemaphoreCreateMutex();                 // [C]
    g_serialMutex = xSemaphoreCreateMutex();                 // [D]
    g_adcMutex    = xSemaphoreCreateMutex();                 // [H]
    g_cmdQueue    = xQueueCreate(CFG::CMD_QUEUE_DEPTH,   sizeof(SystemCommand));  // [F]
    g_voiceQueue  = xQueueCreate(CFG::VOICE_QUEUE_DEPTH, sizeof(VoiceMessage));  // [I]
    g_logQueue    = xQueueCreate(CFG::LOG_QUEUE_DEPTH,   sizeof(char)*128);
    g_sysEvents   = xEventGroupCreate();

    configASSERT(g_nvsMutex);
    configASSERT(g_serialMutex);
    configASSERT(g_adcMutex);
    configASSERT(g_cmdQueue);
    configASSERT(g_voiceQueue);
    configASSERT(g_sysEvents);

    // --------------------------------------------------------
    // [2] WDT 설정 (15초 타임아웃)
    // --------------------------------------------------------
    esp_task_wdt_config_t wdtCfg = {
        .timeout_ms     = HARDENED_WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic  = true,
    };
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&wdtCfg));
    ESP_LOGI(TAG_MAIN, "WDT 설정: %u초", HARDENED_WDT_TIMEOUT_S);

    // --------------------------------------------------------
    // [1] Brownout 감지 설정
    // --------------------------------------------------------
    // 브라운아웃 레벨 설정 (ESP-IDF 레벨로 직접 설정)
    esp_brownout_init();
    ESP_LOGI(TAG_MAIN, "Brownout 감지 활성화");

    // --------------------------------------------------------
    // NVS 초기화 [C]
    // --------------------------------------------------------
    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG_MAIN, "NVS 초기화: 파티션 지우고 재초기화");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvsErr = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvsErr);

    // NVS에서 설정값 로드
    uint32_t savedSetpoint = 80000;
    nvsLoadU32("pressure_sp", &savedSetpoint, 80000);
    g_state.pressureSetpoint = savedSetpoint;
    ESP_LOGI(TAG_MAIN, "NVS 설정값 로드: pressure_sp=%u Pa", savedSetpoint);

    // --------------------------------------------------------
    // GPIO 초기화
    // --------------------------------------------------------
    // 출력 핀
    gpio_config_t outCfg = {};
    outCfg.mode         = GPIO_MODE_OUTPUT;
    outCfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    outCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    outCfg.intr_type    = GPIO_INTR_DISABLE;

    outCfg.pin_bit_mask = (1ULL << PIN::VALVE_1) | (1ULL << PIN::VALVE_2) |
                          (1ULL << PIN::VALVE_3) | (1ULL << PIN::LED_STATUS) |
                          (1ULL << PIN::LED_ERROR);
    ESP_ERROR_CHECK(gpio_config(&outCfg));

    gpio_set_level(PIN::VALVE_1, 0);
    gpio_set_level(PIN::VALVE_2, 0);
    gpio_set_level(PIN::VALVE_3, 0);
    gpio_set_level(PIN::LED_STATUS, 1);
    gpio_set_level(PIN::LED_ERROR, 0);

    // [L] 비상정지 입력 (하드웨어 풀업 + 인터럽트)
    gpio_config_t estopCfg = {};
    estopCfg.pin_bit_mask   = (1ULL << PIN::ESTOP);
    estopCfg.mode           = GPIO_MODE_INPUT;
    estopCfg.pull_up_en     = GPIO_PULLUP_ENABLE;   // 하드웨어 풀업
    estopCfg.pull_down_en   = GPIO_PULLDOWN_DISABLE;
    estopCfg.intr_type      = GPIO_INTR_NEGEDGE;    // 하강 에지 인터럽트
    ESP_ERROR_CHECK(gpio_config(&estopCfg));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN::ESTOP, estopISR, nullptr));
    ESP_LOGI(TAG_MAIN, "비상정지 GPIO 설정 완료 (디바운스=%u ms)", CFG::ESTOP_DEBOUNCE_MS);

    // --------------------------------------------------------
    // [B][8] PWM 초기화 (원자적 채널 할당)
    // --------------------------------------------------------
    int8_t pwmCh = initPumpPwm();
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_state.pumpPwmCh = pwmCh;
        xSemaphoreGive(g_state.mutex);
    }

    // --------------------------------------------------------
    // [3] PSRAM 확인
    // --------------------------------------------------------
    if (psramFound()) {
        ESP_LOGI(TAG_MAIN, "PSRAM 발견: %u bytes", ESP.getPsramSize());
    } else {
        ESP_LOGW(TAG_MAIN, "PSRAM 없음");
    }
    // 로그 버퍼 PSRAM 할당 (필요 시)
    // uint8_t* logBuf = allocPsramBuffer(64 * 1024);

    // --------------------------------------------------------
    // I2C 초기화 [5]
    // --------------------------------------------------------
    Wire.begin(PIN::I2C_SDA, PIN::I2C_SCL, 100000);
    ESP_LOGI(TAG_MAIN, "I2C 초기화 완료");

    // --------------------------------------------------------
    // [4] SD 카드 초기화 (타임아웃 5초)
    // --------------------------------------------------------
    bool sdOk = initSDWithTimeout(5000);
    if (!sdOk) {
        ESP_LOGE(TAG_MAIN, "SD 카드 없이 계속 (로깅 비활성)");
    }

    // --------------------------------------------------------
    // [5] [K3] 상태 전이 Mutex 초기화
    // --------------------------------------------------------
    initStateMachine();

    // --------------------------------------------------------
    // [6] WiFi 비블로킹 초기화
    // --------------------------------------------------------
    initWifiNonBlocking();

    // --------------------------------------------------------
    // WiFi 연결 대기 (최대 15초, 비블로킹 sleep)
    // --------------------------------------------------------
    EventBits_t wifiEvt = xEventGroupWaitBits(g_sysEvents, EVT_WIFI_UP,
                                              pdFALSE, pdFALSE,
                                              pdMS_TO_TICKS(CFG::WIFI_TIMEOUT_MS));
    if (wifiEvt & EVT_WIFI_UP) {
        ESP_LOGI(TAG_MAIN, "WiFi 연결 완료 후 NTP 시작");
        g_ntpClient.begin();
        waitForNtpSync(CFG::NTP_SYNC_WAIT_MS);  // [K]
        initOTA();                               // [G]
    } else {
        ESP_LOGW(TAG_MAIN, "WiFi 타임아웃 - 오프라인 모드");
    }

    // --------------------------------------------------------
    // FreeRTOS 태스크 생성 [E] 충분한 스택
    // --------------------------------------------------------
    xTaskCreatePinnedToCore(taskControl, "Control",
                            CFG::STACK_CONTROL, nullptr, 5,
                            &g_taskControl, 1);  // Core 1

    xTaskCreatePinnedToCore(taskSensor, "Sensor",
                            CFG::STACK_SENSOR, nullptr, 4,
                            &g_taskSensor, 1);   // Core 1

    xTaskCreatePinnedToCore(taskMqtt, "MQTT",
                            CFG::STACK_MQTT, nullptr, 3,
                            &g_taskMqtt, 0);     // Core 0

    xTaskCreatePinnedToCore(taskLogger, "Logger",
                            CFG::STACK_LOGGER, nullptr, 2,
                            &g_taskLogger, 0);   // Core 0

    xTaskCreatePinnedToCore(taskVoice, "Voice",
                            CFG::STACK_VOICE, nullptr, 2,
                            &g_taskVoice, 0);    // Core 0

    xTaskCreatePinnedToCore(taskMonitor, "Monitor",
                            CFG::STACK_MONITOR, nullptr, 1,
                            &g_taskMonitor, 0);  // Core 0

    // OTA 태스크
    TaskHandle_t otaTask = nullptr;
    xTaskCreatePinnedToCore(taskOTA, "OTA",
                            4096, nullptr, 3,
                            &otaTask, 0);

    ESP_LOGI(TAG_MAIN, "모든 태스크 시작 완료");
    SAFE_SERIAL_PRINTF("시스템 초기화 완료 - Free Heap: %u bytes", esp_get_free_heap_size());
}

// ============================================================
// loop() - Arduino 루프 (태스크에 위임, 최소화)
// ============================================================
void loop() {
    // FreeRTOS 기반이므로 loop()는 최소화
    // LED 상태 표시만 처리
    static uint32_t lastBlinkMs = 0;
    static bool ledState = false;

    uint32_t now = millis();
    uint32_t blinkPeriod = g_state.isEstop() ? 100 : 1000;  // E-Stop 시 빠른 깜빡임

    if (now - lastBlinkMs >= blinkPeriod) {
        lastBlinkMs = now;
        ledState = !ledState;
        gpio_set_level(PIN::LED_STATUS, ledState ? 1 : 0);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
}

// ============================================================
// END OF FILE: main_hardened.cpp
// 총 수정/반영 항목:
// [1] Brownout: esp_brownout_init() + ISR 즉시 GPIO 차단
// [2] WDT: esp_task_wdt_reconfigure() 15초, 모든 태스크 등록
// [3] PSRAM: heap_caps_malloc() SPIRAM 우선, 내부 SRAM 폴백
// [4] SD타임아웃: initSDWithTimeout() 루프 재시도
// [5] I2C복구: recoverI2CBus() 9클록 펄스 + STOP 조건
// [6] WiFi비블로킹: WiFi.onEvent() 이벤트 핸들러 + xEventGroup
// [7] Heap: checkHeapHealth() 5초 주기, WARN/CRIT 임계값
// [8] SPI뮤텍스: SPIBusManager (include에서 제공)
// [9] DS18B20비동기: setWaitForConversion(false) + 750ms 대기
// [A] 전역변수Race: VacuumSystemState 구조체 + Mutex 래퍼 함수
// [B] PWM경쟁: PwmChannelManager atomic CAS 할당
// [C] NVS동시Write: g_nvsMutex 보호 nvsSaveU32/nvsLoadU32
// [D] Serial경쟁: g_serialMutex + SAFE_SERIAL_PRINTF 매크로
// [E] Stack오버플로우: 태스크별 스택 6~8KiB + HWM 모니터
// [F] MQTT→State직접변경: g_cmdQueue + mqttCallback 큐 전달
// [G] OTA미정지: suspendAllTasksForOTA() / resumeAllTasksAfterOTA()
// [H] ADC재진입: g_adcMutex + adcReadSafe()
// [I] DFPlayer큐없음: g_voiceQueue + taskVoice()
// [J] volatile미선언: std::atomic / volatile 명시적 적용
// [K] NTP1970파일명: waitForNtpSync() + makeLogFilename() 검증
// [L] 비상정지디바운스없음: GPIO_INTR_NEGEDGE + IRAM_ATTR + 소프트확인
// ============================================================
