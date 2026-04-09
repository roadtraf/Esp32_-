// ================================================================
// SafeSD.h - SD    ( + SPI )
// v3.9.4 Hardened Edition
// ================================================================
// [   #4 + #8]
//
//  :
//   - SD.open(path, FILE_APPEND): SPI    
//   - file.close() : SD    flush   500ms
//   - UI Task TFT   DataLoggerTask SD 
//      SPI CS      SD  corruption
//   - sdReady  SPI   
//
// :
//   - SPIBusManager  SD  
//   - SD.open()  (FreeRTOS EventBit )
//   -     
//   -  SD   WDT feed 
// ================================================================
#pragma once

#include <Arduino.h>
#include <SD.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "HardenedConfig.h"
#include "SPIBusManager.h"
#include "EnhancedWatchdog.h"

// ================================================================
//  SD  RAII 
// ================================================================
//  :
//   SafeSDFile f("/logs/data.csv", FILE_APPEND);
//   if (!f.isOpen()) return;
//   f->println("data");
//   //     close + SPI  
// ================================================================
class SafeSDFile {
public:
    SafeSDFile(const char* path, const char* mode = FILE_APPEND,
               uint32_t timeoutMs = SD_OPEN_TIMEOUT_MS)
        : _guard(SPI_DEV_SD, timeoutMs), _opened(false)
    {
        if (!_guard.acquired()) {
            Serial.printf("[SafeSD] SPI   : %s\n", path);
            return;
        }

        if (!_sdReady) {
            return;
        }

        // SD.open()   
        //      (ESP32 SD  )
        //  SPI   ,  WDT 
        WDT_FEED();  // SD.open()  WDT feed

        _file = SD.open(path, mode);
        _opened = _file ? true : false;

        if (!_opened) {
            Serial.printf("[SafeSD]   : %s\n", path);
        }
    }

    ~SafeSDFile() {
        if (_opened && _file) {
            WDT_FEED();   // flush  WDT reset 
            _file.close();
            _opened = false;
        }
        // _guard  SPI   
    }

    bool isOpen() const { return _opened; }

    //   (File*  )
    File* operator->() { return &_file; }
    File& get()        { return _file; }

    // SD    
    static bool _sdReady;

private:
    SPIGuard _guard;
    File     _file;
    bool     _opened;
};

// ================================================================
//  SD 
// ================================================================
class SafeSDManager {
public:
    static SafeSDManager& getInstance() {
        static SafeSDManager instance;
        return instance;
    }

    //   
    bool begin(uint8_t csPin = SD_CS_PIN) {
        SPIGuard guard(SPI_DEV_SD, 2000);
        if (!guard.acquired()) {
            Serial.println("[SafeSD]  SPI  ");
            return false;
        }

        WDT_FEED();

        if (!SD.begin(csPin)) {
            Serial.println("[SafeSD]  SD   ");
            SafeSDFile::_sdReady = false;
            return false;
        }

        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("[SafeSD]  SD  ");
            SafeSDFile::_sdReady = false;
            return false;
        }

        //  
        _ensureDir("/logs");
        _ensureDir("/reports");

        SafeSDFile::_sdReady = true;
        _initTime = millis();

        Serial.printf("[SafeSD]  SD   (: %s, : %lluMB)\n",
            cardType == CARD_MMC  ? "MMC"  :
            cardType == CARD_SD   ? "SDSC" :
            cardType == CARD_SDHC ? "SDHC" : "?",
            SD.cardSize() / (1024ULL * 1024ULL));

        return true;
    }

    bool isReady() const { return SafeSDFile::_sdReady; }

    //     ( ) 
    bool safeAppend(const char* path, const char* data,
                    uint8_t maxRetry = SD_MAX_RETRY_COUNT)
    {
        if (!SafeSDFile::_sdReady) return false;

        for (uint8_t attempt = 0; attempt < maxRetry; attempt++) {
            SafeSDFile f(path, FILE_APPEND);

            if (!f.isOpen()) {
                Serial.printf("[SafeSD]   ( %d/%d)\n",
                              attempt + 1, maxRetry);
                vTaskDelay(pdMS_TO_TICKS(SD_RETRY_DELAY_MS));
                continue;
            }

            WDT_FEED();
            f->println(data);
            //  close + WDT feed
            return true;
        }

        _writeFailCount++;
        Serial.printf("[SafeSD]    : %s\n", path);
        return false;
    }

    //     
    bool exists(const char* path) {
        SPIGuard guard(SPI_DEV_SD, SD_OPEN_TIMEOUT_MS);
        if (!guard.acquired() || !SafeSDFile::_sdReady) return false;
        return SD.exists(path);
    }

    //   
    uint32_t getWriteFailCount() const { return _writeFailCount; }

    void printStats() const {
        Serial.printf("[SafeSD]  : %lu |  : %lus\n",
                      _writeFailCount,
                      (millis() - _initTime) / 1000);
    }

private:
    SafeSDManager() : _writeFailCount(0), _initTime(0) {}

    bool _ensureDir(const char* path) {
        if (!SD.exists(path)) {
            return SD.mkdir(path);
        }
        return true;
    }

    uint32_t _writeFailCount;
    uint32_t _initTime;
};

// ================================================================
//  
// ================================================================
#define SAFE_SD     SafeSDManager::getInstance()

//    append
#define SD_SAFE_APPEND(path, data) \
    SafeSDManager::getInstance().safeAppend(path, data)
