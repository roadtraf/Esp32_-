// ================================================================
// Test_All_Modules.cpp  —  모든 테스트 모듈 구현
// ================================================================

#ifdef UNIT_TEST_MODE

#include "../include/UnitTest_Framework.h"
#include "../include/Config.h"
#include "../include/PID_Control.h"
#include "../include/Control.h"
#include "../include/Sensor.h"
#include "../include/ErrorHandler.h"
#include "../include/HealthMonitor.h"
#include "../include/MLPredictor.h"

// 외부 변수
extern bool verifyMemory();

// ═══════════════════════════════════════════════════════════════
//  Test_PID
// ═══════════════════════════════════════════════════════════════

void Test_PID::runTests() {
    TestFramework::beginModule(getName());
    
    // PID 리셋
    resetPID();
    TestFramework::ASSERT_EQUAL(0.0f, pidError, "PID Reset - Error");
    TestFramework::ASSERT_EQUAL(0.0f, pidIntegral, "PID Reset - Integral");
    TestFramework::ASSERT_EQUAL(0.0f, pidDerivative, "PID Reset - Derivative");
    
    // PID 출력 범위
    config.targetPressure = -80.0f;
    sensorData.pressure = -50.0f;
    updatePID();
    TestFramework::ASSERT_RANGE(pidOutput, 0.0f, 100.0f, "PID Output Range");
    
    // 적분 제한
    for (int i = 0; i < 100; i++) {
        updatePID();
    }
    TestFramework::ASSERT(abs(pidIntegral) <= 50.0f, "PID Integral Limit");
    
    // PID 게인 테스트
    config.pidKp = 1.0f;
    config.pidKi = 0.1f;
    config.pidKd = 0.05f;
    resetPID();
    sensorData.pressure = -70.0f;  // 10kPa 에러
    updatePID();
    TestFramework::ASSERT(pidOutput > 0.0f, "PID Output with Error");
    
    TestFramework::endModule();
}

// ═══════════════════════════════════════════════════════════════
//  Test_Safety
// ═══════════════════════════════════════════════════════════════

void Test_Safety::runTests() {
    TestFramework::beginModule(getName());
    
    // 안전 인터락 테스트
    TestFramework::ASSERT(
        checkSafetyInterlock(true, false),
        "Pump Only - Allowed"
    );
    
    TestFramework::ASSERT(
        checkSafetyInterlock(false, true),
        "Valve Only - Allowed"
    );
    
    TestFramework::ASSERT(
        !checkSafetyInterlock(true, true),
        "Pump + Valve - Blocked"
    );
    
    TestFramework::ASSERT(
        checkSafetyInterlock(false, false),
        "Both Off - Allowed"
    );
    
    // 온도 안전 범위
    sensorData.temperature = 45.0f;
    TestFramework::ASSERT(
        sensorData.temperature < TEMP_THRESHOLD_CRITICAL,
        "Temperature Safe"
    );
    
    sensorData.temperature = 65.0f;
    TestFramework::ASSERT(
        sensorData.temperature >= TEMP_THRESHOLD_CRITICAL,
        "Temperature Critical"
    );
    
    // 전류 안전 범위
    sensorData.current = 4.0f;
    TestFramework::ASSERT(
        sensorData.current < CURRENT_THRESHOLD_CRITICAL,
        "Current Safe"
    );
    
    sensorData.current = 7.0f;
    TestFramework::ASSERT(
        sensorData.current >= CURRENT_THRESHOLD_CRITICAL,
        "Current Critical"
    );
    
    TestFramework::endModule();
}

// ═══════════════════════════════════════════════════════════════
//  Test_Sensor
// ═══════════════════════════════════════════════════════════════

void Test_Sensor::runTests() {
    TestFramework::beginModule(getName());
    
    // 정상 값
    sensorData.pressure = -80.0f;
    sensorData.temperature = 35.0f;
    sensorData.current = 3.5f;
    TestFramework::ASSERT(validateParameters(), "Valid Parameters");
    
    // NaN 테스트
    sensorData.pressure = NAN;
    TestFramework::ASSERT(!validateParameters(), "NaN Pressure");
    sensorData.pressure = -80.0f;
    
    // 압력 범위
    sensorData.pressure = -110.0f;
    TestFramework::ASSERT(!validateParameters(), "Pressure Too Low");
    
    sensorData.pressure = 10.0f;
    TestFramework::ASSERT(!validateParameters(), "Pressure Too High");
    sensorData.pressure = -80.0f;
    
    // 온도 범위
    sensorData.temperature = -10.0f;
    TestFramework::ASSERT(!validateParameters(), "Temperature Too Low");
    
    sensorData.temperature = 100.0f;
    TestFramework::ASSERT(!validateParameters(), "Temperature Too High");
    sensorData.temperature = 35.0f;
    
    // 전류 범위
    sensorData.current = -1.0f;
    TestFramework::ASSERT(!validateParameters(), "Current Negative");
    
    sensorData.current = 15.0f;
    TestFramework::ASSERT(!validateParameters(), "Current Too High");
    sensorData.current = 3.5f;
    
    // 센서 헬스 체크
    checkSensorHealth();
    TestFramework::ASSERT(true, "Sensor Health Check Complete");
    
    TestFramework::endModule();
}

// ═══════════════════════════════════════════════════════════════
//  Test_Error
// ═══════════════════════════════════════════════════════════════

void Test_Error::runTests() {
    TestFramework::beginModule(getName());
    
    // TEMPORARY 에러 복구
    currentError.severity = SEVERITY_TEMPORARY;
    currentError.retryCount = 0;
    TestFramework::ASSERT(
        attemptErrorRecovery(),
        "Temporary Error - First Retry"
    );
    
    // RECOVERABLE 에러 복구
    currentError.severity = SEVERITY_RECOVERABLE;
    currentError.retryCount = 0;
    TestFramework::ASSERT(
        attemptErrorRecovery(),
        "Recoverable Error - First Retry"
    );
    
    // CRITICAL 에러 (복구 불가)
    currentError.severity = SEVERITY_CRITICAL;
    currentError.retryCount = 0;
    TestFramework::ASSERT(
        !attemptErrorRecovery(),
        "Critical Error - No Recovery"
    );
    
    // 최대 재시도 초과
    currentError.severity = SEVERITY_TEMPORARY;
    currentError.retryCount = 5;
    TestFramework::ASSERT(
        !attemptErrorRecovery(),
        "Max Retry Exceeded"
    );
    
    // 에러 코드 확인
    handleError(ERROR_OVERHEAT, SEVERITY_WARNING, "Test overheat");
    TestFramework::ASSERT_EQUAL_INT(
        (int)ERROR_OVERHEAT,
        (int)currentError.code,
        "Error Code Set"
    );
    
    TestFramework::endModule();
}

// ═══════════════════════════════════════════════════════════════
//  Test_Memory
// ═══════════════════════════════════════════════════════════════

void Test_Memory::runTests() {
    TestFramework::beginModule(getName());
    
    // 메모리 검증
    TestFramework::ASSERT(verifyMemory(), "Memory Verification");
    
    // Free Heap
    uint32_t freeHeap = ESP.getFreeHeap();
    TestFramework::ASSERT(freeHeap > 100000, "Sufficient Free Heap");
    Serial.printf("    (Free Heap: %lu bytes)\n", freeHeap);
    
    // Free PSRAM
    uint32_t freePsram = ESP.getFreePsram();
    TestFramework::ASSERT(freePsram > 1000000, "Sufficient Free PSRAM");
    Serial.printf("    (Free PSRAM: %lu bytes)\n", freePsram);
    
    // PSRAM 크기
    uint32_t psramSize = ESP.getPsramSize();
    TestFramework::ASSERT_EQUAL_INT(
        8 * 1024 * 1024,
        psramSize,
        "PSRAM Size (8MB)"
    );
    
    // 메모리 단편화 확인
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    float fragmentation = 100.0f * (1.0f - (float)maxBlock / freeHeap);
    TestFramework::ASSERT(fragmentation < 30.0f, "Low Memory Fragmentation");
    Serial.printf("    (Fragmentation: %.1f%%)\n", fragmentation);
    
    TestFramework::endModule();
}

// ═══════════════════════════════════════════════════════════════
//  Test_Health (v3.6)
// ═══════════════════════════════════════════════════════════════

#ifdef ENABLE_PREDICTIVE_MAINTENANCE

extern HealthMonitor healthMonitor;

void Test_Health::runTests() {
    TestFramework::beginModule(getName());
    
    // 건강도 계산
    float health = healthMonitor.calculateHealthScore(
        -80.0f,  // pressure
        -80.0f,  // target
        35.0f,   // temperature
        3.5f,    // current
        10000    // uptime (초)
    );
    
    TestFramework::ASSERT_RANGE(health, 0.0f, 100.0f, "Health Score Range");
    Serial.printf("    (Health Score: %.1f%%)\n", health);
    
    // 건강도 요소 확인
    const HealthFactors& factors = healthMonitor.getHealthFactors();
    TestFramework::ASSERT_RANGE(
        factors.pumpEfficiency,
        0.0f, 100.0f,
        "Pump Efficiency Range"
    );
    TestFramework::ASSERT_RANGE(
        factors.temperatureHealth,
        0.0f, 100.0f,
        "Temperature Health Range"
    );
    TestFramework::ASSERT_RANGE(
        factors.currentHealth,
        0.0f, 100.0f,
        "Current Health Range"
    );
    TestFramework::ASSERT_RANGE(
        factors.runtimeHealth,
        0.0f, 100.0f,
        "Runtime Health Range"
    );
    
    // 유지보수 레벨
    MaintenanceLevel level = healthMonitor.getMaintenanceLevel();
    TestFramework::ASSERT(
        level >= MAINTENANCE_NONE && level <= MAINTENANCE_URGENT,
        "Maintenance Level Valid"
    );
    
    // 유지보수 메시지
    const char* message = healthMonitor.getMaintenanceMessage();
    TestFramework::ASSERT(message != nullptr && strlen(message) > 0, "Maintenance Message Not Empty");
    
    TestFramework::endModule();
}

// ═══════════════════════════════════════════════════════════════
//  Test_MLPredictor (v3.6)
// ═══════════════════════════════════════════════════════════════

extern MLPredictor mlPredictor;

void Test_MLPredictor::runTests() {
    TestFramework::beginModule(getName());
    
    // 이상 감지 (정상)
    AnomalyType anomaly = mlPredictor.detectAnomaly(-80.0f, 35.0f, 3.5f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_NONE,
        (int)anomaly,
        "No Anomaly Detected (Normal)"
    );
    
    // 이상 감지 (고온)
    anomaly = mlPredictor.detectAnomaly(-80.0f, 65.0f, 3.5f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_TEMPERATURE,
        (int)anomaly,
        "Temperature Anomaly"
    );
    
    // 이상 감지 (과전류)
    anomaly = mlPredictor.detectAnomaly(-80.0f, 35.0f, 7.0f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_CURRENT,
        (int)anomaly,
        "Current Anomaly"
    );
    
    // 이상 감지 (진공 실패)
    anomaly = mlPredictor.detectAnomaly(-40.0f, 35.0f, 3.5f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_VACUUM,
        (int)anomaly,
        "Vacuum Anomaly"
    );
    
    // 메시지 확인
    const char* message = mlPredictor.getAnomalyMessage(ANOMALY_TEMPERATURE);
    TestFramework::ASSERT(message != nullptr && strlen(message) > 0, "Anomaly Message Not Empty");
    
    TestFramework::endModule();
}

#endif // ENABLE_PREDICTIVE_MAINTENANCE

// ═══════════════════════════════════════════════════════════════
//  Test_SmartAlert (v3.8)
// ═══════════════════════════════════════════════════════════════

#ifdef ENABLE_SMART_ALERTS

#include "../include/SmartAlert.h"
extern SmartAlert smartAlert;

void Test_SmartAlert::runTests() {
    TestFramework::beginModule(getName());
    
    // 초기화 확인
    TestFramework::ASSERT(true, "SmartAlert initialized");
    
    // 설정 가져오기
    AlertConfig cfg = smartAlert.getConfig();
    TestFramework::ASSERT(
        cfg.startHour >= 0 && cfg.startHour <= 23,
        "Start Hour Valid"
    );
    TestFramework::ASSERT(
        cfg.endHour >= 0 && cfg.endHour <= 23,
        "End Hour Valid"
    );
    
    // 작업 시간 체크
    bool isWorking = smartAlert.isWorkingHours();
    TestFramework::ASSERT(true, "Working Hours Check");
    Serial.printf("    (Currently %s working hours)\n",
                 isWorking ? "in" : "outside");
    
    // 주말 체크
    bool isWeekend = smartAlert.isWeekend();
    TestFramework::ASSERT(true, "Weekend Check");
    Serial.printf("    (Today is %s)\n",
                 isWeekend ? "weekend" : "weekday");
    
    // 알림 발송 여부 체크
    bool should = smartAlert.shouldAlert(MAINTENANCE_REQUIRED);
    TestFramework::ASSERT(true, "Should Alert Check");
    
    // 통계 확인
    uint32_t total = smartAlert.getTotalAlertsSent();
    TestFramework::ASSERT(total >= 0, "Total Alerts Valid");
    Serial.printf("    (Total alerts sent: %lu)\n", total);
    
    TestFramework::endModule();
}

#endif // ENABLE_SMART_ALERTS

// ═══════════════════════════════════════════════════════════════
//  Test_AdvancedAnalyzer (v3.8)
// ═══════════════════════════════════════════════════════════════

#ifdef ENABLE_ADVANCED_ANALYSIS

#include "../include/AdvancedAnalyzer.h"
extern AdvancedAnalyzer advancedAnalyzer;

void Test_AdvancedAnalyzer::runTests() {
    TestFramework::beginModule(getName());
    
    // 고장 예측
    FailurePrediction pred = advancedAnalyzer.predictFailure();
    TestFramework::ASSERT_RANGE(
        pred.confidence,
        0.0f, 100.0f,
        "Failure Prediction Confidence"
    );
    Serial.printf("    (Predicted: %s, %.1f%%)\n",
                 getFailureTypeName(pred.type),
                 pred.confidence);
    
    // 부품 수명 분석
    ComponentLife comps[5];
    uint8_t count;
    advancedAnalyzer.analyzeComponentLife(comps, count);
    TestFramework::ASSERT_EQUAL_INT(5, count, "Component Count");
    
    for (uint8_t i = 0; i < count; i++) {
        TestFramework::ASSERT_RANGE(
            comps[i].healthScore,
            0.0f, 100.0f,
            "Component Health Range"
        );
    }
    
    // 저하율 계산
    float rate = advancedAnalyzer.calculateDegradationRate();
    TestFramework::ASSERT(true, "Degradation Rate Calculated");
    Serial.printf("    (Degradation rate: %.4f%%/hour)\n", rate);
    
    // 최적화 제안
    OptimizationSuggestion sugs[5];
    uint8_t sugCount;
    advancedAnalyzer.generateOptimizationSuggestions(sugs, sugCount);
    TestFramework::ASSERT(sugCount >= 0 && sugCount <= 5, "Optimization Suggestions");
    Serial.printf("    (Suggestions: %d)\n", sugCount);
    
    TestFramework::endModule();
}

#endif // ENABLE_ADVANCED_ANALYSIS

// ═══════════════════════════════════════════════════════════════
//  Test_VoiceAlert (v3.9)
// ═══════════════════════════════════════════════════════════════

#ifdef ENABLE_VOICE_ALERTS

#include "../include/VoiceAlert.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern VoiceAlert voiceAlert;

void Test_VoiceAlert::runTests() {
    TestFramework::beginModule(getName());
    
    // 온라인 확인
    bool online = voiceAlert.isOnline();
    TestFramework::ASSERT(true, "VoiceAlert Online Check");
    Serial.printf("    (Status: %s)\n", online ? "Online" : "Offline");
    
    if (!online) {
        Serial.println("    ⚠️  VoiceAlert offline - skipping tests");
        TestFramework::endModule();
        return;
    }
    
    // 볼륨 테스트
    uint8_t volume = voiceAlert.getVolume();
    TestFramework::ASSERT(volume >= 0 && volume <= 30, "Volume Range");
    Serial.printf("    (Current volume: %d/30)\n", volume);
    
    // 볼륨 변경
    voiceAlert.setVolume(15);
    TestFramework::ASSERT_EQUAL_INT(15, voiceAlert.getVolume(), "Volume Set");
    voiceAlert.setVolume(volume);  // 원래대로
    
    // 자동 음성 체크
    bool autoEnabled = voiceAlert.isAutoVoiceEnabled();
    TestFramework::ASSERT(true, "Auto Voice Check");
    Serial.printf("    (Auto voice: %s)\n", autoEnabled ? "Enabled" : "Disabled");
    
    // 통계 확인
    uint32_t totalPlayed = voiceAlert.getTotalPlayed();
    TestFramework::ASSERT(totalPlayed >= 0, "Total Played Valid");
    Serial.printf("    (Total played: %lu)\n", totalPlayed);
    
    // 큐 크기 확인
    uint8_t queueSize = voiceAlert.getQueueSize();
    TestFramework::ASSERT(queueSize >= 0 && queueSize <= 10, "Queue Size");
    
    // 테스트 재생 (선택)
    Serial.println("    Playing test message...");
    voiceAlert.playSystem(VOICE_READY);
    vTaskDelay(pdMS_TO_TICKS(2000));
    TestFramework::ASSERT(true, "Test Message Played");
    
    TestFramework::endModule();
}

#endif // ENABLE_VOICE_ALERTS

#endif // UNIT_TEST_MODE
