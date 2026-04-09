// ================================================================
// SD_Logger_Hardened.cpp - SD    (v3.9.4)
// ================================================================
// [  #4 + #8]
//
//  :
//   - SD.open()    return (  )
//   - SPI    SD 
//   - getCurrentTimeISO8601() String  ( )
//   - file.close()  WDT feed 
//
// :
//   - SafeSD   (SPI  + )
//   - WDT feed  ( SD   reset )
//   - char[]   (String )
//   -   
// ================================================================
#include "Config.h"
#include "SensorManager.h"
extern SensorManager sensorManager;
#include "SD_Logger.h"
#include "StateMachine.h"
#include "SafeSD.h"
#include "EnhancedWatchdog.h"
#include <SD.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// SafeSDFile::_sdReady 
bool SafeSDFile::_sdReady = false;

//  SD  
void initSD() {
    if (!SafeSDManager::getInstance().begin(SD_CS_PIN)) {
        Serial.println("[SD]  ");
        return;
    }
    Serial.println("[SD]  ");
}

//  :    
static void writeHeaderIfEmpty(SafeSDFile& f, const char* header) {
    if (f.get().size() == 0) {
        f->println(header);
    }
}

//    
void logCycle() {
    if (!SafeSDFile::_sdReady) return;

    char iso[ISO8601_BUFFER_SIZE];
    getCurrentTimeISO8601(iso, sizeof(iso));

    char line[256];
    snprintf(line, sizeof(line),
             "%lu,%s,%lu,%.2f,%.2f,%.2f,%d",
             stats.totalCycles,
             iso,
             millis() - stateStartTime,
             stats.minPressure,
             stats.maxPressure,
             stats.averageCurrent,
             currentState == STATE_COMPLETE ? 1 : 0);

    // [4][8] SafeSD: SPI  + WDT feed 
    SafeSDFile f("/logs/cycle_log.csv", FILE_APPEND);
    if (!f.isOpen()) {
        Serial.println("[SD] cycle_log  ");
        return;
    }

    writeHeaderIfEmpty(f, "CycleNum,ISO8601,Duration,MinPressure,MaxPressure,AvgCurrent,Success");
    WDT_FEED();
    f->println(line);
    //   close + WDT feed
}

//    
void logError(const ErrorInfo& error) {
    if (!SafeSDFile::_sdReady) return;

    char iso[ISO8601_BUFFER_SIZE];
    getCurrentTimeISO8601(iso, sizeof(iso));

    char line[256];
    snprintf(line, sizeof(line), "%lu,%s,%d,%d,%s",
             error.timestamp, iso,
             error.code, error.severity,
             error.message);

    SafeSDFile f("/logs/error_log.csv", FILE_APPEND);
    if (!f.isOpen()) {
        Serial.println("[SD] error_log  ");
        return;
    }

    writeHeaderIfEmpty(f, "Timestamp,ISO8601,Code,Severity,Message");
    WDT_FEED();
    f->println(line);

    Serial.println("[SD]   ");
}

//     
void logSensorTrend() {
    if (!SafeSDFile::_sdReady) return;

    char iso[ISO8601_BUFFER_SIZE];
    getCurrentTimeISO8601(iso, sizeof(iso));

    char line[256];
    snprintf(line, sizeof(line), "%lu,%s,%.2f,%.2f,%s",
             millis(), iso,
             sensorManager.getPressure(),
             sensorManager.getCurrent(),
             getStateName(currentState));

    SafeSDFile f("/logs/sensor_trend.csv", FILE_APPEND);
    if (!f.isOpen()) return;

    writeHeaderIfEmpty(f, "Timestamp,ISO8601,Pressure,Current,State");
    WDT_FEED();
    f->println(line);
}

//    
void generateDailyReport() {
    if (!SafeSDFile::_sdReady) return;

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char filename[64];
    strftime(filename, sizeof(filename), "/reports/daily_%Y%m%d.txt", &timeinfo);

    char iso[ISO8601_BUFFER_SIZE];
    getCurrentTimeISO8601(iso, sizeof(iso));

    // [4]   : WDT feed  
    SafeSDFile f(filename, FILE_WRITE, SD_WRITE_TIMEOUT_MS);
    if (!f.isOpen()) {
        Serial.println("[SD]    ");
        return;
    }

    WDT_FEED();
    f->println("========================================");
    f->println("  v3.9.4 Hardened");
    f->println("========================================");
    f->printf(" : %s\n", iso);
    f->println();
    f->println(":");
    f->printf("   : %lu\n",      stats.totalCycles);
    f->printf("  : %lu\n",           stats.successfulCycles);
    f->printf("  : %lu\n",           stats.failedCycles);
    f->printf("   : %lu\n",        stats.totalErrors);
    f->printf("   : %lu\n",    stats.uptime);
    WDT_FEED();
    f->println();
    f->println(" :");
    f->printf("   : %.2f kPa\n", stats.minPressure);
    f->printf("   : %.2f kPa\n", stats.maxPressure);
    f->printf("   : %.2f A\n",   stats.averageCurrent);
    f->println();
    f->println(" :");
    f->printf("   : %u bytes\n",   esp_get_free_heap_size());
    f->printf("   : %u bytes\n",   esp_get_minimum_free_heap_size());
    if (psramFound()) {
        f->printf("  PSRAM : %u bytes\n",
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    }
    f->printf("  SD  : %lu\n", SafeSDManager::getInstance().getWriteFailCount());
    f->printf("  SPI : %lu\n",     SPIBusManager::getInstance().getTimeoutCount());
    f->printf("  WDT : %lu\n",   enhancedWatchdog.getTotalRestarts());
    WDT_FEED();
    f->println("========================================");

    Serial.printf("[SD]   : %s\n", filename);
}

//     
void cleanupOldLogs() {
    //   
    Serial.println("[SD]   ...");
}

//  NTP   
void syncTime() {
    configTime(9 * 3600, 0, "pool.ntp.org", "time.google.com", "time.windows.com");

    Serial.print("[NTP]   ");
    uint8_t attempts = 0;
    while (time(nullptr) < 100000 && attempts < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        WDT_FEED();  // [] NTP   WDT feed
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    time_t t = time(nullptr);
    if (t > 100000) {
        Serial.printf("[NTP]  : %s", ctime(&t));
    } else {
        Serial.println("[NTP]   ( )");
    }
}

//  ISO8601  
void getCurrentTimeISO8601(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize < ISO8601_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }

    time_t now;
    struct tm timeinfo;
    time(&now);

    // NTP   millis()   fallback
    if (now < 100000) {
        snprintf(buffer, bufferSize, "NOPT+%lus", millis() / 1000);
        return;
    }

    localtime_r(&now, &timeinfo);
    strftime(buffer, bufferSize, "%Y-%m-%dT%H:%M:%S+09:00", &timeinfo);
}
