// ================================================================
// SafeSensor.h - DS18B20 완전 비동기 + WDT 안전 센서 시스템
// v3.9.4 Hardened Edition
// ================================================================
// [가상 테스트 시나리오 #9]
//
// 기존 문제:
//   - requestTemperatures() → 12비트 변환 750ms 동안 OneWire 프로토콜
//     과정에서 인터럽트 일시 비활성화 → UI/제어 지연 발생
//   - 기존 코드는 setWaitForConversion(false) 설정했지만,
//     requestTemperatures() 자체가 최소 수 ms 블로킹
//   - SensorReadTask가 100ms 주기로 호출 시 WDT checkin 지연 가능
//   - 온도 센서 없을 때 readSensors()에서 무한대기 가능
//
// 해결:
//   - 전용 DS18B20Task (낮은 우선순위) 분리
//   - 상태 머신: REQUEST → WAIT_CONVERSION → READ → IDLE
//   - 결과를 atomic 변수로 공유 (SensorReadTask는 값만 읽음)
//   - 센서 부재 시 즉시 fallback 반환
//   - 변환 완료 전 읽기 시도 방지
// ================================================================
#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "HardenedConfig.h"
#include "EnhancedWatchdog.h"

// ================================================================
// DS18B20 상태 머신
// ================================================================
enum DS18B20State {
    DS18B20_IDLE,               // 대기
    DS18B20_REQUESTING,         // 변환 요청 중
    DS18B20_WAIT_CONVERSION,    // 변환 완료 대기 (750ms)
    DS18B20_READING,            // 값 읽기 중
    DS18B20_ERROR,              // 오류 상태
};

// ================================================================
// 비동기 온도 센서 관리자
// ================================================================
class SafeDS18B20 {
public:
    SafeDS18B20(uint8_t pin)
        : _ow(pin), _sensor(&_ow),
          _state(DS18B20_IDLE),
          _lastTemp(DS18B20_FALLBACK_TEMP),
          _tempValid(false),
          _sensorPresent(false),
          _requestTime(0),
          _errorCount(0),
          _readCount(0)
    {}

    // ── 초기화 ──
    bool begin() {
        Serial.println("[DS18B20] 초기화...");
        _sensor.begin();

        int count = _sensor.getDeviceCount();
        Serial.printf("[DS18B20] 감지된 센서: %d개\n", count);

        if (count == 0) {
            Serial.println("[DS18B20] ⚠️  센서 없음 - fallback 모드");
            _sensorPresent = false;
            return false;
        }

        // 해상도 설정 (12bit = 750ms, 9bit = 93.75ms)
        // WDT 안전을 위해 9bit 옵션도 고려:
        // 9bit: 0.5°C 분해능 → 93.75ms 변환 (WDT에 안전)
        // 12bit: 0.0625°C → 750ms (별도 태스크 필요)
        _sensor.setResolution(12);
        _sensor.setWaitForConversion(false);  // 반드시 비동기!

        _sensorPresent = true;
        _state = DS18B20_IDLE;

        Serial.println("[DS18B20] ✅ 초기화 완료");
        return true;
    }

    // ── 태스크 함수 (전용 태스크에서 호출) ──
    // FreeRTOS Task로 실행: xTaskCreate(ds18b20TaskFunc, ...)
    void runTask() {
        for (;;) {
            step();
            // 낮은 우선순위 태스크 - 다른 태스크에 CPU 양보
            vTaskDelay(pdMS_TO_TICKS(100));

            WDT_CHECKIN("DS18B20");
        }
    }

    // ── 상태 머신 단계 ──
    void step() {
        if (!_sensorPresent) {
            // 센서 없음: 주기적 재검색 (30초마다)
            static uint32_t lastSearchTime = 0;
            if (millis() - lastSearchTime > 30000) {
                lastSearchTime = millis();
                if (_sensor.getDeviceCount() > 0) {
                    Serial.println("[DS18B20] 센서 재감지!");
                    begin();
                }
            }
            return;
        }

        uint32_t now = millis();

        switch (_state) {
            case DS18B20_IDLE:
                // 주기적으로 변환 요청 (1초마다)
                static uint32_t lastRequest = 0;
                if (now - lastRequest >= 1000) {
                    lastRequest = now;
                    _requestConversion();
                }
                break;

            case DS18B20_WAIT_CONVERSION:
                // 변환 완료 대기
                if (now - _requestTime >= DS18B20_CONVERSION_TIME_MS) {
                    _readTemperature();
                }
                break;

            case DS18B20_ERROR:
                // 오류: 2초 후 재시도
                if (now - _requestTime >= 2000) {
                    Serial.printf("[DS18B20] 오류 복구 시도 (총 %lu회)\n", _errorCount);
                    _sensor.begin();  // 재초기화
                    _state = DS18B20_IDLE;
                }
                break;

            default:
                _state = DS18B20_IDLE;
                break;
        }
    }

    // ── 온도 값 읽기 (Non-blocking, 항상 즉시 반환) ──
    // SensorReadTask에서 이 함수만 호출 → 블로킹 없음
    float getTemperature() const {
        return _lastTemp;
    }

    bool isValid()   const { return _tempValid && _sensorPresent; }
    bool isPresent() const { return _sensorPresent; }

    // ── 통계 ──
    uint32_t getErrorCount() const { return _errorCount; }
    uint32_t getReadCount()  const { return _readCount; }
    DS18B20State getState()  const { return _state; }

    void printStatus() const {
        Serial.printf("[DS18B20] 온도: %.2f°C | 유효: %s | 오류: %lu회\n",
                      _lastTemp, _tempValid ? "✓" : "✗", _errorCount);
    }

private:
    void _requestConversion() {
        _state = DS18B20_REQUESTING;
        _sensor.requestTemperatures();
        _requestTime = millis();
        _state = DS18B20_WAIT_CONVERSION;
    }

    void _readTemperature() {
        _state = DS18B20_READING;

        // isConversionComplete() 확인 (라이브러리 지원 시)
        float temp = _sensor.getTempCByIndex(0);

        if (temp == DEVICE_DISCONNECTED_C ||
            temp == DEVICE_DISCONNECTED_RAW ||
            temp < -55.0f || temp > 125.0f)
        {
            Serial.printf("[DS18B20] ⚠️  비정상값: %.2f°C\n", temp);
            _errorCount++;

            // 3회 연속 오류 시 재초기화
            if (_errorCount % 3 == 0) {
                _sensorPresent = (_sensor.getDeviceCount() > 0);
                if (!_sensorPresent) {
                    Serial.println("[DS18B20] ❌ 센서 연결 끊김");
                }
            }
            _state = DS18B20_ERROR;
            return;
        }

        // 급격한 변화 필터링 (노이즈 방지)
        if (_tempValid && abs(temp - _lastTemp) > 10.0f) {
            Serial.printf("[DS18B20] ⚠️  급격한 변화 무시: %.2f→%.2f\n",
                          _lastTemp, temp);
            _state = DS18B20_IDLE;
            return;
        }

        _lastTemp  = temp;
        _tempValid = true;
        _readCount++;
        _state = DS18B20_IDLE;
    }

    OneWire         _ow;
    DallasTemperature _sensor;
    volatile DS18B20State _state;
    volatile float  _lastTemp;
    volatile bool   _tempValid;
    volatile bool   _sensorPresent;
    uint32_t        _requestTime;
    uint32_t        _errorCount;
    uint32_t        _readCount;
};

// ================================================================
// 전역 인스턴스 선언 (정의는 SafeSensor.cpp에서)
// ================================================================
extern SafeDS18B20 safeDS18B20;

// ================================================================
// DS18B20 전용 FreeRTOS 태스크 함수
// ================================================================
void ds18b20Task(void* param);
