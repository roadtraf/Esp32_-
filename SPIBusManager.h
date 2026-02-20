// ================================================================
// SPIBusManager.h - SPI 버스 전역 충돌 방지 관리자
// v3.9.4 Hardened Edition
// ================================================================
// [가상 테스트 시나리오 #8]
// 문제:
//   - UITask: TFT 업데이트 (ILI9488, CS=10)
//   - UITask 내 handleTouch(): XPT2046 접근 (CS=14)
//   - DataLoggerTask: SD 접근 (CS=46)
//   - 모두 동일 SPI2_HOST 공유
//   - beginTransaction/endTransaction 누락 → 데이터 오염
//   - CS 동시 LOW 가능성
//
// 해결:
//   - 전역 FreeRTOS Mutex로 SPI 버스 직렬화
//   - RAII 스타일 SPI Guard 클래스
//   - CS 핀 자동 관리
//   - 타임아웃 기반 데드락 방지
// ================================================================
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "HardenedConfig.h"

// ================================================================
// SPI 디바이스 ID
// ================================================================
enum SPIDevice {
    SPI_DEV_TFT   = 0,     // ILI9488 디스플레이
    SPI_DEV_TOUCH = 1,     // XPT2046 터치
    SPI_DEV_SD    = 2,     // SD 카드
    SPI_DEV_NONE  = 255    // 미사용
};

// ================================================================
// SPI 버스 관리자 (싱글톤)
// ================================================================
class SPIBusManager {
public:
    // ── 싱글톤 ──
    static SPIBusManager& getInstance() {
        static SPIBusManager instance;
        return instance;
    }

    // ── 초기화 ──
    void begin() {
        if (_initialized) return;

        _mutex = xSemaphoreCreateMutex();
        if (!_mutex) {
            Serial.println("[SPIBus] ❌ Mutex 생성 실패!");
            return;
        }

        // CS 핀 초기화 (모두 HIGH = 비활성)
        pinMode(TFT_CS_PIN,    OUTPUT); digitalWrite(TFT_CS_PIN,    HIGH);
        pinMode(TOUCH_CS_PIN,  OUTPUT); digitalWrite(TOUCH_CS_PIN,  HIGH);
        pinMode(SD_CS_PIN_SPI, OUTPUT); digitalWrite(SD_CS_PIN_SPI, HIGH);

        _currentOwner = SPI_DEV_NONE;
        _initialized = true;

        Serial.println("[SPIBus] ✅ SPI 버스 관리자 초기화 완료");
    }

    // ── 버스 획득 (타임아웃 포함) ──
    bool acquire(SPIDevice device, uint32_t timeoutMs = SPI_MUTEX_TIMEOUT_MS) {
        if (!_initialized) return false;

        TickType_t timeout = pdMS_TO_TICKS(timeoutMs);
        if (xSemaphoreTake(_mutex, timeout) != pdTRUE) {
            Serial.printf("[SPIBus] ⚠️ 뮤텍스 타임아웃 (요청: %d, 점유: %d)\n",
                          device, _currentOwner);
            _timeoutCount++;
            return false;
        }

        _currentOwner = device;
        _lastAcquireTime = millis();
        return true;
    }

    // ── 버스 반환 ──
    void release(SPIDevice device) {
        if (!_initialized) return;
        if (_currentOwner != device) {
            Serial.printf("[SPIBus] ⚠️ 잘못된 release (소유: %d, 반환자: %d)\n",
                          _currentOwner, device);
            return;
        }

        // CS 핀 확실히 HIGH
        _deassertAllCS();
        _currentOwner = SPI_DEV_NONE;
        xSemaphoreGive(_mutex);
    }

    // ── 통계 ──
    uint32_t getTimeoutCount()  const { return _timeoutCount; }
    SPIDevice getCurrentOwner() const { return _currentOwner; }
    bool      isInitialized()   const { return _initialized; }

    // ── 진단 ──
    void printStats() const {
        Serial.printf("[SPIBus] 타임아웃 발생: %lu회, 현재 소유: %d\n",
                      _timeoutCount, _currentOwner);
    }

private:
    SPIBusManager() : _mutex(nullptr), _initialized(false),
                      _currentOwner(SPI_DEV_NONE), _timeoutCount(0),
                      _lastAcquireTime(0) {}
    ~SPIBusManager() = default;
    SPIBusManager(const SPIBusManager&) = delete;
    SPIBusManager& operator=(const SPIBusManager&) = delete;

    void _deassertAllCS() {
        digitalWrite(TFT_CS_PIN,    HIGH);
        digitalWrite(TOUCH_CS_PIN,  HIGH);
        digitalWrite(SD_CS_PIN_SPI, HIGH);
    }

    SemaphoreHandle_t _mutex;
    bool              _initialized;
    volatile SPIDevice _currentOwner;
    uint32_t          _timeoutCount;
    uint32_t          _lastAcquireTime;
};

// ================================================================
// RAII SPI Guard - 스코프 기반 자동 획득/반환
// ================================================================
// 사용 예:
//   {
//       SPIGuard guard(SPI_DEV_SD);
//       if (!guard.acquired()) return; // 획득 실패 시 안전하게 포기
//       File f = SD.open(...)
//   } // 스코프 종료 시 자동 반환
// ================================================================
class SPIGuard {
public:
    explicit SPIGuard(SPIDevice device,
                      uint32_t timeoutMs = SPI_MUTEX_TIMEOUT_MS)
        : _device(device), _acquired(false)
    {
        _acquired = SPIBusManager::getInstance().acquire(_device, timeoutMs);
    }

    ~SPIGuard() {
        if (_acquired) {
            SPIBusManager::getInstance().release(_device);
        }
    }

    bool acquired() const { return _acquired; }

    // 복사/이동 금지
    SPIGuard(const SPIGuard&)            = delete;
    SPIGuard& operator=(const SPIGuard&) = delete;

private:
    SPIDevice _device;
    bool      _acquired;
};

// ================================================================
// 편의 매크로
// ================================================================
#define SPI_BUS_BEGIN()     SPIBusManager::getInstance().begin()

// 함수 레벨 획득/반환 (반드시 쌍으로 사용)
#define SPI_ACQUIRE(dev)    SPIBusManager::getInstance().acquire(dev)
#define SPI_RELEASE(dev)    SPIBusManager::getInstance().release(dev)

// RAII 가드 (권장)
#define SPI_GUARD_TFT()     SPIGuard _spiGuard(SPI_DEV_TFT)
#define SPI_GUARD_TOUCH()   SPIGuard _spiGuard(SPI_DEV_TOUCH)
#define SPI_GUARD_SD()      SPIGuard _spiGuard(SPI_DEV_SD)

// 획득 실패 시 조기 반환
#define SPI_GUARD_OR_RETURN(dev) \
    SPIGuard _spiGuard_##dev(dev); \
    if (!_spiGuard_##dev.acquired()) return;

#define SPI_GUARD_OR_RETURN_VAL(dev, val) \
    SPIGuard _spiGuard_##dev(dev); \
    if (!_spiGuard_##dev.acquired()) return (val);
