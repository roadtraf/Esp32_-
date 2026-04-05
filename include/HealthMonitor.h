/*
 * HealthMonitor.h
 * ESP32-S3 Vacuum Control System v3.9.1 Phase 1
 * System Health Monitoring & Predictive Maintenance
 * String → char[] 최적화
 */

#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <Arduino.h>

// 건강도 임계값
#define HEALTH_EXCELLENT    90.0f
#define HEALTH_GOOD         75.0f
#define HEALTH_WARNING      50.0f
#define HEALTH_CRITICAL     25.0f

// 유지보수 알림 레벨
enum MaintenanceLevel {
    MAINTENANCE_NONE = 0,
    MAINTENANCE_SOON = 1,
    MAINTENANCE_REQUIRED = 2,
    MAINTENANCE_URGENT = 3
};

// 건강도 요소
struct HealthFactors {
    float pumpEfficiency;      // 펌프 효율 (0-100%)
    float temperatureHealth;   // 온도 건강도 (0-100%)
    float currentHealth;       // 전류 건강도 (0-100%)
    float runtimeHealth;       // 작동시간 건강도 (0-100%)
};

class HealthMonitor {
private:
    float currentHealthScore;
    HealthFactors factors;
    MaintenanceLevel maintenanceLevel;
    
    // 누적 데이터
    unsigned long totalRuntime;        // 총 작동시간 (초)
    unsigned long lastMaintenanceTime; // 마지막 유지보수 시간
    
    // 성능 추적
    float avgVacuumAchieveTime;        // 평균 진공 달성 시간
    float avgCurrentConsumption;       // 평균 전류 소비
    float peakTemperature;             // 최고 온도
    
    // 이상 카운터
    int overTempCount;
    int overCurrentCount;
    int lowVacuumCount;

public:
    HealthMonitor();
    
    // 초기화
    void begin();
    
    // 건강도 계산
    float calculateHealthScore(
        float vacuumPressure,
        float targetPressure,
        float temperature,
        float current,
        unsigned long runtime
    );
    
    // 개별 건강도 요소 계산
    float calculatePumpEfficiency(float vacuumPressure, float targetPressure);
    float calculateTemperatureHealth(float temperature);
    float calculateCurrentHealth(float current);
    float calculateRuntimeHealth(unsigned long runtime);
    
    // 유지보수 레벨 결정
    MaintenanceLevel determineMaintenanceLevel();
    
    // 데이터 업데이트
    void updateRuntime(unsigned long seconds);
    void recordTemperature(float temp);
    void recordCurrent(float curr);
    void recordVacuumAchieveTime(float seconds);
    
    // 이상 기록
    void recordOverTemp();
    void recordOverCurrent();
    void recordLowVacuum();
    
    // 유지보수 수행 기록
    void performMaintenance();
    
    // Getter
    float getHealthScore() { return currentHealthScore; }
    HealthFactors getHealthFactors() { return factors; }
    MaintenanceLevel getMaintenanceLevel() { return maintenanceLevel; }
    unsigned long getTotalRuntime() { return totalRuntime; }
    unsigned long getTimeSinceLastMaintenance();
    
    // v3.9.1: String → const char* 변환
    const char* getMaintenanceMessage();
    void getDetailedMaintenanceAdvice(char* buffer, size_t size);
};

#endif // HEALTH_MONITOR_H
