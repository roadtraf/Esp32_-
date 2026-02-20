// ================================================================
// HealthMonitor.cpp - 시스템 건강도 모니터링 (v3.9.2 수정)
// sensorData → sensorManager 변경
// ================================================================

#include "../include/HealthMonitor.h"
#include "../include/Config.h"
#include "SensorManager.h"  // ← 추가

// v3.9: 음성 알림
#ifdef ENABLE_VOICE_ALERTS
#include "../include/VoiceAlert.h"
extern VoiceAlert voiceAlert;
#endif

// 외부 SensorManager 참조
extern SensorManager sensorManager;

HealthMonitor::HealthMonitor() {
    currentHealthScore = 100.0f;
    maintenanceLevel = MAINTENANCE_NONE;
    totalRuntime = 0;
    lastMaintenanceTime = 0;
    avgVacuumAchieveTime = 0;
    avgCurrentConsumption = 0;
    peakTemperature = 0;
    overTempCount = 0;
    overCurrentCount = 0;
    lowVacuumCount = 0;
    
    memset(&factors, 0, sizeof(factors));
}

void HealthMonitor::begin() {
    Serial.println("[HealthMonitor] 초기화 완료");
    currentHealthScore = 100.0f;
    lastMaintenanceTime = millis();
}

// ================================================================
// update() - SensorManager 사용 버전
// ================================================================
void HealthMonitor::update(
    float pressure,
    float temperature,
    float current,
    uint8_t pwm,
    SystemState state
) {
    // 매개변수로 받은 값 사용
    // 추가로 필요한 센서 값은 sensorManager에서 가져옴
    
    MaintenanceLevel previousLevel = maintenanceLevel;
    
    // 각 요소별 건강도 계산
    factors.pumpEfficiency = calculatePumpEfficiency(pressure, TARGET_PRESSURE);
    factors.temperatureHealth = calculateTemperatureHealth(temperature);
    factors.currentHealth = calculateCurrentHealth(current);
    factors.runtimeHealth = calculateRuntimeHealth(totalRuntime);
    
    // 가중 평균으로 전체 건강도 계산
    currentHealthScore = (
        factors.pumpEfficiency * 0.35f +
        factors.temperatureHealth * 0.25f +
        factors.currentHealth * 0.25f +
        factors.runtimeHealth * 0.15f
    );
    
    // 이상 카운터 페널티
    float penalty = 0;
    penalty += overTempCount * 2.0f;
    penalty += overCurrentCount * 1.5f;
    penalty += lowVacuumCount * 1.0f;
    
    currentHealthScore = max(0.0f, currentHealthScore - penalty);
    
    // 유지보수 레벨 결정
    maintenanceLevel = determineMaintenanceLevel();
    
    #ifdef ENABLE_VOICE_ALERTS
    if (voiceAlert.isOnline() && maintenanceLevel != previousLevel) {
        if (maintenanceLevel > previousLevel && maintenanceLevel >= MAINTENANCE_SOON) {
            voiceAlert.playMaintenanceMessage(maintenanceLevel);
            
            if (maintenanceLevel == MAINTENANCE_URGENT) {
                voiceAlert.enableRepeat(true);
                voiceAlert.setRepeatCount(2);
            }
        }
    }
    #endif
    
    totalRuntime++;
}

float HealthMonitor::calculatePumpEfficiency(float pressure, float targetPressure) {
    if (targetPressure == 0) return 100.0f;
    
    float ratio = abs(pressure) / abs(targetPressure);
    
    if (ratio >= 0.95f) return 100.0f;
    else if (ratio >= 0.90f) return 90.0f;
    else if (ratio >= 0.85f) return 80.0f;
    else if (ratio >= 0.80f) return 70.0f;
    else if (ratio >= 0.70f) return 60.0f;
    else return 50.0f;
}

float HealthMonitor::calculateTemperatureHealth(float temperature) {
    if (temperature < TEMP_THRESHOLD_WARNING) return 100.0f;
    else if (temperature < TEMP_THRESHOLD_CRITICAL) {
        overTempCount++;
        return 80.0f;
    }
    else if (temperature < TEMP_THRESHOLD_SHUTDOWN) {
        overTempCount += 2;
        return 60.0f;
    }
    else {
        overTempCount += 3;
        return 40.0f;
    }
}

float HealthMonitor::calculateCurrentHealth(float current) {
    if (current < CURRENT_THRESHOLD_WARNING) return 100.0f;
    else if (current < CURRENT_THRESHOLD_CRITICAL) {
        overCurrentCount++;
        return 80.0f;
    }
    else {
        overCurrentCount += 2;
        return 60.0f;
    }
}

float HealthMonitor::calculateRuntimeHealth(unsigned long runtime) {
    // 1000시간 = 100%, 10000시간 = 50%
    if (runtime < 1000) return 100.0f;
    else if (runtime < 5000) return 90.0f;
    else if (runtime < 10000) return 80.0f;
    else if (runtime < 20000) return 70.0f;
    else return 60.0f;
}

MaintenanceLevel HealthMonitor::determineMaintenanceLevel() {
    if (currentHealthScore >= 90.0f) return MAINTENANCE_NONE;
    else if (currentHealthScore >= 75.0f) return MAINTENANCE_SOON;
    else if (currentHealthScore >= 60.0f) return MAINTENANCE_RECOMMENDED;
    else if (currentHealthScore >= 45.0f) return MAINTENANCE_URGENT;
    else return MAINTENANCE_CRITICAL;
}

float HealthMonitor::getHealthScore() const {
    return currentHealthScore;
}

MaintenanceLevel HealthMonitor::getMaintenanceLevel() const {
    return maintenanceLevel;
}

const char* HealthMonitor::getMaintenanceLevelString() const {
    switch(maintenanceLevel) {
        case MAINTENANCE_NONE: return "정상";
        case MAINTENANCE_SOON: return "곧 필요";
        case MAINTENANCE_RECOMMENDED: return "권장";
        case MAINTENANCE_URGENT: return "긴급";
        case MAINTENANCE_CRITICAL: return "위험";
        default: return "알 수 없음";
    }
}

void HealthMonitor::reset() {
    currentHealthScore = 100.0f;
    maintenanceLevel = MAINTENANCE_NONE;
    overTempCount = 0;
    overCurrentCount = 0;
    lowVacuumCount = 0;
    lastMaintenanceTime = millis();
}

void HealthMonitor::printStatus() const {
    Serial.println("\n=== 건강도 모니터 ===");
    Serial.printf("건강도: %.1f%%\n", currentHealthScore);
    Serial.printf("유지보수: %s\n", getMaintenanceLevelString());
    Serial.printf("펌프 효율: %.1f%%\n", factors.pumpEfficiency);
    Serial.printf("온도 건강: %.1f%%\n", factors.temperatureHealth);
    Serial.printf("전류 건강: %.1f%%\n", factors.currentHealth);
    Serial.printf("작동시간: %lu\n", totalRuntime);
    Serial.println("=====================\n");
}
