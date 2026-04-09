// ================================================================
// Utils.h -    (v3.9.3 String )
// String  char[]    
// ================================================================
#pragma once

#include <Arduino.h>

//   
constexpr size_t TIME_BUFFER_SIZE = 32;
constexpr size_t DATETIME_BUFFER_SIZE = 64;
constexpr size_t FORMAT_BUFFER_SIZE = 32;
constexpr size_t CHIP_ID_BUFFER_SIZE = 24;
constexpr size_t REASON_BUFFER_SIZE = 48;

// ================================================================
//   (String )
// ================================================================
namespace Utils {
    //    ( ) 
    void formatTime(char* buffer, size_t bufferSize, uint32_t seconds);
    void formatDateTime(char* buffer, size_t bufferSize, time_t timestamp);
    void formatUptime(char* buffer, size_t bufferSize, uint32_t milliseconds);
    
    //    ( ) 
    void formatFloat(char* buffer, size_t bufferSize, float value, uint8_t decimals);
    void formatPercent(char* buffer, size_t bufferSize, float value);
    void formatBytes(char* buffer, size_t bufferSize, uint32_t bytes);
    
    //    
    bool isInRange(float value, float min, float max);
    bool isValidFloat(float value);
    bool isValidString(const char* str, size_t maxLen);
    
    //    
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    float constrainFloat(float value, float min, float max);
    float averageFloat(const float* array, size_t count);
    
    //   
    void logInfo(const char* tag, const char* message);
    void logWarning(const char* tag, const char* message);
    void logError(const char* tag, const char* message);
    void logDebug(const char* tag, const char* message);
    
    //    
    uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
    void rgb565ToRGB(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b);
    
    //    
    bool fileExists(const char* path);
    size_t getFileSize(const char* path);
    bool deleteFile(const char* path);
    
    //   
    void printMemoryInfo();
    uint32_t getFreeHeap();
    float getHeapFragmentation();
    
    //   ( ) 
    void getChipID(char* buffer, size_t bufferSize);
    void getResetReason(char* buffer, size_t bufferSize);
    void softReset();
    
    //  CRC/ 
    uint32_t calculateCRC32(const uint8_t* data, size_t length);
    uint16_t calculateChecksum(const uint8_t* data, size_t length);
    
    //    (snprintf ) 
    inline void safeSnprintf(char* buffer, size_t bufferSize, const char* format, ...) {
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, bufferSize, format, args);
        va_end(args);
        buffer[bufferSize - 1] = '\0';  //  
    }
}
