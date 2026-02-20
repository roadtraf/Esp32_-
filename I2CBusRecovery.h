// ================================================================
// I2CBusRecovery.h - I2C 버스 복구 & 안전 접근 시스템
// v3.9.4 Hardened Edition
// ================================================================
// [가상 테스트 시나리오 #5]
//
// 재현된 장애 모드:
//   A) 센서 전원 순간 변동 → I2C ACK 미수신 → Wire 상태 불량
//   B) 클럭 라인 노이즈 → SCL 카운트 어긋남 → 프로토콜 데동기화
//   C) 장치가 SDA를 LOW로 잡은 채 멈춤 (read 중 reset된 경우)
//      → Wire.begin() 재시도해도 SDA LOW 유지 → 영구 데드락
//
// 해결책:
//   - I2C Bus Recovery (SMBus 2.0 §4.3.2 기반)
//     : SCL 9클럭 토글 → 슬레이브 state machine 리셋
//     : STOP condition 강제 발생
//   - Wire.begin() 재초기화
//   - 타임아웃 기반 endTransmission
//   - 전원 변동 대응: 재시도 전 안정화 대기
// ================================================================
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "HardenedConfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ================================================================
// I2C 오류 코드
// ================================================================
enum I2CError {
    I2C_OK              = 0,
    I2C_ERR_TIMEOUT     = 1,   // 타임아웃
    I2C_ERR_NACK_ADDR   = 2,   // 주소 NACK
    I2C_ERR_NACK_DATA   = 3,   // 데이터 NACK
    I2C_ERR_BUS_BUSY    = 4,   // 버스 사용 중
    I2C_ERR_RECOVERY    = 5,   // 복구 시도 중
    I2C_ERR_FATAL       = 6,   // 복구 불가
};

// I2C endTransmission 반환값
// 0=success, 1=data too long, 2=NACK addr, 3=NACK data, 4=other
static const char* i2cErrorStr(uint8_t err) {
    switch (err) {
        case 0: return "OK";
        case 1: return "DATA_TOO_LONG";
        case 2: return "NACK_ADDR";
        case 3: return "NACK_DATA";
        case 4: return "OTHER";
        default: return "UNKNOWN";
    }
}

// ================================================================
// I2C Bus Recovery 관리자
// ================================================================
class I2CBusRecovery {
public:
    static I2CBusRecovery& getInstance() {
        static I2CBusRecovery instance;
        return instance;
    }

    // ── 초기화 ──
    void begin(uint8_t sdaPin = I2C_SDA_PIN,
               uint8_t sclPin = I2C_SCL_PIN,
               uint32_t freq  = I2C_FREQ_HZ)
    {
        _sdaPin = sdaPin;
        _sclPin = sclPin;
        _freq   = freq;

        _mutex = xSemaphoreCreateMutex();
        if (!_mutex) {
            Serial.println("[I2C] ❌ Mutex 생성 실패!");
        }

        // Wire 초기화
        Wire.begin(_sdaPin, _sclPin, _freq);
        Wire.setTimeOut(I2C_TIMEOUT_MS);

        _recoveryCount = 0;
        _initialized   = true;

        // 초기 버스 상태 확인
        if (!isBusHealthy()) {
            Serial.println("[I2C] ⚠️  초기 버스 상태 불량 → 복구 시도");
            recover();
        } else {
            Serial.println("[I2C] ✅ 버스 정상");
        }
    }

    // ── 버스 건강 확인 ──
    // SDA/SCL 모두 HIGH여야 정상 (idle 상태)
    bool isBusHealthy() {
        // 일시적으로 GPIO 입력으로 읽기
        pinMode(_sdaPin, INPUT_PULLUP);
        pinMode(_sclPin, INPUT_PULLUP);
        delayMicroseconds(10);

        bool sdaHigh = digitalRead(_sdaPin);
        bool sclHigh = digitalRead(_sclPin);

        // Wire 재설정
        Wire.begin(_sdaPin, _sclPin, _freq);

        if (!sdaHigh) {
            Serial.printf("[I2C] ⚠️  SDA LOW 감지 (SCL=%d)\n", sclHigh);
            return false;
        }
        if (!sclHigh) {
            Serial.println("[I2C] ⚠️  SCL LOW 감지 (다른 장치가 클럭 잡음)");
            return false;
        }
        return true;
    }

    // ── 버스 복구 (SMBus 2.0 §4.3.2) ──
    // 슬레이브가 SDA를 LOW로 잡고 있을 때 강제 해제
    bool recover() {
        Serial.println("[I2C] === 버스 복구 시작 ===");
        _recoveryCount++;

        // Step 1: Wire 해제
        Wire.end();
        delayMicroseconds(100);

        // Step 2: GPIO 직접 제어로 전환
        pinMode(_sdaPin, OUTPUT);
        pinMode(_sclPin, OUTPUT);
        digitalWrite(_sdaPin, HIGH);
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(I2C_RECOVER_DELAY_US * 10);

        // Step 3: SCL 9클럭 토글
        // 슬레이브 state machine을 어떤 상태에 있더라도 리셋
        Serial.println("[I2C] SCL 9클럭 토글 중...");
        for (int i = 0; i < I2C_RECOVER_CLOCK_COUNT; i++) {
            // SDA 상태 확인 (중간에 HIGH가 되면 슬레이브가 해제된 것)
            pinMode(_sdaPin, INPUT_PULLUP);
            delayMicroseconds(I2C_RECOVER_DELAY_US);

            bool sdaReleased = digitalRead(_sdaPin);

            pinMode(_sclPin, OUTPUT);
            digitalWrite(_sclPin, LOW);
            delayMicroseconds(I2C_RECOVER_DELAY_US);
            digitalWrite(_sclPin, HIGH);
            delayMicroseconds(I2C_RECOVER_DELAY_US);

            if (sdaReleased) {
                Serial.printf("[I2C] SDA 해제 확인 (%d번째 클럭)\n", i + 1);
                break;
            }
        }

        // Step 4: STOP condition 강제 생성
        // SDA LOW → SCL HIGH → SDA HIGH
        pinMode(_sdaPin, OUTPUT);
        digitalWrite(_sdaPin, LOW);
        delayMicroseconds(I2C_RECOVER_DELAY_US);
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(I2C_RECOVER_DELAY_US);
        digitalWrite(_sdaPin, HIGH);
        delayMicroseconds(I2C_RECOVER_DELAY_US);

        // Step 5: Wire 재초기화
        Wire.begin(_sdaPin, _sclPin, _freq);
        Wire.setTimeOut(I2C_TIMEOUT_MS);
        delay(I2C_SENSOR_WARMUP_MS);  // 센서 안정화 대기

        // Step 6: 복구 결과 확인
        bool healthy = isBusHealthy();

        if (healthy) {
            Serial.println("[I2C] ✅ 복구 성공");
        } else {
            Serial.println("[I2C] ❌ 복구 실패 - 하드웨어 점검 필요");
        }

        return healthy;
    }

    // ── 안전한 쓰기 (재시도 포함) ──
    I2CError safeWrite(uint8_t addr, const uint8_t* data, size_t len,
                       bool sendStop = true)
    {
        if (!_initialized) return I2C_ERR_FATAL;

        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
            return I2C_ERR_BUS_BUSY;
        }

        I2CError result = I2C_ERR_FATAL;

        for (int attempt = 0; attempt < I2C_MAX_RETRY; attempt++) {
            Wire.beginTransmission(addr);
            Wire.write(data, len);
            uint8_t err = Wire.endTransmission(sendStop);

            if (err == 0) {
                result = I2C_OK;
                break;
            }

            Serial.printf("[I2C] 쓰기 오류 addr=0x%02X err=%s (시도 %d/%d)\n",
                          addr, i2cErrorStr(err), attempt + 1, I2C_MAX_RETRY);

            // NACK 발생 시 버스 복구 시도
            if (err == 2 || err == 3 || err == 4) {
                xSemaphoreGive(_mutex);
                delay(I2C_SENSOR_WARMUP_MS);
                recover();
                if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
                    return I2C_ERR_BUS_BUSY;
                }
            } else {
                delay(10);
            }

            result = (err == 2 || err == 3) ? I2C_ERR_NACK_DATA : I2C_ERR_TIMEOUT;
        }

        xSemaphoreGive(_mutex);
        return result;
    }

    // ── 안전한 읽기 ──
    I2CError safeRead(uint8_t addr, uint8_t* buffer, size_t len)
    {
        if (!_initialized) return I2C_ERR_FATAL;

        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
            return I2C_ERR_BUS_BUSY;
        }

        I2CError result = I2C_ERR_FATAL;

        for (int attempt = 0; attempt < I2C_MAX_RETRY; attempt++) {
            size_t received = Wire.requestFrom(addr, len);

            if (received == len) {
                for (size_t i = 0; i < len; i++) {
                    buffer[i] = Wire.read();
                }
                result = I2C_OK;
                break;
            }

            Serial.printf("[I2C] 읽기 오류 addr=0x%02X recv=%d (시도 %d/%d)\n",
                          addr, received, attempt + 1, I2C_MAX_RETRY);

            xSemaphoreGive(_mutex);
            delay(10);
            if (received == 0) recover();
            if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
                return I2C_ERR_BUS_BUSY;
            }

            result = I2C_ERR_TIMEOUT;
        }

        xSemaphoreGive(_mutex);
        return result;
    }

    // ── 디바이스 스캔 ──
    void scan() {
        Serial.println("[I2C] === 디바이스 스캔 ===");
        int found = 0;
        for (uint8_t addr = 8; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("[I2C] 발견: 0x%02X\n", addr);
                found++;
            }
        }
        Serial.printf("[I2C] 총 %d개 발견\n", found);
    }

    // ── 통계 ──
    uint32_t getRecoveryCount() const { return _recoveryCount; }

private:
    I2CBusRecovery() : _sdaPin(I2C_SDA_PIN), _sclPin(I2C_SCL_PIN),
                       _freq(I2C_FREQ_HZ), _mutex(nullptr),
                       _initialized(false), _recoveryCount(0) {}

    uint8_t           _sdaPin;
    uint8_t           _sclPin;
    uint32_t          _freq;
    SemaphoreHandle_t _mutex;
    bool              _initialized;
    uint32_t          _recoveryCount;
};

// ================================================================
// 편의 매크로
// ================================================================
#define I2C_BUS     I2CBusRecovery::getInstance()
#define I2C_RECOVER() I2CBusRecovery::getInstance().recover()
#define I2C_HEALTHY() I2CBusRecovery::getInstance().isBusHealthy()
