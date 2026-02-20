// ================================================================
// AdvancedAnalyzer.cpp  —  v3.8.3 AI 기반 고급 분석 (완전판)
// ================================================================
#include "../include/AdvancedAnalyzer.h"
#include "../include/Config.h"
#include "../include/DataLogger.h"
#include <SD.h>

// 외부 참조
extern HealthMonitor healthMonitor;
extern DataLogger dataLogger;
extern Statistics stats;
extern SensorData sensorData;
extern SystemConfig config;
extern bool sdReady;

// 전역 인스턴스
AdvancedAnalyzer advancedAnalyzer;

// ─────────────────── 부품 정격 수명 (시간) ──────────────────
#define PUMP_RATED_LIFE     10000    // 펌프: 10,000시간
#define MOTOR_RATED_LIFE    15000    // 모터: 15,000시간
#define SEAL_RATED_LIFE     5000     // 씰: 5,000시간
#define VALVE_RATED_LIFE    8000     // 밸브: 8,000시간
#define SENSOR_RATED_LIFE   20000    // 센서: 20,000시간

// ─────────────────── 생성자 ─────────────────────────────────
AdvancedAnalyzer::AdvancedAnalyzer() {
    initialized = false;
    baselineHealth = 100.0f;
    baselineTimestamp = 0;
    degradationRate = 0.0f;
}

// ─────────────────── 초기화 ─────────────────────────────────
void AdvancedAnalyzer::begin() {
    setBaseline();
    initialized = true;
    Serial.println("[AdvancedAnalyzer] 초기화 완료");
}

// ═══════════════════════════════════════════════════════════════
//  고장 예측
// ═══════════════════════════════════════════════════════════════

FailurePrediction AdvancedAnalyzer::predictFailure() {
    FailurePrediction pred;
    
    // 여러 분석 방법 실행
    FailureType vacuumType = analyzeVacuumTrend();
    FailureType tempType = analyzeTemperatureTrend();
    FailureType currentType = analyzeCurrentTrend();
    FailureType combinedType = analyzeCombinedPatterns();
    
    // 가장 확률 높은 고장 선택
    float maxProb = 0.0f;
    FailureType selectedType = FAILURE_NONE;
    
    struct {
        FailureType type;
        float prob;
    } candidates[] = {
        {vacuumType, calculateFailureProbability(vacuumType)},
        {tempType, calculateFailureProbability(tempType)},
        {currentType, calculateFailureProbability(currentType)},
        {combinedType, calculateFailureProbability(combinedType)}
    };
    
    for (int i = 0; i < 4; i++) {
        if (candidates[i].prob > maxProb) {
            maxProb = candidates[i].prob;
            selectedType = candidates[i].type;
        }
    }
    
    pred.type = selectedType;
    pred.confidence = maxProb;
    pred.estimatedDays = estimateTimeToFailure(selectedType);
    
    strncpy(pred.description, getFailureTypeDescription(selectedType), 
            sizeof(pred.description) - 1);
    strncpy(pred.recommendation, getFailureTypeRecommendation(selectedType), 
            sizeof(pred.recommendation) - 1);
    
    Serial.printf("[Analysis] 예측 고장: %s (신뢰도 %.1f%%)\n",
                 getFailureTypeName(selectedType), maxProb);
    
    return pred;
}

void AdvancedAnalyzer::predictMultipleFailures(FailurePrediction* predictions, 
                                                uint8_t maxCount, 
                                                uint8_t& actualCount) {
    actualCount = 0;
    
    // 모든 고장 유형별 확률 계산
    struct {
        FailureType type;
        float prob;
    } all[8] = {
        {FAILURE_PUMP_DEGRADATION, 0},
        {FAILURE_SEAL_LEAK, 0},
        {FAILURE_MOTOR_BEARING, 0},
        {FAILURE_VALVE_MALFUNCTION, 0},
        {FAILURE_SENSOR_DRIFT, 0},
        {FAILURE_THERMAL_ISSUE, 0},
        {FAILURE_ELECTRICAL, 0},
        {FAILURE_MECHANICAL_WEAR, 0}
    };
    
    // 확률 계산
    for (int i = 0; i < 8; i++) {
        all[i].prob = calculateFailureProbability(all[i].type);
    }
    
    // 확률 순 정렬 (버블 소트)
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7 - i; j++) {
            if (all[j].prob < all[j+1].prob) {
                auto temp = all[j];
                all[j] = all[j+1];
                all[j+1] = temp;
            }
        }
    }
    
    // 상위 N개 선택
    for (uint8_t i = 0; i < maxCount && i < 8; i++) {
        if (all[i].prob > 10.0f) {  // 10% 이상만
            predictions[i].type = all[i].type;
            predictions[i].confidence = all[i].prob;
            predictions[i].estimatedDays = estimateTimeToFailure(all[i].type);
            
            strncpy(predictions[i].description, 
                   getFailureTypeDescription(all[i].type),
                   sizeof(predictions[i].description) - 1);
            strncpy(predictions[i].recommendation,
                   getFailureTypeRecommendation(all[i].type),
                   sizeof(predictions[i].recommendation) - 1);
            
            actualCount++;
        }
    }
}

// ─────────────────── 패턴 분석 ──────────────────────────────

FailureType AdvancedAnalyzer::analyzeVacuumTrend() {
    // 진공도 추세 분석
    #ifdef ENABLE_DATA_LOGGING
    TrendStatistics trend = dataLogger.getDailyTrend();
    #else
    TrendStatistics trend = {0};
    trend.volatility = 5.0f;
    #endif
    
    float avgPressure = abs(sensorData.pressure);
    float targetPressure = abs(config.targetPressure);
    
    // 펌프 성능 저하: 목표 압력 도달 실패
    if (avgPressure < targetPressure * 0.8f) {
        Serial.println("[Analysis] 진공도 저하 → 펌프 성능 저하 의심");
        return FAILURE_PUMP_DEGRADATION;
    }
    
    // 씰 누수: 압력 유지 실패 (변동성 높음)
    if (trend.volatility > 10.0f) {
        Serial.println("[Analysis] 압력 변동성 높음 → 씰 누수 의심");
        return FAILURE_SEAL_LEAK;
    }
    
    return FAILURE_NONE;
}

FailureType AdvancedAnalyzer::analyzeTemperatureTrend() {
    // 온도 추세 분석
    float temp = sensorData.temperature;
    
    // 열 문제: 지속적 고온
    if (temp > TEMP_THRESHOLD_WARNING) {
        Serial.printf("[Analysis] 고온 %.1f°C → 열 문제 의심\n", temp);
        return FAILURE_THERMAL_ISSUE;
    }
    
    // 모터 베어링 마모: 온도 상승 + 전류 증가
    if (temp > 45.0f && sensorData.current > 4.5f) {
        Serial.println("[Analysis] 온도+전류 상승 → 베어링 마모 의심");
        return FAILURE_MOTOR_BEARING;
    }
    
    return FAILURE_NONE;
}

FailureType AdvancedAnalyzer::analyzeCurrentTrend() {
    // 전류 추세 분석
    float current = sensorData.current;
    
    // 전기적 문제: 비정상 전류
    if (current > CURRENT_THRESHOLD_WARNING) {
        Serial.printf("[Analysis] 고전류 %.2fA → 전기적 문제 의심\n", current);
        return FAILURE_ELECTRICAL;
    }
    
    // 기계적 마모: 전류 점진적 증가 (추세 분석 필요)
    
    return FAILURE_NONE;
}

FailureType AdvancedAnalyzer::analyzeCombinedPatterns() {
    // 복합 패턴 분석
    
    // 센서 드리프트: 비현실적 값
    if (sensorData.pressure > -10.0f || sensorData.pressure < -100.0f) {
        Serial.println("[Analysis] 비정상 압력값 → 센서 드리프트 의심");
        return FAILURE_SENSOR_DRIFT;
    }
    
    // 밸브 오작동: 진공 파괴 시간 비정상 (상태 머신 타이밍 분석)
    
    return FAILURE_NONE;
}

// ─────────────────── 확률 계산 ──────────────────────────────

float AdvancedAnalyzer::calculateFailureProbability(FailureType type) {
    float probability = 0.0f;
    
    switch (type) {
        case FAILURE_PUMP_DEGRADATION: {
            float pumpHealth = calculatePumpHealth();
            probability = 100.0f - pumpHealth;
            break;
        }
        
        case FAILURE_SEAL_LEAK: {
            #ifdef ENABLE_DATA_LOGGING
            TrendStatistics trend = dataLogger.getDailyTrend();
            float volatilityFactor = min(trend.volatility * 5.0f, 50.0f);
            #else
            float volatilityFactor = 20.0f;
            #endif
            float agingFactor = min((stats.uptime / 3600) / 100.0f, 50.0f);
            probability = volatilityFactor + agingFactor;
            break;
        }
        
        case FAILURE_MOTOR_BEARING: {
            float motorHealth = calculateMotorHealth();
            probability = 100.0f - motorHealth;
            break;
        }
        
        case FAILURE_VALVE_MALFUNCTION: {
            float valveHealth = calculateValveHealth();
            probability = 100.0f - valveHealth;
            break;
        }
        
        case FAILURE_SENSOR_DRIFT: {
            float sensorHealth = calculateSensorHealth();
            probability = 100.0f - sensorHealth;
            break;
        }
        
        case FAILURE_THERMAL_ISSUE: {
            float temp = sensorData.temperature;
            if (temp > TEMP_THRESHOLD_CRITICAL) probability = 80.0f;
            else if (temp > TEMP_THRESHOLD_WARNING) probability = 50.0f;
            else probability = (temp / TEMP_THRESHOLD_WARNING) * 30.0f;
            break;
        }
        
        case FAILURE_ELECTRICAL: {
            float current = sensorData.current;
            if (current > CURRENT_THRESHOLD_CRITICAL) probability = 80.0f;
            else if (current > CURRENT_THRESHOLD_WARNING) probability = 50.0f;
            else probability = (current / CURRENT_THRESHOLD_WARNING) * 30.0f;
            break;
        }
        
        case FAILURE_MECHANICAL_WEAR: {
            uint32_t hours = stats.uptime / 3600;
            probability = min((float)hours / 100.0f, 100.0f);
            break;
        }
        
        case FAILURE_NONE:
        default:
            probability = 0.0f;
            break;
    }
    
    return max(0.0f, min(probability, 100.0f));
}

uint32_t AdvancedAnalyzer::estimateTimeToFailure(FailureType type) {
    float probability = calculateFailureProbability(type);
    
    if (probability < 20.0f) return 365;  // 1년 이상
    if (probability < 40.0f) return 180;  // 6개월
    if (probability < 60.0f) return 90;   // 3개월
    if (probability < 80.0f) return 30;   // 1개월
    return 7;  // 1주일
}

// ═══════════════════════════════════════════════════════════════
//  부품 수명 분석
// ═══════════════════════════════════════════════════════════════

void AdvancedAnalyzer::analyzeComponentLife(ComponentLife* components, uint8_t& count) {
    count = 0;
    
    components[count++] = analyzePump();
    components[count++] = analyzeMotor();
    components[count++] = analyzeSeal();
    components[count++] = analyzeValve();
    components[count++] = analyzeSensor();
}

ComponentLife AdvancedAnalyzer::analyzePump() {
    ComponentLife comp;
    comp.name = "Vacuum Pump";
    comp.totalHours = stats.uptime / 3600;
    comp.ratedLifeHours = PUMP_RATED_LIFE;
    comp.healthScore = calculatePumpHealth();
    comp.remainingLife = max(0.0f, 100.0f - (comp.totalHours * 100.0f / comp.ratedLifeHours));
    
    uint32_t hoursLeft = (comp.ratedLifeHours > comp.totalHours) ? 
                        (comp.ratedLifeHours - comp.totalHours) : 0;
    comp.daysToReplacement = hoursLeft / 24;
    
    return comp;
}

ComponentLife AdvancedAnalyzer::analyzeMotor() {
    ComponentLife comp;
    comp.name = "Motor";
    comp.totalHours = stats.uptime / 3600;
    comp.ratedLifeHours = MOTOR_RATED_LIFE;
    comp.healthScore = calculateMotorHealth();
    comp.remainingLife = max(0.0f, 100.0f - (comp.totalHours * 100.0f / comp.ratedLifeHours));
    
    uint32_t hoursLeft = (comp.ratedLifeHours > comp.totalHours) ? 
                        (comp.ratedLifeHours - comp.totalHours) : 0;
    comp.daysToReplacement = hoursLeft / 24;
    
    return comp;
}

ComponentLife AdvancedAnalyzer::analyzeSeal() {
    ComponentLife comp;
    comp.name = "Vacuum Seal";
    comp.totalHours = stats.uptime / 3600;
    comp.ratedLifeHours = SEAL_RATED_LIFE;
    comp.healthScore = calculateSealHealth();
    comp.remainingLife = max(0.0f, 100.0f - (comp.totalHours * 100.0f / comp.ratedLifeHours));
    
    uint32_t hoursLeft = (comp.ratedLifeHours > comp.totalHours) ? 
                        (comp.ratedLifeHours - comp.totalHours) : 0;
    comp.daysToReplacement = hoursLeft / 24;
    
    return comp;
}

ComponentLife AdvancedAnalyzer::analyzeValve() {
    ComponentLife comp;
    comp.name = "Solenoid Valve";
    comp.totalHours = stats.uptime / 3600;
    comp.ratedLifeHours = VALVE_RATED_LIFE;
    comp.healthScore = calculateValveHealth();
    comp.remainingLife = max(0.0f, 100.0f - (comp.totalHours * 100.0f / comp.ratedLifeHours));
    
    uint32_t hoursLeft = (comp.ratedLifeHours > comp.totalHours) ? 
                        (comp.ratedLifeHours - comp.totalHours) : 0;
    comp.daysToReplacement = hoursLeft / 24;
    
    return comp;
}

ComponentLife AdvancedAnalyzer::analyzeSensor() {
    ComponentLife comp;
    comp.name = "Pressure Sensor";
    comp.totalHours = stats.uptime / 3600;
    comp.ratedLifeHours = SENSOR_RATED_LIFE;
    comp.healthScore = calculateSensorHealth();
    comp.remainingLife = max(0.0f, 100.0f - (comp.totalHours * 100.0f / comp.ratedLifeHours));
    
    uint32_t hoursLeft = (comp.ratedLifeHours > comp.totalHours) ? 
                        (comp.ratedLifeHours - comp.totalHours) : 0;
    comp.daysToReplacement = hoursLeft / 24;
    
    return comp;
}

// ─────────────────── 부품 건강도 계산 ────────────────────────

float AdvancedAnalyzer::calculatePumpHealth() {
    float avgPressure = abs(sensorData.pressure);
    float targetPressure = abs(config.targetPressure);
    
    float efficiency = (targetPressure > 0) ? (avgPressure / targetPressure) * 100.0f : 100.0f;
    
    uint32_t hours = stats.uptime / 3600;
    float agingFactor = 100.0f - (hours * 100.0f / PUMP_RATED_LIFE);
    
    return (efficiency * 0.7f + agingFactor * 0.3f);
}

float AdvancedAnalyzer::calculateMotorHealth() {
    float temp = sensorData.temperature;
    float current = sensorData.current;
    
    float tempHealth = (temp < 40.0f) ? 100.0f : (100.0f - (temp - 40.0f) * 2.0f);
    float currentHealth = (current < 4.0f) ? 100.0f : (100.0f - (current - 4.0f) * 10.0f);
    
    uint32_t hours = stats.uptime / 3600;
    float agingFactor = 100.0f - (hours * 100.0f / MOTOR_RATED_LIFE);
    
    return (tempHealth * 0.4f + currentHealth * 0.3f + agingFactor * 0.3f);
}

float AdvancedAnalyzer::calculateSealHealth() {
    #ifdef ENABLE_DATA_LOGGING
    TrendStatistics trend = dataLogger.getDailyTrend();
    float volatility = trend.volatility;
    #else
    float volatility = 5.0f;
    #endif
    
    float stabilityHealth = (volatility < 5.0f) ? 100.0f : (100.0f - volatility * 5.0f);
    
    uint32_t hours = stats.uptime / 3600;
    float agingFactor = 100.0f - (hours * 100.0f / SEAL_RATED_LIFE);
    
    return (stabilityHealth * 0.6f + agingFactor * 0.4f);
}

float AdvancedAnalyzer::calculateValveHealth() {
    uint32_t hours = stats.uptime / 3600;
    float agingFactor = 100.0f - (hours * 100.0f / VALVE_RATED_LIFE);
    
    float cycleHealth = (stats.totalCycles < 100000) ? 100.0f : 
                       (100.0f - (stats.totalCycles - 100000) / 1000.0f);
    
    return (agingFactor * 0.5f + cycleHealth * 0.5f);
}

float AdvancedAnalyzer::calculateSensorHealth() {
    float pressure = sensorData.pressure;
    
    bool inRange = (pressure >= -100.0f && pressure <= 0.0f);
    float rangeHealth = inRange ? 100.0f : 50.0f;
    
    uint32_t hours = stats.uptime / 3600;
    float agingFactor = 100.0f - (hours * 100.0f / SENSOR_RATED_LIFE);
    
    return (rangeHealth * 0.7f + agingFactor * 0.3f);
}

// ═══════════════════════════════════════════════════════════════
//  최적화 제안
// ═══════════════════════════════════════════════════════════════

void AdvancedAnalyzer::generateOptimizationSuggestions(OptimizationSuggestion* suggestions, 
                                                       uint8_t& count) {
    count = 0;
    
    // 타이밍 최적화
    if (shouldOptimizeTiming()) {
        OptimizationSuggestion& sug = suggestions[count++];
        strcpy(sug.title, "Cycle Time Optimization");
        strcpy(sug.description, "Reduce vacuum hold time to increase throughput while maintaining quality.");
        sug.estimatedImprovement = 10.0f;
        sug.priority = 3;
    }
    
    // PID 튜닝
    if (shouldOptimizePID()) {
        OptimizationSuggestion& sug = suggestions[count++];
        strcpy(sug.title, "PID Parameter Tuning");
        strcpy(sug.description, "Adjust PID parameters to reduce overshoot and improve stability.");
        sug.estimatedImprovement = 12.0f;
        sug.priority = 4;
    }
    
    // 전력 절감
    if (shouldReducePower()) {
        OptimizationSuggestion& sug = suggestions[count++];
        strcpy(sug.description, "Optimize pump speed during hold phase to reduce power consumption.");
        sug.estimatedImprovement = 15.0f;
        sug.priority = 2;
    }
    
    // 유지보수 주기
    if (shouldIncreaseMaintenance()) {
        OptimizationSuggestion& sug = suggestions[count++];
        strcpy(sug.title, "Maintenance Schedule");
        strcpy(sug.description, "Increase maintenance frequency to prevent failures and extend equipment life.");
        sug.estimatedImprovement = 20.0f;
        sug.priority = 5;
    }
    
    // 항상 포함: 예방 정비
    if (count < 5) {
        OptimizationSuggestion& sug = suggestions[count++];
        strcpy(sug.title, "Preventive Maintenance");
        strcpy(sug.description, "Schedule regular inspections to catch issues before they become failures.");
        sug.estimatedImprovement = 25.0f;
        sug.priority = 5;
    }
}

void AdvancedAnalyzer::suggestMaintenanceSchedule(char* schedule, size_t maxSize) {
    snprintf(schedule, maxSize,
        "Weekly: Check vacuum seals\n"
        "Monthly: Inspect pump and motor\n"
        "Quarterly: Replace filters and seals\n"
        "Annually: Complete overhaul"
    );
}

void AdvancedAnalyzer::suggestOperationalImprovements(char* improvements, size_t maxSize) {
    snprintf(improvements, maxSize,
        "1. Reduce hold time by 10%%\n"
        "2. Enable auto power-save mode\n"
        "3. Optimize PID parameters\n"
        "4. Schedule maintenance during low-use periods"
    );
}

bool AdvancedAnalyzer::shouldOptimizeTiming() {
    return (stats.averageCycleTime > 60.0f);
}

bool AdvancedAnalyzer::shouldOptimizePID() {
    #ifdef ENABLE_DATA_LOGGING
    TrendStatistics trend = dataLogger.getDailyTrend();
    return (trend.volatility > 5.0f);
    #else
    return false;
    #endif
}

bool AdvancedAnalyzer::shouldReducePower() {
    return (stats.averageCurrent > 4.5f);
}

bool AdvancedAnalyzer::shouldIncreaseMaintenance() {
    return (healthMonitor.getHealthScore() < 80.0f);
}

// ═══════════════════════════════════════════════════════════════
//  종합 리포트
// ═══════════════════════════════════════════════════════════════

AnalysisReport AdvancedAnalyzer::generateComprehensiveReport() {
    AnalysisReport report;
    
    report.timestamp = time(nullptr);
    report.currentHealth = healthMonitor.getHealthScore();
    
    #ifdef ENABLE_DATA_LOGGING
    report.predictedHealth7d = dataLogger.predictHealthScore(24 * 7);
    report.predictedHealth30d = dataLogger.predictHealthScore(24 * 30);
    #else
    report.predictedHealth7d = report.currentHealth - 2.0f;
    report.predictedHealth30d = report.currentHealth - 5.0f;
    #endif
    
    // 고장 예측 (상위 3개)
    predictMultipleFailures(report.predictions, 3, report.predictionCount);
    
    // 부품 수명
    analyzeComponentLife(report.components, report.componentCount);
    
    // 최적화 제안
    generateOptimizationSuggestions(report.suggestions, report.suggestionCount);
    
    Serial.println("[AdvancedAnalyzer] 종합 리포트 생성 완료");
    
    return report;
}

void AdvancedAnalyzer::exportReportToSD(const char* filename) {
    if (!sdReady) {
        Serial.println("[AdvancedAnalyzer] SD 카드 없음");
        return;
    }
    
    AnalysisReport report = generateComprehensiveReport();
    
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "/reports/%s", filename);
    
    File file = SD.open(fullPath, FILE_WRITE);
    if (!file) {
        Serial.println("[AdvancedAnalyzer] 리포트 파일 생성 실패");
        return;
    }
    
    // 헤더
    file.println("========================================");
    file.println("ESP32-S3 Vacuum Control System");
    file.println("Advanced Analysis Report");
    file.println("========================================");
    file.println();
    
    // 시간
    char timeStr[32];
    struct tm timeinfo;
    localtime_r((time_t*)&report.timestamp, &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    file.printf("Generated: %s\n\n", timeStr);
    
    // 건강도
    file.println("Health Status:");
    file.printf("  Current: %.1f%%\n", report.currentHealth);
    file.printf("  Predicted (7d): %.1f%%\n", report.predictedHealth7d);
    file.printf("  Predicted (30d): %.1f%%\n", report.predictedHealth30d);
    file.println();
    
    // 고장 예측
    file.println("Failure Predictions:");
    for (uint8_t i = 0; i < report.predictionCount; i++) {
        file.printf("  %d. %s\n", i+1, getFailureTypeName(report.predictions[i].type));
        file.printf("     Confidence: %.1f%%\n", report.predictions[i].confidence);
        file.printf("     Estimated: %lu days\n", report.predictions[i].estimatedDays);
        file.printf("     Recommendation: %s\n", report.predictions[i].recommendation);
        file.println();
    }
    
    // 부품 수명
    file.println("Component Life:");
    for (uint8_t i = 0; i < report.componentCount; i++) {
        file.printf("  %s:\n", report.components[i].name);
        file.printf("    Total hours: %lu / %lu\n", 
                   report.components[i].totalHours,
                   report.components[i].ratedLifeHours);
        file.printf("    Remaining: %.1f%%\n", report.components[i].remainingLife);
        file.printf("    Health: %.1f%%\n", report.components[i].healthScore);
        file.printf("    Replace in: %lu days\n", report.components[i].daysToReplacement);
        file.println();
    }
    
    // 최적화 제안
    file.println("Optimization Suggestions:");
    for (uint8_t i = 0; i < report.suggestionCount; i++) {
        file.printf("  %d. [P%d] %s\n", 
                   i+1, 
                   report.suggestions[i].priority,
                   report.suggestions[i].title);
        file.printf("     %s\n", report.suggestions[i].description);
        file.printf("     Improvement: %.1f%%\n", report.suggestions[i].estimatedImprovement);
        file.println();
    }
    
    file.println("========================================");
    file.close();
    
    Serial.printf("[AdvancedAnalyzer] 리포트 저장: %s\n", fullPath);
}

// ═══════════════════════════════════════════════════════════════
//  패턴 감지
// ═══════════════════════════════════════════════════════════════

bool AdvancedAnalyzer::detectAbnormalPattern(const char* patternType) {
    // 간단한 패턴 감지
    if (strcmp(patternType, "pressure_drop") == 0) {
        return (abs(sensorData.pressure) < abs(config.targetPressure) * 0.7f);
    }
    else if (strcmp(patternType, "temp_rise") == 0) {
        return (sensorData.temperature > TEMP_THRESHOLD_WARNING);
    }
    else if (strcmp(patternType, "current_spike") == 0) {
        return (sensorData.current > CURRENT_THRESHOLD_WARNING);
    }
    
    return false;
}

float AdvancedAnalyzer::calculateDegradationRate() {
    if (baselineTimestamp == 0) return 0.0f;
    
    uint32_t elapsedHours = (time(nullptr) - baselineTimestamp) / 3600;
    if (elapsedHours == 0) return 0.0f;
    
    float currentHealth = healthMonitor.getHealthScore();
    float healthDrop = baselineHealth - currentHealth;
    
    degradationRate = healthDrop / elapsedHours;
    
    return degradationRate;
}

// ═══════════════════════════════════════════════════════════════
//  비용 분석
// ═══════════════════════════════════════════════════════════════

float AdvancedAnalyzer::estimateMaintenanceCost() {
    // 부품별 비용 (USD)
    const float PUMP_COST = 500.0f;
    const float MOTOR_COST = 300.0f;
    const float SEAL_COST = 50.0f;
    const float VALVE_COST = 100.0f;
    const float SENSOR_COST = 150.0f;
    const float LABOR_COST_PER_HOUR = 50.0f;
    
    float totalCost = 0.0f;
    
    // 부품 교체 비용
    ComponentLife pump = analyzePump();
    if (pump.daysToReplacement < 30) totalCost += PUMP_COST;
    
    ComponentLife seal = analyzeSeal();
    if (seal.daysToReplacement < 90) totalCost += SEAL_COST;
    
    // 인건비 (2시간 작업 가정)
    totalCost += LABOR_COST_PER_HOUR * 2.0f;
    
    return totalCost;
}

float AdvancedAnalyzer::estimateDowntimeCost(uint32_t hours) {
    // 시간당 가동 중단 비용 (USD)
    const float DOWNTIME_COST_PER_HOUR = 200.0f;
    
    return hours * DOWNTIME_COST_PER_HOUR;
}

float AdvancedAnalyzer::calculateROI(const char* improvement) {
    // ROI = (이득 - 비용) / 비용 * 100%
    
    if (strcmp(improvement, "timing_optimization") == 0) {
        float cost = 100.0f;  // 튜닝 비용
        float benefit = 1000.0f;  // 연간 생산성 향상
        return (benefit - cost) / cost * 100.0f;
    }
    else if (strcmp(improvement, "power_reduction") == 0) {
        float cost = 200.0f;
        float benefit = 800.0f;  // 연간 전력 절감
        return (benefit - cost) / cost * 100.0f;
    }
    
    return 0.0f;
}

// ═══════════════════════════════════════════════════════════════
//  벤치마킹
// ═══════════════════════════════════════════════════════════════

void AdvancedAnalyzer::setBaseline() {
    baselineHealth = healthMonitor.getHealthScore();
    baselineTimestamp = time(nullptr);
    
    Serial.printf("[AdvancedAnalyzer] 기준선 설정: %.1f%% at %lu\n", 
                 baselineHealth, baselineTimestamp);
}

float AdvancedAnalyzer::compareWithBaseline() {
    float currentHealth = healthMonitor.getHealthScore();
    return currentHealth - baselineHealth;
}

// ═══════════════════════════════════════════════════════════════
//  통계 함수
// ═══════════════════════════════════════════════════════════════

float AdvancedAnalyzer::calculateTrendSlope(float* data, uint16_t count) {
    if (count < 2) return 0.0f;
    
    // 선형 회귀 (단순)
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    
    for (uint16_t i = 0; i < count; i++) {
        sumX += i;
        sumY += data[i];
        sumXY += i * data[i];
        sumX2 += i * i;
    }
    
    float slope = (count * sumXY - sumX * sumY) / (count * sumX2 - sumX * sumX);
    
    return slope;
}

float AdvancedAnalyzer::calculateCorrelation(float* x, float* y, uint16_t count) {
    if (count < 2) return 0.0f;
    
    // 피어슨 상관계수 (단순)
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
    
    for (uint16_t i = 0; i < count; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }
    
    float numerator = count * sumXY - sumX * sumY;
    float denominator = sqrt((count * sumX2 - sumX * sumX) * (count * sumY2 - sumY * sumY));
    
    if (denominator == 0) return 0.0f;
    
    return numerator / denominator;
}

// ═══════════════════════════════════════════════════════════════
//  유틸리티 함수
// ═══════════════════════════════════════════════════════════════

const char* getFailureTypeName(FailureType type) {
    switch (type) {
        case FAILURE_NONE: return "None";
        case FAILURE_PUMP_DEGRADATION: return "Pump Degradation";
        case FAILURE_SEAL_LEAK: return "Seal Leak";
        case FAILURE_MOTOR_BEARING: return "Motor Bearing Wear";
        case FAILURE_VALVE_MALFUNCTION: return "Valve Malfunction";
        case FAILURE_SENSOR_DRIFT: return "Sensor Drift";
        case FAILURE_THERMAL_ISSUE: return "Thermal Issue";
        case FAILURE_ELECTRICAL: return "Electrical Problem";
        case FAILURE_MECHANICAL_WEAR: return "Mechanical Wear";
        default: return "Unknown";
    }
}

const char* getFailureTypeDescription(FailureType type) {
    switch (type) {
        case FAILURE_PUMP_DEGRADATION:
            return "Pump efficiency declining, unable to achieve target vacuum.";
        case FAILURE_SEAL_LEAK:
            return "Seal integrity compromised, causing pressure instability.";
        case FAILURE_MOTOR_BEARING:
            return "Motor bearing wear causing temperature and current increase.";
        case FAILURE_VALVE_MALFUNCTION:
            return "Valve not operating correctly, affecting cycle performance.";
        case FAILURE_SENSOR_DRIFT:
            return "Sensor readings becoming unreliable or out of range.";
        case FAILURE_THERMAL_ISSUE:
            return "Excessive heat generation indicating cooling problems.";
        case FAILURE_ELECTRICAL:
            return "Abnormal current draw suggesting electrical issues.";
        case FAILURE_MECHANICAL_WEAR:
            return "General mechanical degradation from extended use.";
        default:
            return "No specific failure detected.";
    }
}

const char* getFailureTypeRecommendation(FailureType type) {
    switch (type) {
        case FAILURE_PUMP_DEGRADATION:
            return "Inspect pump impeller and replace if worn. Check for blockages.";
        case FAILURE_SEAL_LEAK:
            return "Replace vacuum seals and gaskets. Check for surface damage.";
        case FAILURE_MOTOR_BEARING:
            return "Lubricate or replace motor bearings. Check alignment.";
        case FAILURE_VALVE_MALFUNCTION:
            return "Clean or replace solenoid valve. Verify power supply.";
        case FAILURE_SENSOR_DRIFT:
            return "Calibrate sensors. Replace if drift persists.";
        case FAILURE_THERMAL_ISSUE:
            return "Improve ventilation. Clean cooling fins. Check fan operation.";
        case FAILURE_ELECTRICAL:
            return "Check wiring connections. Inspect motor windings. Test PSU.";
        case FAILURE_MECHANICAL_WEAR:
            return "Perform comprehensive maintenance. Replace worn parts.";
        default:
            return "Continue monitoring system health.";
    }
}
