// ================================================================
// SharedState.h - 멀티태스크 공유 상태 안전 접근
// v3.9.4 Hardened Edition
// ================================================================
// [A] sensorData / stats / currentState 동시접근 보호
// [B] ledcWrite PWM 채널 경쟁 보호
// [F] MQTT→StateChange Queue 기반 안전 전달
// [J] volatile 선언으로 컴파일러 캐싱 방지
// ================================================================
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "AdditionalHardening.h"
// Config.h의 구조체 의존
#include "Config.h"

// ================================================================
// [J] volatile 선언 - 멀티코어 가시성 보장
// ================================================================
// 기존 Config.h의 extern 선언들은 volatile이 없어
// 컴파일러가 레지스터에 캐싱 → 타 코어의 변경값 미반영
// 여기서는 전역 변수 접근용 래퍼로 해결 (하위호환 유지)

// ================================================================
// [A] 공유 데이터 보호 관리자
// ================================================================
class SharedStateManager {
public:
    static SharedStateManager& getInstance() {
        static SharedStateManager instance;
        return instance;
    }

    void begin() {
        _sensorMutex = xSemaphoreCreateMutex();
        _statsMutex  = xSemaphoreCreateMutex();
        _stateMutex  = xSemaphoreCreateMutex();
        _pwmMutex    = xSemaphoreCreateMutex();
        _nvsMutex    = xSemaphoreCreateMutex();
        _adcMutex    = xSemaphoreCreateMutex();

        // [F] MQTT 명령 큐
        _mqttCmdQueue = xQueueCreate(MQTT_CMD_QUEUE_SIZE, sizeof(MQTTCommand));

        if (!_sensorMutex || !_statsMutex || !_stateMutex ||
            !_pwmMutex    || !_nvsMutex   || !_adcMutex   ||
            !_mqttCmdQueue) {
            Serial.println("[SharedState] ❌ 초기화 실패! 시스템 정지");
            esp_restart();
        }
        Serial.println("[SharedState] ✅ 공유 상태 보호 초기화 완료");
    }

    // ── [A] SensorData 안전 읽기/쓰기 ──
    SensorData readSensorData() {
        SensorData snapshot;
        if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            snapshot = *_pSensorData;
            xSemaphoreGive(_sensorMutex);
        }
        return snapshot;
    }

    void writeSensorData(const SensorData& data) {
        if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            *_pSensorData = data;
            xSemaphoreGive(_sensorMutex);
        }
    }

    void updateSensorField(float pressure, float current, float temperature) {
        if (xSemaphoreTake(_sensorMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            _pSensorData->pressure    = pressure;
            _pSensorData->current     = current;
            _pSensorData->temperature = temperature;
            _pSensorData->timestamp   = millis();
            xSemaphoreGive(_sensorMutex);
        }
    }

    // 센서 데이터 포인터 등록 (main에서 호출)
    void setSensorDataPtr(SensorData* ptr) { _pSensorData = ptr; }

    // ── [A] Statistics 안전 접근 ──
    void setStatsPtr(Statistics* ptr) { _pStats = ptr; }

    Statistics readStats() {
        Statistics s;
        if (_pStats && xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            s = *_pStats;
            xSemaphoreGive(_statsMutex);
        }
        return s;
    }

    void incrementCycles(bool success) {
        if (!_pStats) return;
        if (xSemaphoreTake(_statsMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            _pStats->totalCycles++;
            if (success) _pStats->successfulCycles++;
            else         _pStats->failedCycles++;
            xSemaphoreGive(_statsMutex);
        }
    }

    // ── [A] SystemState 안전 읽기/쓰기 ──
    void setStatePtr(SystemState* ptr) { _pState = ptr; }

    SystemState readState() {
        SystemState s = STATE_IDLE;
        if (_pState && xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            s = *_pState;
            xSemaphoreGive(_stateMutex);
        }
        return s;
    }

    // !! 주의: 상태 변경은 VacuumCtrl Task 컨텍스트에서만!
    // 타 태스크는 반드시 sendStateChangeCmd() 사용
    void writeState(SystemState newState) {
        if (_pState && xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            *_pState = newState;
            xSemaphoreGive(_stateMutex);
        }
    }

    // ── [B] PWM 안전 쓰기 ──
    // Control.cpp, ControlManager.cpp, UIManager.cpp 모두 이 함수 사용
    bool safeLedcWrite(uint8_t channel, uint32_t duty) {
        if (xSemaphoreTake(_pwmMutex, pdMS_TO_TICKS(PWM_MUTEX_TIMEOUT_MS)) != pdTRUE) {
            Serial.printf("[PWM] 뮤텍스 타임아웃 (ch=%d)\n", channel);
            return false;
        }
        ledcWrite(channel, duty);
        xSemaphoreGive(_pwmMutex);
        return true;
    }

    // ── [C] NVS(Preferences) 안전 접근 ──
    bool acquireNVS() {
        return xSemaphoreTake(_nvsMutex, pdMS_TO_TICKS(NVS_MUTEX_TIMEOUT_MS)) == pdTRUE;
    }
    void releaseNVS() { xSemaphoreGive(_nvsMutex); }

    // ── [H] ADC 안전 읽기 (오버샘플링 포함) ──
    int safeAnalogRead(uint8_t pin) {
        if (xSemaphoreTake(_adcMutex, pdMS_TO_TICKS(ADC_MUTEX_TIMEOUT_MS)) != pdTRUE) {
            return -1;
        }

        // 오버샘플링으로 노이즈 제거
        int32_t sum = 0;
        int validCount = 0;
        int samples[ADC_OVERSAMPLE_COUNT];

        for (int i = 0; i < ADC_OVERSAMPLE_COUNT; i++) {
            samples[i] = analogRead(pin);
        }

        // 평균 계산
        for (int i = 0; i < ADC_OVERSAMPLE_COUNT; i++) sum += samples[i];
        float avg = (float)sum / ADC_OVERSAMPLE_COUNT;

        // 이상값 제거 (평균 ±15% 초과 샘플 제외)
        sum = 0; validCount = 0;
        for (int i = 0; i < ADC_OVERSAMPLE_COUNT; i++) {
            if (abs(samples[i] - avg) / avg < ADC_REJECT_THRESHOLD) {
                sum += samples[i];
                validCount++;
            }
        }

        xSemaphoreGive(_adcMutex);
        return (validCount > 0) ? (sum / validCount) : (int)avg;
    }

    // ── [F] MQTT 명령 큐 ──
    // [사용법]
    //   보내는 쪽 (MQTT callback, Core0):  sharedState.sendStateCmd(STATE_VACUUM_ON)
    //   받는 쪽  (VacuumCtrl Task, Core1): sharedState.receivePendingCmd(cmd)
    enum MQTTCmdType {
        CMD_NONE,
        CMD_STATE_CHANGE,
        CMD_EMERGENCY_STOP,
        CMD_RESET_ERROR,
        CMD_SET_MODE,
    };

    struct MQTTCommand {
        MQTTCmdType type;
        uint32_t    param;   // 상태값 또는 모드값
    };

    bool sendStateCmd(SystemState newState) {
        MQTTCommand cmd = { CMD_STATE_CHANGE, (uint32_t)newState };
        return xQueueSend(_mqttCmdQueue, &cmd, pdMS_TO_TICKS(MQTT_CMD_TIMEOUT_MS)) == pdTRUE;
    }

    bool sendEmergencyStop() {
        MQTTCommand cmd = { CMD_EMERGENCY_STOP, 0 };
        return xQueueSend(_mqttCmdQueue, &cmd, 0) == pdTRUE;  // 긴급: 타임아웃 없음
    }

    bool receivePendingCmd(MQTTCommand& cmd) {
        return xQueueReceive(_mqttCmdQueue, &cmd, 0) == pdTRUE;
    }

    uint32_t getPendingCmdCount() {
        return uxQueueMessagesWaiting(_mqttCmdQueue);
    }

private:
    SharedStateManager()
        : _sensorMutex(nullptr), _statsMutex(nullptr), _stateMutex(nullptr),
          _pwmMutex(nullptr), _nvsMutex(nullptr), _adcMutex(nullptr),
          _mqttCmdQueue(nullptr),
          _pSensorData(nullptr), _pStats(nullptr), _pState(nullptr) {}

    SemaphoreHandle_t _sensorMutex;
    SemaphoreHandle_t _statsMutex;
    SemaphoreHandle_t _stateMutex;
    SemaphoreHandle_t _pwmMutex;
    SemaphoreHandle_t _nvsMutex;
    SemaphoreHandle_t _adcMutex;
    QueueHandle_t     _mqttCmdQueue;

    SensorData*   _pSensorData;
    Statistics*   _pStats;
    SystemState*  _pState;
};

// ================================================================
// 편의 매크로
// ================================================================
#define SHARED_STATE    SharedStateManager::getInstance()

// PWM 안전 쓰기
#define SAFE_PWM_WRITE(ch, duty) \
    SharedStateManager::getInstance().safeLedcWrite(ch, duty)

// NVS RAII 가드
class NVSGuard {
public:
    NVSGuard()  : _acquired(SharedStateManager::getInstance().acquireNVS()) {}
    ~NVSGuard() { if (_acquired) SharedStateManager::getInstance().releaseNVS(); }
    bool acquired() const { return _acquired; }
private:
    bool _acquired;
};

// ================================================================
// [L] 비상정지 디바운스 처리기
// ================================================================
class EStopDebouncer {
public:
    EStopDebouncer() : _confirmCount(0), _lastRaw(false), _confirmed(false),
                       _lastChangeTime(0) {}

    // SensorTask에서 주기적 호출 (100ms 주기 가정)
    bool update(bool rawSignal) {
        uint32_t now = millis();

        if (rawSignal != _lastRaw) {
            _lastRaw = rawSignal;
            _lastChangeTime = now;
            _confirmCount = 0;
        }

        // 신호가 ESTOP_DEBOUNCE_MS 동안 안정적이면
        if (now - _lastChangeTime >= ESTOP_DEBOUNCE_MS) {
            if (_lastRaw != _confirmed) {
                _confirmCount++;
                if (_confirmCount >= ESTOP_CONFIRM_COUNT) {
                    _confirmed = _lastRaw;
                    _confirmCount = 0;
                    if (_confirmed) {
                        Serial.println("[EStop] ⚠️  비상정지 확정 (디바운스 완료)");
                    }
                }
            }
        }

        return _confirmed;
    }

    bool isActive() const { return _confirmed; }
    void reset()          { _confirmed = false; _confirmCount = 0; }

private:
    int      _confirmCount;
    bool     _lastRaw;
    bool     _confirmed;
    uint32_t _lastChangeTime;
};

extern EStopDebouncer eStopDebouncer;

// ================================================================
// [D] Serial 안전 출력 (멀티태스크)
// ================================================================
class SafeSerial {
public:
    static void begin() {
        _mutex = xSemaphoreCreateMutex();
    }

    static void printf(const char* fmt, ...) {
        if (!_mutex) return;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) != pdTRUE) return;

        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        Serial.print(buf);
        xSemaphoreGive(_mutex);
    }

    static void println(const char* msg) {
        if (!_mutex) return;
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) != pdTRUE) return;
        Serial.println(msg);
        xSemaphoreGive(_mutex);
    }

private:
    static SemaphoreHandle_t _mutex;
};
