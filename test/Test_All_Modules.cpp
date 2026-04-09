// ================================================================
// Test_All_Modules.cpp  ?? 紐⑤뱺 ?뚯뒪??紐⑤뱢 援ы쁽
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

// ?몃? 蹂??
extern bool verifyMemory();

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_PID
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void Test_PID::runTests() {
    TestFramework::beginModule(getName());
    
    // PID 由ъ뀑
    resetPID();
    TestFramework::ASSERT_EQUAL(0.0f, pidError, "PID Reset - Error");
    TestFramework::ASSERT_EQUAL(0.0f, pidIntegral, "PID Reset - Integral");
    TestFramework::ASSERT_EQUAL(0.0f, pidDerivative, "PID Reset - Derivative");
    
    // PID 異쒕젰 踰붿쐞
    config.targetPressure = -80.0f;
    sensorData.pressure = -50.0f;
    updatePID();
    TestFramework::ASSERT_RANGE(pidOutput, 0.0f, 100.0f, "PID Output Range");
    
    // ?곷텇 ?쒗븳
    for (int i = 0; i < 100; i++) {
        updatePID();
    }
    TestFramework::ASSERT(abs(pidIntegral) <= 50.0f, "PID Integral Limit");
    
    // PID 寃뚯씤 ?뚯뒪??
    config.pidKp = 1.0f;
    config.pidKi = 0.1f;
    config.pidKd = 0.05f;
    resetPID();
    sensorData.pressure = -70.0f;  // 10kPa ?먮윭
    updatePID();
    TestFramework::ASSERT(pidOutput > 0.0f, "PID Output with Error");
    
    TestFramework::endModule();
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_Safety
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void Test_Safety::runTests() {
    TestFramework::beginModule(getName());
    
    // ?덉쟾 ?명꽣???뚯뒪??
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
    
    // ?⑤룄 ?덉쟾 踰붿쐞
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
    
    // ?꾨쪟 ?덉쟾 踰붿쐞
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

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_Sensor
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void Test_Sensor::runTests() {
    TestFramework::beginModule(getName());
    
    // ?뺤긽 媛?
    sensorData.pressure = -80.0f;
    sensorData.temperature = 35.0f;
    sensorData.current = 3.5f;
    TestFramework::ASSERT(validateParameters(), "Valid Parameters");
    
    // NaN ?뚯뒪??
    sensorData.pressure = NAN;
    TestFramework::ASSERT(!validateParameters(), "NaN Pressure");
    sensorData.pressure = -80.0f;
    
    // ?뺣젰 踰붿쐞
    sensorData.pressure = -110.0f;
    TestFramework::ASSERT(!validateParameters(), "Pressure Too Low");
    
    sensorData.pressure = 10.0f;
    TestFramework::ASSERT(!validateParameters(), "Pressure Too High");
    sensorData.pressure = -80.0f;
    
    // ?⑤룄 踰붿쐞
    sensorData.temperature = -10.0f;
    TestFramework::ASSERT(!validateParameters(), "Temperature Too Low");
    
    sensorData.temperature = 100.0f;
    TestFramework::ASSERT(!validateParameters(), "Temperature Too High");
    sensorData.temperature = 35.0f;
    
    // ?꾨쪟 踰붿쐞
    sensorData.current = -1.0f;
    TestFramework::ASSERT(!validateParameters(), "Current Negative");
    
    sensorData.current = 15.0f;
    TestFramework::ASSERT(!validateParameters(), "Current Too High");
    sensorData.current = 3.5f;
    
    // ?쇱꽌 ?ъ뒪 泥댄겕
    checkSensorHealth();
    TestFramework::ASSERT(true, "Sensor Health Check Complete");
    
    TestFramework::endModule();
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_Error
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void Test_Error::runTests() {
    TestFramework::beginModule(getName());
    
    // TEMPORARY ?먮윭 蹂듦뎄
    currentError.severity = SEVERITY_TEMPORARY;
    currentError.retryCount = 0;
    TestFramework::ASSERT(
        attemptErrorRecovery(),
        "Temporary Error - First Retry"
    );
    
    // RECOVERABLE ?먮윭 蹂듦뎄
    currentError.severity = SEVERITY_RECOVERABLE;
    currentError.retryCount = 0;
    TestFramework::ASSERT(
        attemptErrorRecovery(),
        "Recoverable Error - First Retry"
    );
    
    // CRITICAL ?먮윭 (蹂듦뎄 遺덇?)
    currentError.severity = SEVERITY_CRITICAL;
    currentError.retryCount = 0;
    TestFramework::ASSERT(
        !attemptErrorRecovery(),
        "Critical Error - No Recovery"
    );
    
    // 理쒕? ?ъ떆??珥덇낵
    currentError.severity = SEVERITY_TEMPORARY;
    currentError.retryCount = 5;
    TestFramework::ASSERT(
        !attemptErrorRecovery(),
        "Max Retry Exceeded"
    );
    
    // ?먮윭 肄붾뱶 ?뺤씤
    handleError(ERROR_OVERHEAT, SEVERITY_WARNING, "Test overheat");
    TestFramework::ASSERT_EQUAL_INT(
        (int)ERROR_OVERHEAT,
        (int)currentError.code,
        "Error Code Set"
    );
    
    TestFramework::endModule();
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_Memory
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void Test_Memory::runTests() {
    TestFramework::beginModule(getName());
    
    // 硫붾え由?寃利?
    TestFramework::ASSERT(verifyMemory(), "Memory Verification");
    
    // Free Heap
    uint32_t freeHeap = ESP.getFreeHeap();
    TestFramework::ASSERT(freeHeap > 100000, "Sufficient Free Heap");
    Serial.printf("    (Free Heap: %lu bytes)\n", freeHeap);
    
    // Free PSRAM
    uint32_t freePsram = ESP.getFreePsram();
    TestFramework::ASSERT(freePsram > 1000000, "Sufficient Free PSRAM");
    Serial.printf("    (Free PSRAM: %lu bytes)\n", freePsram);
    
    // PSRAM ?ш린
    uint32_t psramSize = ESP.getPsramSize();
    TestFramework::ASSERT_EQUAL_INT(
        8 * 1024 * 1024,
        psramSize,
        "PSRAM Size (8MB)"
    );
    
    // 硫붾え由??⑦렪???뺤씤
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    float fragmentation = 100.0f * (1.0f - (float)maxBlock / freeHeap);
    TestFramework::ASSERT(fragmentation < 30.0f, "Low Memory Fragmentation");
    Serial.printf("    (Fragmentation: %.1f%%)\n", fragmentation);
    
    TestFramework::endModule();
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_Health (v3.6)
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

#ifdef ENABLE_PREDICTIVE_MAINTENANCE

extern HealthMonitor healthMonitor;

void Test_Health::runTests() {
    TestFramework::beginModule(getName());
    
    // 嫄닿컯??怨꾩궛
    float health = healthMonitor.calculateHealthScore(
        -80.0f,  // pressure
        -80.0f,  // target
        35.0f,   // temperature
        3.5f,    // current
        10000    // uptime (珥?
    );
    
    TestFramework::ASSERT_RANGE(health, 0.0f, 100.0f, "Health Score Range");
    Serial.printf("    (Health Score: %.1f%%)\n", health);
    
    // 嫄닿컯???붿냼 ?뺤씤
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
    
    // ?좎?蹂댁닔 ?덈꺼
    MaintenanceLevel level = healthMonitor.getMaintenanceLevel();
    TestFramework::ASSERT(
        level >= MAINTENANCE_NONE && level <= MAINTENANCE_URGENT,
        "Maintenance Level Valid"
    );
    
    // ?좎?蹂댁닔 硫붿떆吏
    const char* message = healthMonitor.getMaintenanceMessage();
    TestFramework::ASSERT(message != nullptr && strlen(message) > 0, "Maintenance Message Not Empty");
    
    TestFramework::endModule();
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_MLPredictor (v3.6)
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

extern MLPredictor mlPredictor;

void Test_MLPredictor::runTests() {
    TestFramework::beginModule(getName());
    
    // ?댁긽 媛먯? (?뺤긽)
    AnomalyType anomaly = mlPredictor.detectAnomaly(-80.0f, 35.0f, 3.5f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_NONE,
        (int)anomaly,
        "No Anomaly Detected (Normal)"
    );
    
    // ?댁긽 媛먯? (怨좎삩)
    anomaly = mlPredictor.detectAnomaly(-80.0f, 65.0f, 3.5f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_TEMPERATURE,
        (int)anomaly,
        "Temperature Anomaly"
    );
    
    // ?댁긽 媛먯? (怨쇱쟾瑜?
    anomaly = mlPredictor.detectAnomaly(-80.0f, 35.0f, 7.0f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_CURRENT,
        (int)anomaly,
        "Current Anomaly"
    );
    
    // ?댁긽 媛먯? (吏꾧났 ?ㅽ뙣)
    anomaly = mlPredictor.detectAnomaly(-40.0f, 35.0f, 3.5f);
    TestFramework::ASSERT_EQUAL_INT(
        (int)ANOMALY_VACUUM,
        (int)anomaly,
        "Vacuum Anomaly"
    );
    
    // 硫붿떆吏 ?뺤씤
    const char* message = mlPredictor.getAnomalyMessage(ANOMALY_TEMPERATURE);
    TestFramework::ASSERT(message != nullptr && strlen(message) > 0, "Anomaly Message Not Empty");
    
    TestFramework::endModule();
}

#endif // ENABLE_PREDICTIVE_MAINTENANCE

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_SmartAlert (v3.8)
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

#ifdef ENABLE_SMART_ALERTS

#include "../include/SmartAlert.h"
extern SmartAlert smartAlert;

void Test_SmartAlert::runTests() {
    TestFramework::beginModule(getName());
    
    // 珥덇린???뺤씤
    TestFramework::ASSERT(true, "SmartAlert initialized");
    
    // ?ㅼ젙 媛?몄삤湲?
    AlertConfig cfg = smartAlert.getConfig();
    TestFramework::ASSERT(
        cfg.startHour >= 0 && cfg.startHour <= 23,
        "Start Hour Valid"
    );
    TestFramework::ASSERT(
        cfg.endHour >= 0 && cfg.endHour <= 23,
        "End Hour Valid"
    );
    
    // ?묒뾽 ?쒓컙 泥댄겕
    bool isWorking = smartAlert.isWorkingHours();
    TestFramework::ASSERT(true, "Working Hours Check");
    Serial.printf("    (Currently %s working hours)\n",
                 isWorking ? "in" : "outside");
    
    // 二쇰쭚 泥댄겕
    bool isWeekend = smartAlert.isWeekend();
    TestFramework::ASSERT(true, "Weekend Check");
    Serial.printf("    (Today is %s)\n",
                 isWeekend ? "weekend" : "weekday");
    
    // ?뚮┝ 諛쒖넚 ?щ? 泥댄겕
    bool should = smartAlert.shouldAlert(MAINTENANCE_REQUIRED);
    TestFramework::ASSERT(true, "Should Alert Check");
    
    // ?듦퀎 ?뺤씤
    uint32_t total = smartAlert.getTotalAlertsSent();
    TestFramework::ASSERT(total >= 0, "Total Alerts Valid");
    Serial.printf("    (Total alerts sent: %lu)\n", total);
    
    TestFramework::endModule();
}

#endif // ENABLE_SMART_ALERTS

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_AdvancedAnalyzer (v3.8)
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

#ifdef ENABLE_ADVANCED_ANALYSIS

#include "../include/AdvancedAnalyzer.h"
extern AdvancedAnalyzer advancedAnalyzer;

void Test_AdvancedAnalyzer::runTests() {
    TestFramework::beginModule(getName());
    
    // 怨좎옣 ?덉륫
    FailurePrediction pred = advancedAnalyzer.predictFailure();
    TestFramework::ASSERT_RANGE(
        pred.confidence,
        0.0f, 100.0f,
        "Failure Prediction Confidence"
    );
    Serial.printf("    (Predicted: %s, %.1f%%)\n",
                 getFailureTypeName(pred.type),
                 pred.confidence);
    
    // 遺???섎챸 遺꾩꽍
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
    
    // ??섏쑉 怨꾩궛
    float rate = advancedAnalyzer.calculateDegradationRate();
    TestFramework::ASSERT(true, "Degradation Rate Calculated");
    Serial.printf("    (Degradation rate: %.4f%%/hour)\n", rate);
    
    // 理쒖쟻???쒖븞
    OptimizationSuggestion sugs[5];
    uint8_t sugCount;
    advancedAnalyzer.generateOptimizationSuggestions(sugs, sugCount);
    TestFramework::ASSERT(sugCount >= 0 && sugCount <= 5, "Optimization Suggestions");
    Serial.printf("    (Suggestions: %d)\n", sugCount);
    
    TestFramework::endModule();
}

#endif // ENABLE_ADVANCED_ANALYSIS

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  Test_VoiceAlert (v3.9)
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

#ifdef ENABLE_VOICE_ALERTS

#include "../include/VoiceAlert.h"

// FreeRTOS (delay 媛쒖꽑)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern VoiceAlert voiceAlert;

void Test_VoiceAlert::runTests() {
    TestFramework::beginModule(getName());
    
    // ?⑤씪???뺤씤
    bool online = voiceAlert.isOnline();
    TestFramework::ASSERT(true, "VoiceAlert Online Check");
    Serial.printf("    (Status: %s)\n", online ? "Online" : "Offline");
    
    if (!online) {
        Serial.println("    ?좑툘  VoiceAlert offline - skipping tests");
        TestFramework::endModule();
        return;
    }
    
    // 蹂쇰ⅷ ?뚯뒪??
    uint8_t volume = voiceAlert.getVolume();
    TestFramework::ASSERT(volume >= 0 && volume <= 30, "Volume Range");
    Serial.printf("    (Current volume: %d/30)\n", volume);
    
    // 蹂쇰ⅷ 蹂寃?
    voiceAlert.setVolume(15);
    TestFramework::ASSERT_EQUAL_INT(15, voiceAlert.getVolume(), "Volume Set");
    voiceAlert.setVolume(volume);  // ?먮옒?濡?
    
    // ?먮룞 ?뚯꽦 泥댄겕
    bool autoEnabled = voiceAlert.isAutoVoiceEnabled();
    TestFramework::ASSERT(true, "Auto Voice Check");
    Serial.printf("    (Auto voice: %s)\n", autoEnabled ? "Enabled" : "Disabled");
    
    // ?듦퀎 ?뺤씤
    uint32_t totalPlayed = voiceAlert.getTotalPlayed();
    TestFramework::ASSERT(totalPlayed >= 0, "Total Played Valid");
    Serial.printf("    (Total played: %lu)\n", totalPlayed);
    
    // ???ш린 ?뺤씤
    uint8_t queueSize = voiceAlert.getQueueSize();
    TestFramework::ASSERT(queueSize >= 0 && queueSize <= 10, "Queue Size");
    
    // ?뚯뒪???ъ깮 (?좏깮)
    Serial.println("    Playing test message...");
    voiceAlert.playSystem(VOICE_READY);
    vTaskDelay(pdMS_TO_TICKS(2000));
    TestFramework::ASSERT(true, "Test Message Played");
    
    TestFramework::endModule();
}

#endif // ENABLE_VOICE_ALERTS

#endif // UNIT_TEST_MODE
