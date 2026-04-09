// ================================================================
// SPIBusManager.h - SPI     
// v3.9.4 Hardened Edition
// ================================================================
// [   #8]
// :
//   - UITask: TFT  (ILI9488, CS=10)
//   - UITask  handleTouch(): XPT2046  (CS=14)
//   - DataLoggerTask: SD  (CS=46)
//   -   SPI2_HOST 
//   - beginTransaction/endTransaction    
//   - CS  LOW 
//
// :
//   -  FreeRTOS Mutex SPI  
//   - RAII  SPI Guard 
//   - CS   
//   -    
// ================================================================
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "HardenedConfig.h"

// ================================================================
// SPI  ID
// ================================================================
enum SPIDevice {
    SPI_DEV_TFT   = 0,     // ILI9488 
    SPI_DEV_TOUCH = 1,     // XPT2046 
    SPI_DEV_SD    = 2,     // SD 
    SPI_DEV_NONE  = 255    // 
};

// ================================================================
// SPI   ()
// ================================================================
class SPIBusManager {
public:
    //   
    static SPIBusManager& getInstance() {
        static SPIBusManager instance;
        return instance;
    }

    //   
    void begin() {
        if (_initialized) return;

        _mutex = xSemaphoreCreateMutex();
        if (!_mutex) {
            Serial.println("[SPIBus]  Mutex  !");
            return;
        }

        // CS   ( HIGH = )
        pinMode(TFT_CS_PIN,    OUTPUT); digitalWrite(TFT_CS_PIN,    HIGH);
        pinMode(TOUCH_CS_PIN,  OUTPUT); digitalWrite(TOUCH_CS_PIN,  HIGH);
        pinMode(SD_CS_PIN_SPI, OUTPUT); digitalWrite(SD_CS_PIN_SPI, HIGH);

        _currentOwner = SPI_DEV_NONE;
        _initialized = true;

        Serial.println("[SPIBus]  SPI    ");
    }

    //    ( ) 
    bool acquire(SPIDevice device, uint32_t timeoutMs = SPI_MUTEX_TIMEOUT_MS) {
        if (!_initialized) return false;

        TickType_t timeout = pdMS_TO_TICKS(timeoutMs);
        if (xSemaphoreTake(_mutex, timeout) != pdTRUE) {
            Serial.printf("[SPIBus]    (: %d, : %d)\n",
                          device, _currentOwner);
            _timeoutCount++;
            return false;
        }

        _currentOwner = device;
        _lastAcquireTime = millis();
        return true;
    }

    //    
    void release(SPIDevice device) {
        if (!_initialized) return;
        if (_currentOwner != device) {
            Serial.printf("[SPIBus]   release (: %d, : %d)\n",
                          _currentOwner, device);
            return;
        }

        // CS   HIGH
        _deassertAllCS();
        _currentOwner = SPI_DEV_NONE;
        xSemaphoreGive(_mutex);
    }

    //   
    uint32_t getTimeoutCount()  const { return _timeoutCount; }
    SPIDevice getCurrentOwner() const { return _currentOwner; }
    bool      isInitialized()   const { return _initialized; }

    //   
    void printStats() const {
        Serial.printf("[SPIBus]  : %lu,  : %d\n",
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
// RAII SPI Guard -    /
// ================================================================
//  :
//   {
//       SPIGuard guard(SPI_DEV_SD);
//       if (!guard.acquired()) return; //     
//       File f = SD.open(...)
//   } //     
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

    // / 
    SPIGuard(const SPIGuard&)            = delete;
    SPIGuard& operator=(const SPIGuard&) = delete;

private:
    SPIDevice _device;
    bool      _acquired;
};

// ================================================================
//  
// ================================================================
#define SPI_BUS_BEGIN()     SPIBusManager::getInstance().begin()

//   / (  )
#define SPI_ACQUIRE(dev)    SPIBusManager::getInstance().acquire(dev)
#define SPI_RELEASE(dev)    SPIBusManager::getInstance().release(dev)

// RAII  ()
#define SPI_GUARD_TFT()     SPIGuard _spiGuard(SPI_DEV_TFT)
#define SPI_GUARD_TOUCH()   SPIGuard _spiGuard(SPI_DEV_TOUCH)
#define SPI_GUARD_SD()      SPIGuard _spiGuard(SPI_DEV_SD)

//     
#define SPI_GUARD_OR_RETURN(dev) \
    SPIGuard _spiGuard_##dev(dev); \
    if (!_spiGuard_##dev.acquired()) return;

#define SPI_GUARD_OR_RETURN_VAL(dev, val) \
    SPIGuard _spiGuard_##dev(dev); \
    if (!_spiGuard_##dev.acquired()) return (val);
