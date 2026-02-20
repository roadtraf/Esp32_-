// ================================================================
// Utils.h - 공통 유틸리티 모듈 (v3.9.3 String 최적화)
// String → char[] 변환으로 힙 사용 최소화
// ================================================================
#pragma once

#include <Arduino.h>

// 버퍼 크기 상수
constexpr size_t TIME_BUFFER_SIZE = 32;
constexpr size_t DATETIME_BUFFER_SIZE = 64;
constexpr size_t FORMAT_BUFFER_SIZE = 32;
constexpr size_t CHIP_ID_BUFFER_SIZE = 24;
constexpr size_t REASON_BUFFER_SIZE = 48;

// ================================================================
// 유틸리티 네임스페이스 (String 최적화)
// ================================================================
namespace Utils {
    // ── 시간 포맷팅 (버퍼 기반) ──
    void formatTime(char* buffer, size_t bufferSize, uint32_t seconds);
    void formatDateTime(char* buffer, size_t bufferSize, time_t timestamp);
    void formatUptime(char* buffer, size_t bufferSize, uint32_t milliseconds);
    
    // ── 문자열 변환 (버퍼 기반) ──
    void formatFloat(char* buffer, size_t bufferSize, float value, uint8_t decimals);
    void formatPercent(char* buffer, size_t bufferSize, float value);
    void formatBytes(char* buffer, size_t bufferSize, uint32_t bytes);
    
    // ── 데이터 검증 ──
    bool isInRange(float value, float min, float max);
    bool isValidFloat(float value);
    bool isValidString(const char* str, size_t maxLen);
    
    // ── 수학 함수 ──
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    float constrainFloat(float value, float min, float max);
    float averageFloat(const float* array, size_t count);
    
    // ── 로깅 ──
    void logInfo(const char* tag, const char* message);
    void logWarning(const char* tag, const char* message);
    void logError(const char* tag, const char* message);
    void logDebug(const char* tag, const char* message);
    
    // ── 색상 변환 ──
    uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
    void rgb565ToRGB(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b);
    
    // ── 파일 시스템 ──
    bool fileExists(const char* path);
    size_t getFileSize(const char* path);
    bool deleteFile(const char* path);
    
    // ── 메모리 ──
    void printMemoryInfo();
    uint32_t getFreeHeap();
    float getHeapFragmentation();
    
    // ── 시스템 (버퍼 기반) ──
    void getChipID(char* buffer, size_t bufferSize);
    void getResetReason(char* buffer, size_t bufferSize);
    void softReset();
    
    // ── CRC/체크섬 ──
    uint32_t calculateCRC32(const uint8_t* data, size_t length);
    uint16_t calculateChecksum(const uint8_t* data, size_t length);
    
    // ── 편의 함수 (snprintf 래퍼) ──
    inline void safeSnprintf(char* buffer, size_t bufferSize, const char* format, ...) {
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, bufferSize, format, args);
        va_end(args);
        buffer[bufferSize - 1] = '\0';  // 안전성 보장
    }
}
