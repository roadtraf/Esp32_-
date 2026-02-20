// ================================================================
// DataLogger.h  —  v3.9.1 Phase 1 최적화
// 건강도 이력 및 추세 데이터 로깅
// String → char[] 변환
// ================================================================
#pragma once
#include "Config.h"
#include "HealthMonitor.h"
#include <SD.h>

// ─────────────────── 로그 데이터 구조체 ─────────────────────
struct HealthLogEntry {
    uint32_t timestamp;           // Unix timestamp
    float healthScore;            // 전체 건강도
    float pumpEfficiency;         // 펌프 효율
    float temperatureHealth;      // 온도 건강도
    float currentHealth;          // 전류 건강도
    float runtimeHealth;          // 작동시간 건강도
    float pressure;               // 실제 압력
    float temperature;            // 실제 온도
    float current;                // 실제 전류
    MaintenanceLevel maintenanceLevel;  // 유지보수 레벨
};

struct TrendStatistics {
    float avg_24h;                // 24시간 평균
    float avg_7d;                 // 7일 평균
    float avg_30d;                // 30일 평균
    float trend;                  // 추세 (양수=상승, 음수=하락)
    float volatility;             // 변동성
    uint32_t lastUpdate;          // 마지막 업데이트 시간
};

// ─────────────────── DataLogger 클래스 ──────────────────────
class DataLogger {
public:
    DataLogger();
    
    // 초기화
    void begin();
    
    // 건강도 로깅
    void logHealthData(const HealthMonitor& healthMonitor);
    void logHealthDataDetailed(float health, float pumpEff, float tempHealth, 
                               float currentHealth, float runtimeHealth,
                               MaintenanceLevel level);
    
    // 유지보수 이력 로깅
    void logMaintenance(float healthBefore, float healthAfter, const char* note);
    
    // 추세 분석
    TrendStatistics calculateTrend(uint16_t hours);  // 최근 N시간 추세
    TrendStatistics getDailyTrend();                 // 24시간 추세
    TrendStatistics getWeeklyTrend();                // 7일 추세
    TrendStatistics getMonthlyTrend();               // 30일 추세
    
    // 데이터 읽기
    bool getWeeklyHealthHistory(float* values, int& count);
    bool readHealthHistory(HealthLogEntry* buffer, uint16_t maxCount, uint16_t& actualCount);
    bool readMaintenanceHistory(char* buffer, uint16_t maxSize);
    
    // CSV 내보내기
    bool exportHealthToCSV(const char* filename);
    bool exportMaintenanceToCSV(const char* filename);
    
    // 데이터 분석
    float predictHealthScore(uint8_t hoursAhead);   // N시간 후 건강도 예측
    uint32_t estimateDaysToMaintenance();           // 유지보수까지 남은 일수
    
    // 파일 관리
    void clearOldLogs(uint16_t daysToKeep);
    uint32_t getLogSize();
    uint32_t getLogCount();
    
private:
    bool initialized;
    uint32_t lastLogTime;
    uint32_t logInterval;         // 로그 간격 (기본 1시간)
    
    // 내부 함수
    void ensureDirectories();
    void writeHealthEntry(const HealthLogEntry& entry);
    bool appendToMaintenanceLog(uint32_t timestamp, float before, float after, const char* note);
    
    // 추세 계산 헬퍼
    float calculateLinearTrend(float* values, uint16_t count);
    float calculateVolatility(float* values, uint16_t count);
    float calculateAverage(float* values, uint16_t count);
    
    // 파일 경로
    static const char* HEALTH_LOG_FILE;
    static const char* MAINTENANCE_LOG_FILE;
    static const char* TREND_DATA_FILE;
};

// ─────────────────── 전역 인스턴스 ──────────────────────────
extern DataLogger dataLogger;

// ─────────────────── 유틸리티 함수 (v3.9.1: String → char[]) ───
void getTimestampString(uint32_t timestamp, char* buffer, size_t size);
uint32_t parseTimestamp(const char* timeStr);
