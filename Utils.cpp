// ================================================================
// Utils.cpp - 공통 유틸리티 구현 (v3.9.3 String 최적화)
// String → char[] 변환으로 힙 단편화 방지
// ================================================================
#include "Utils.h"
#include <SPIFFS.h>
#include <esp_system.h>
#include <rom/rtc.h>

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace Utils {

// ================================================================
// 시간 포맷팅 (버퍼 기반)
// ================================================================

void formatTime(char* buffer, size_t bufferSize, uint32_t seconds) {
    if (!buffer || bufferSize < TIME_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, minutes, secs);
}

void formatDateTime(char* buffer, size_t bufferSize, time_t timestamp) {
    if (!buffer || bufferSize < DATETIME_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void formatUptime(char* buffer, size_t bufferSize, uint32_t milliseconds) {
    if (!buffer || bufferSize < TIME_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    uint32_t seconds = milliseconds / 1000;
    uint32_t days = seconds / 86400;
    uint32_t hours = (seconds % 86400) / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    
    if (days > 0) {
        snprintf(buffer, bufferSize, "%lud %02luh %02lum", days, hours, minutes);
    } else {
        snprintf(buffer, bufferSize, "%02luh %02lum", hours, minutes);
    }
}

// ================================================================
// 문자열 변환 (버퍼 기반)
// ================================================================

void formatFloat(char* buffer, size_t bufferSize, float value, uint8_t decimals) {
    if (!buffer || bufferSize < FORMAT_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    char format[10];
    snprintf(format, sizeof(format), "%%.%df", decimals);
    snprintf(buffer, bufferSize, format, value);
}

void formatPercent(char* buffer, size_t bufferSize, float value) {
    if (!buffer || bufferSize < FORMAT_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    snprintf(buffer, bufferSize, "%.1f%%", value);
}

void formatBytes(char* buffer, size_t bufferSize, uint32_t bytes) {
    if (!buffer || bufferSize < FORMAT_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    if (bytes < 1024) {
        snprintf(buffer, bufferSize, "%lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, bufferSize, "%.1f KB", bytes / 1024.0f);
    } else {
        snprintf(buffer, bufferSize, "%.1f MB", bytes / (1024.0f * 1024.0f));
    }
}

// ================================================================
// 데이터 검증
// ================================================================

bool isInRange(float value, float min, float max) {
    return (value >= min && value <= max);
}

bool isValidFloat(float value) {
    return !isnan(value) && !isinf(value);
}

bool isValidString(const char* str, size_t maxLen) {
    if (str == nullptr) return false;
    return (strlen(str) <= maxLen);
}

// ================================================================
// 수학 함수
// ================================================================

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    if (in_max == in_min) return out_min;  // 0으로 나누기 방지
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float constrainFloat(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float averageFloat(const float* array, size_t count) {
    if (!array || count == 0) return 0.0f;
    
    float sum = 0.0f;
    for (size_t i = 0; i < count; i++) {
        sum += array[i];
    }
    
    return sum / count;
}

// ================================================================
// 로깅
// ================================================================

void logInfo(const char* tag, const char* message) {
    Serial.printf("[INFO][%s] %s\n", tag, message);
}

void logWarning(const char* tag, const char* message) {
    Serial.printf("[WARN][%s] %s\n", tag, message);
}

void logError(const char* tag, const char* message) {
    Serial.printf("[ERROR][%s] %s\n", tag, message);
}

void logDebug(const char* tag, const char* message) {
#ifdef DEBUG_MODE
    Serial.printf("[DEBUG][%s] %s\n", tag, message);
#endif
}

// ================================================================
// 색상 변환
// ================================================================

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void rgb565ToRGB(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (!r || !g || !b) return;
    
    *r = (color >> 11) << 3;
    *g = ((color >> 5) & 0x3F) << 2;
    *b = (color & 0x1F) << 3;
}

// ================================================================
// 파일 시스템
// ================================================================

bool fileExists(const char* path) {
    if (!path) return false;
    
    File file = SPIFFS.open(path, "r");
    if (!file) return false;
    
    file.close();
    return true;
}

size_t getFileSize(const char* path) {
    if (!path) return 0;
    
    File file = SPIFFS.open(path, "r");
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}

bool deleteFile(const char* path) {
    if (!path) return false;
    return SPIFFS.remove(path);
}

// ================================================================
// 메모리
// ================================================================

void printMemoryInfo() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    
    Serial.println("\n=== 메모리 정보 ===");
    Serial.printf("  전체 힙:     %u bytes\n", totalHeap);
    Serial.printf("  사용 가능:   %u bytes\n", freeHeap);
    Serial.printf("  최소 여유:   %u bytes\n", minFreeHeap);
    Serial.printf("  사용률:      %.1f%%\n", 
                  (totalHeap - freeHeap) * 100.0f / totalHeap);
    
    #ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        Serial.printf("  PSRAM 전체:  %u bytes\n", ESP.getPsramSize());
        Serial.printf("  PSRAM 여유:  %u bytes\n", ESP.getFreePsram());
    }
    #endif
    
    Serial.println("==================\n");
}

uint32_t getFreeHeap() {
    return ESP.getFreeHeap();
}

float getHeapFragmentation() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxAllocHeap = ESP.getMaxAllocHeap();
    
    if (freeHeap == 0) return 100.0f;
    
    return (1.0f - (float)maxAllocHeap / (float)freeHeap) * 100.0f;
}

// ================================================================
// 시스템 (버퍼 기반)
// ================================================================

void getChipID(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize < CHIP_ID_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    uint64_t chipid = ESP.getEfuseMac();
    snprintf(buffer, bufferSize, "%04X%08X", 
             (uint16_t)(chipid >> 32), (uint32_t)chipid);
}

void getResetReason(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize < REASON_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    RESET_REASON reason = rtc_get_reset_reason(0);
    
    switch (reason) {
        case POWERON_RESET:
            snprintf(buffer, bufferSize, "Power On");
            break;
        case SW_RESET:
            snprintf(buffer, bufferSize, "Software Reset");
            break;
        case OWDT_RESET:
            snprintf(buffer, bufferSize, "WDT Reset");
            break;
        case DEEPSLEEP_RESET:
            snprintf(buffer, bufferSize, "Deep Sleep");
            break;
        case SDIO_RESET:
            snprintf(buffer, bufferSize, "SDIO Reset");
            break;
        case TG0WDT_SYS_RESET:
            snprintf(buffer, bufferSize, "Timer Group0 WDT");
            break;
        case TG1WDT_SYS_RESET:
            snprintf(buffer, bufferSize, "Timer Group1 WDT");
            break;
        case RTCWDT_SYS_RESET:
            snprintf(buffer, bufferSize, "RTC WDT");
            break;
        case INTRUSION_RESET:
            snprintf(buffer, bufferSize, "Intrusion");
            break;
        case TGWDT_CPU_RESET:
            snprintf(buffer, bufferSize, "CPU WDT");
            break;
        case SW_CPU_RESET:
            snprintf(buffer, bufferSize, "CPU Software");
            break;
        case RTCWDT_CPU_RESET:
            snprintf(buffer, bufferSize, "RTC CPU WDT");
            break;
        case RTCWDT_BROWN_OUT_RESET:
            snprintf(buffer, bufferSize, "Brown Out");
            break;
        case RTCWDT_RTC_RESET:
            snprintf(buffer, bufferSize, "RTC Reset");
            break;
        default:
            snprintf(buffer, bufferSize, "Unknown (%d)", reason);
            break;
    }
}

void softReset() {
    Serial.println("\n🔄 소프트웨어 재시작...\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
}

// ================================================================
// CRC/체크섬
// ================================================================

uint32_t calculateCRC32(const uint8_t* data, size_t length) {
    if (!data || length == 0) return 0;
    
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

uint16_t calculateChecksum(const uint8_t* data, size_t length) {
    if (!data || length == 0) return 0;
    
    uint16_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    
    return checksum;
}

} // namespace Utils
