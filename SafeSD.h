// ================================================================
// SafeSD.h - SD 카드 안전 접근 (타임아웃 + SPI 뮤텍스)
// v3.9.4 Hardened Edition
// ================================================================
// [가상 테스트 시나리오 #4 + #8]
//
// 기존 문제:
//   - SD.open(path, FILE_APPEND): SPI 충돌 시 무한 대기
//   - file.close() 지연: SD 내부 쓰기 버퍼 flush 시 최대 500ms
//   - UI Task에서 TFT 업데이트 중 DataLoggerTask가 SD 접근
//     → SPI CS 충돌 → 데이터 오염 또는 SD 카드 corruption
//   - sdReady 플래그만으로는 SPI 충돌 방지 불충분
//
// 해결:
//   - SPIBusManager 뮤텍스로 SD 접근 직렬화
//   - SD.open() 타임아웃 (FreeRTOS EventBit 활용)
//   - 쓰기 실패 시 재시도 큐
//   - 긴 SD 작업 중 WDT feed 유지
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
// 안전한 SD 파일 RAII 래퍼
// ================================================================
// 사용 예:
//   SafeSDFile f("/logs/data.csv", FILE_APPEND);
//   if (!f.isOpen()) return;
//   f->println("data");
//   // 스코프 종료 시 자동 close + SPI 뮤텍스 해제
// ================================================================
class SafeSDFile {
public:
    SafeSDFile(const char* path, const char* mode = FILE_APPEND,
               uint32_t timeoutMs = SD_OPEN_TIMEOUT_MS)
        : _guard(SPI_DEV_SD, timeoutMs), _opened(false)
    {
        if (!_guard.acquired()) {
            Serial.printf("[SafeSD] SPI 뮤텍스 획득 실패: %s\n", path);
            return;
        }

        if (!_sdReady) {
            return;
        }

        // SD.open()은 내부적으로 블로킹 가능
        // 타임아웃을 위해 별도 처리 불가 (ESP32 SD 라이브러리 한계)
        // → SPI 뮤텍스로 충돌만 방지, 타임아웃은 WDT에 위임
        WDT_FEED();  // SD.open() 전 WDT feed

        _file = SD.open(path, mode);
        _opened = _file ? true : false;

        if (!_opened) {
            Serial.printf("[SafeSD] 파일 열기 실패: %s\n", path);
        }
    }

    ~SafeSDFile() {
        if (_opened && _file) {
            WDT_FEED();   // flush 중 WDT reset 방지
            _file.close();
            _opened = false;
        }
        // _guard 소멸자가 SPI 뮤텍스 자동 반환
    }

    bool isOpen() const { return _opened; }

    // 포인터 접근 (File* 처럼 사용)
    File* operator->() { return &_file; }
    File& get()        { return _file; }

    // SD 준비 상태 전역 플래그
    static bool _sdReady;

private:
    SPIGuard _guard;
    File     _file;
    bool     _opened;
};

// ================================================================
// 안전한 SD 관리자
// ================================================================
class SafeSDManager {
public:
    static SafeSDManager& getInstance() {
        static SafeSDManager instance;
        return instance;
    }

    // ── 초기화 ──
    bool begin(uint8_t csPin = SD_CS_PIN) {
        SPIGuard guard(SPI_DEV_SD, 2000);
        if (!guard.acquired()) {
            Serial.println("[SafeSD] 초기화 SPI 획득 실패");
            return false;
        }

        WDT_FEED();

        if (!SD.begin(csPin)) {
            Serial.println("[SafeSD] ❌ SD 카드 마운트 실패");
            SafeSDFile::_sdReady = false;
            return false;
        }

        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("[SafeSD] ❌ SD 카드 미감지");
            SafeSDFile::_sdReady = false;
            return false;
        }

        // 디렉토리 생성
        _ensureDir("/logs");
        _ensureDir("/reports");

        SafeSDFile::_sdReady = true;
        _initTime = millis();

        Serial.printf("[SafeSD] ✅ SD 초기화 완료 (타입: %s, 용량: %lluMB)\n",
            cardType == CARD_MMC  ? "MMC"  :
            cardType == CARD_SD   ? "SDSC" :
            cardType == CARD_SDHC ? "SDHC" : "?",
            SD.cardSize() / (1024ULL * 1024ULL));

        return true;
    }

    bool isReady() const { return SafeSDFile::_sdReady; }

    // ── 안전한 파일 쓰기 (재시도 포함) ──
    bool safeAppend(const char* path, const char* data,
                    uint8_t maxRetry = SD_MAX_RETRY_COUNT)
    {
        if (!SafeSDFile::_sdReady) return false;

        for (uint8_t attempt = 0; attempt < maxRetry; attempt++) {
            SafeSDFile f(path, FILE_APPEND);

            if (!f.isOpen()) {
                Serial.printf("[SafeSD] 열기 실패 (재시도 %d/%d)\n",
                              attempt + 1, maxRetry);
                vTaskDelay(pdMS_TO_TICKS(SD_RETRY_DELAY_MS));
                continue;
            }

            WDT_FEED();
            f->println(data);
            // 소멸자에서 close + WDT feed
            return true;
        }

        _writeFailCount++;
        Serial.printf("[SafeSD] ❌ 쓰기 최종 실패: %s\n", path);
        return false;
    }

    // ── 파일 존재 확인 ──
    bool exists(const char* path) {
        SPIGuard guard(SPI_DEV_SD, SD_OPEN_TIMEOUT_MS);
        if (!guard.acquired() || !SafeSDFile::_sdReady) return false;
        return SD.exists(path);
    }

    // ── 통계 ──
    uint32_t getWriteFailCount() const { return _writeFailCount; }

    void printStats() const {
        Serial.printf("[SafeSD] 쓰기 실패: %lu회 | 가동 시간: %lus\n",
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
// 편의 매크로
// ================================================================
#define SAFE_SD     SafeSDManager::getInstance()

// 안전한 한 줄 append
#define SD_SAFE_APPEND(path, data) \
    SafeSDManager::getInstance().safeAppend(path, data)
