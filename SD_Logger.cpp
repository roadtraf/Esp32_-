// ================================================================
// SD_Logger_Hardened.cpp - SD 로거 안전 버전 (v3.9.4)
// ================================================================
// [테스트 시나리오 #4 + #8]
//
// 기존 문제:
//   - SD.open() 실패 시 단순 return (실패 로그 없음)
//   - SPI 뮤텍스 없이 직접 SD 접근
//   - getCurrentTimeISO8601()가 String 반환 (힙 단편화)
//   - file.close() 후 WDT feed 없음
//
// 변경사항:
//   - SafeSD 래퍼 사용 (SPI 뮤텍스 + 타임아웃)
//   - WDT feed 추가 (긴 SD 작업 중 reset 방지)
//   - char[] 기반 포맷 (String 제거)
//   - 실패 통계 기록
// ================================================================
#include "Config.h"
#include "SD_Logger.h"
#include "StateMachine.h"
#include "SafeSD.h"
#include "EnhancedWatchdog.h"
#include <SD.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// SafeSDFile::_sdReady 정의
bool SafeSDFile::_sdReady = false;

// ─────────────────── SD 초기화 ──────────────────────────────
void initSD() {
    if (!SafeSDManager::getInstance().begin(SD_CS_PIN)) {
        Serial.println("[SD] 초기화 실패");
        return;
    }
    Serial.println("[SD] 초기화 완료");
}

// ─────────────────── 내부: 헤더 라인 확인 ───────────────────
static void writeHeaderIfEmpty(SafeSDFile& f, const char* header) {
    if (f.get().size() == 0) {
        f->println(header);
    }
}

// ─────────────────── 사이클 로그 ────────────────────────────
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

    // [4][8] SafeSD: SPI 뮤텍스 + WDT feed 내장
    SafeSDFile f("/logs/cycle_log.csv", FILE_APPEND);
    if (!f.isOpen()) {
        Serial.println("[SD] cycle_log 열기 실패");
        return;
    }

    writeHeaderIfEmpty(f, "CycleNum,ISO8601,Duration,MinPressure,MaxPressure,AvgCurrent,Success");
    WDT_FEED();
    f->println(line);
    // 소멸자에서 자동 close + WDT feed
}

// ─────────────────── 에러 로그 ──────────────────────────────
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
        Serial.println("[SD] error_log 열기 실패");
        return;
    }

    writeHeaderIfEmpty(f, "Timestamp,ISO8601,Code,Severity,Message");
    WDT_FEED();
    f->println(line);

    Serial.println("[SD] 에러 로그 저장됨");
}

// ─────────────────── 센서 트렌드 로그 ──────────────────────
void logSensorTrend() {
    if (!SafeSDFile::_sdReady) return;

    char iso[ISO8601_BUFFER_SIZE];
    getCurrentTimeISO8601(iso, sizeof(iso));

    char line[256];
    snprintf(line, sizeof(line), "%lu,%s,%.2f,%.2f,%s",
             millis(), iso,
             sensorData.pressure,
             sensorData.current,
             getStateName(currentState));

    SafeSDFile f("/logs/sensor_trend.csv", FILE_APPEND);
    if (!f.isOpen()) return;

    writeHeaderIfEmpty(f, "Timestamp,ISO8601,Pressure,Current,State");
    WDT_FEED();
    f->println(line);
}

// ─────────────────── 일일 리포트 ────────────────────────────
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

    // [4] 긴 쓰기 작업: WDT feed 적극 활용
    SafeSDFile f(filename, FILE_WRITE, SD_WRITE_TIMEOUT_MS);
    if (!f.isOpen()) {
        Serial.println("[SD] 일일 리포트 생성 실패");
        return;
    }

    WDT_FEED();
    f->println("========================================");
    f->println("일일 리포트 v3.9.4 Hardened");
    f->println("========================================");
    f->printf("생성 시간: %s\n", iso);
    f->println();
    f->println("통계:");
    f->printf("  총 사이클: %lu\n",      stats.totalCycles);
    f->printf("  성공: %lu\n",           stats.successfulCycles);
    f->printf("  실패: %lu\n",           stats.failedCycles);
    f->printf("  총 에러: %lu\n",        stats.totalErrors);
    f->printf("  가동 시간: %lu초\n",    stats.uptime);
    WDT_FEED();
    f->println();
    f->println("센서 범위:");
    f->printf("  최소 압력: %.2f kPa\n", stats.minPressure);
    f->printf("  최대 압력: %.2f kPa\n", stats.maxPressure);
    f->printf("  평균 전류: %.2f A\n",   stats.averageCurrent);
    f->println();
    f->println("시스템 헬스:");
    f->printf("  힙 잔여: %u bytes\n",   esp_get_free_heap_size());
    f->printf("  힙 최소: %u bytes\n",   esp_get_minimum_free_heap_size());
    if (psramFound()) {
        f->printf("  PSRAM 잔여: %u bytes\n",
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    }
    f->printf("  SD 쓰기 실패: %lu회\n", SafeSDManager::getInstance().getWriteFailCount());
    f->printf("  SPI 충돌: %lu회\n",     SPIBusManager::getInstance().getTimeoutCount());
    f->printf("  WDT 재시작: %lu회\n",   enhancedWatchdog.getTotalRestarts());
    WDT_FEED();
    f->println("========================================");

    Serial.printf("[SD] 일일 리포트 생성: %s\n", filename);
}

// ─────────────────── 오래된 로그 정리 ───────────────────────
void cleanupOldLogs() {
    // 간단한 구현 유지
    Serial.println("[SD] 오래된 로그 정리...");
}

// ─────────────────── NTP 시간 동기화 ────────────────────────
void syncTime() {
    configTime(9 * 3600, 0, "pool.ntp.org", "time.google.com", "time.windows.com");

    Serial.print("[NTP] 시간 동기화 중");
    uint8_t attempts = 0;
    while (time(nullptr) < 100000 && attempts < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        WDT_FEED();  // [추가] NTP 대기 중 WDT feed
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    time_t t = time(nullptr);
    if (t > 100000) {
        Serial.printf("[NTP] 동기화 성공: %s", ctime(&t));
    } else {
        Serial.println("[NTP] 동기화 실패 (오프라인 모드)");
    }
}

// ─────────────────── ISO8601 타임스탬프 ─────────────────────
void getCurrentTimeISO8601(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize < ISO8601_BUFFER_SIZE) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }

    time_t now;
    struct tm timeinfo;
    time(&now);

    // NTP 미동기화 시 millis() 기반 타임스탬프 fallback
    if (now < 100000) {
        snprintf(buffer, bufferSize, "NOPT+%lus", millis() / 1000);
        return;
    }

    localtime_r(&now, &timeinfo);
    strftime(buffer, bufferSize, "%Y-%m-%dT%H:%M:%S+09:00", &timeinfo);
}
