/*
 * AdvancedAnalyzer_Test.cpp
 * v3.8.3 怨좉툒 遺꾩꽍 ?쒖뒪???뚯뒪??
 */

#include "../include/Config.h"

#ifdef ENABLE_ADVANCED_ANALYSIS

#include "../include/AdvancedAnalyzer.h"
#include "../include/HealthMonitor.h"

// FreeRTOS (delay 媛쒖꽑)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern AdvancedAnalyzer advancedAnalyzer;
extern HealthMonitor healthMonitor;
extern Statistics stats;
extern SensorData sensorData;
extern SystemConfig config;

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪???좏떥由ы떚
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void printTestHeader(const char* testName) {
    Serial.println("\n========================================");
    Serial.printf("TEST: %s\n", testName);
    Serial.println("========================================");
}

void printTestResult(bool passed, const char* testName) {
    if (passed) {
        Serial.printf("[?? PASSED: %s\n", testName);
    } else {
        Serial.printf("[?? FAILED: %s\n", testName);
    }
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??1: 珥덇린???뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_Initialization() {
    printTestHeader("Initialization Test");
    
    advancedAnalyzer.begin();
    
    // 湲곗??좎씠 ?ㅼ젙?섏뿀?붿? ?뺤씤
    float baseline = advancedAnalyzer.compareWithBaseline();
    
    bool passed = (baseline == 0.0f);  // 珥덇린?먮뒗 李⑥씠媛 0
    
    printTestResult(passed, "Initialization");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??2: 怨좎옣 ?덉륫 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_FailurePrediction() {
    printTestHeader("Failure Prediction Test");
    
    // ?뚯뒪???곗씠???ㅼ젙
    sensorData.pressure = -80.0f;  // 紐⑺몴蹂대떎 ??쓬
    sensorData.temperature = 55.0f;  // ?믪? ?⑤룄
    sensorData.current = 5.0f;  // ?믪? ?꾨쪟
    config.targetPressure = -90.0f;
    
    FailurePrediction pred = advancedAnalyzer.predictFailure();
    
    Serial.printf("?덉륫 怨좎옣: %s\n", getFailureTypeName(pred.type));
    Serial.printf("?좊ː?? %.1f%%\n", pred.confidence);
    Serial.printf("?덉긽 諛쒖깮: %lu????n", pred.estimatedDays);
    Serial.printf("?ㅻ챸: %s\n", pred.description);
    Serial.printf("沅뚯옣?ы빆: %s\n", pred.recommendation);
    
    bool passed = (pred.type != FAILURE_NONE && pred.confidence > 0.0f);
    
    printTestResult(passed, "Failure Prediction");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??3: ?ㅼ쨷 怨좎옣 ?덉륫 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_MultipleFailurePredictions() {
    printTestHeader("Multiple Failure Predictions Test");
    
    FailurePrediction predictions[3];
    uint8_t count = 0;
    
    advancedAnalyzer.predictMultipleFailures(predictions, 3, count);
    
    Serial.printf("?덉륫??怨좎옣 ?? %d\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("\n?덉륫 %d:\n", i+1);
        Serial.printf("  ?좏삎: %s\n", getFailureTypeName(predictions[i].type));
        Serial.printf("  ?좊ː?? %.1f%%\n", predictions[i].confidence);
        Serial.printf("  ?덉긽: %lu????n", predictions[i].estimatedDays);
    }
    
    bool passed = (count > 0 && count <= 3);
    
    printTestResult(passed, "Multiple Predictions");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??4: 遺???섎챸 遺꾩꽍 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_ComponentLifeAnalysis() {
    printTestHeader("Component Life Analysis Test");
    
    // ?뚯뒪???곗씠???ㅼ젙 (1000?쒓컙 ?묐룞)
    stats.uptime = 1000 * 3600;
    
    ComponentLife components[5];
    uint8_t count = 0;
    
    advancedAnalyzer.analyzeComponentLife(components, count);
    
    Serial.printf("遺꾩꽍??遺???? %d\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("\n遺??%d: %s\n", i+1, components[i].name);
        Serial.printf("  珥??묐룞?쒓컙: %lu / %lu ?쒓컙\n", 
                     components[i].totalHours, 
                     components[i].ratedLifeHours);
        Serial.printf("  ?붿뿬 ?섎챸: %.1f%%\n", components[i].remainingLife);
        Serial.printf("  嫄닿컯?? %.1f%%\n", components[i].healthScore);
        Serial.printf("  援먯껜源뚯?: %lu??n", components[i].daysToReplacement);
    }
    
    bool passed = (count == 5);
    
    printTestResult(passed, "Component Life Analysis");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??5: 媛쒕퀎 遺??遺꾩꽍 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_IndividualComponentAnalysis() {
    printTestHeader("Individual Component Analysis Test");
    
    ComponentLife pump = advancedAnalyzer.analyzePump();
    ComponentLife motor = advancedAnalyzer.analyzeMotor();
    ComponentLife seal = advancedAnalyzer.analyzeSeal();
    ComponentLife valve = advancedAnalyzer.analyzeValve();
    ComponentLife sensor = advancedAnalyzer.analyzeSensor();
    
    Serial.printf("?뚰봽 嫄닿컯?? %.1f%%\n", pump.healthScore);
    Serial.printf("紐⑦꽣 嫄닿컯?? %.1f%%\n", motor.healthScore);
    Serial.printf("??嫄닿컯?? %.1f%%\n", seal.healthScore);
    Serial.printf("諛몃툕 嫄닿컯?? %.1f%%\n", valve.healthScore);
    Serial.printf("?쇱꽌 嫄닿컯?? %.1f%%\n", sensor.healthScore);
    
    bool passed = (pump.healthScore >= 0.0f && pump.healthScore <= 100.0f &&
                   motor.healthScore >= 0.0f && motor.healthScore <= 100.0f);
    
    printTestResult(passed, "Individual Component Analysis");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??6: 理쒖쟻???쒖븞 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_OptimizationSuggestions() {
    printTestHeader("Optimization Suggestions Test");
    
    // 理쒖쟻?붽? ?꾩슂???곹솴 ?ㅼ젙
    stats.averageCycleTime = 65.0f;  // ?믪? ?ъ씠???쒓컙
    stats.averageCurrent = 4.8f;  // ?믪? ?꾨쪟
    
    OptimizationSuggestion suggestions[5];
    uint8_t count = 0;
    
    advancedAnalyzer.generateOptimizationSuggestions(suggestions, count);
    
    Serial.printf("?쒖븞 ?? %d\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("\n?쒖븞 %d:\n", i+1);
        Serial.printf("  ?쒕ぉ: %s\n", suggestions[i].title);
        Serial.printf("  ?ㅻ챸: %s\n", suggestions[i].description);
        Serial.printf("  ?덉긽 媛쒖꽑: %.1f%%\n", suggestions[i].estimatedImprovement);
        Serial.printf("  ?곗꽑?쒖쐞: %d\n", suggestions[i].priority);
    }
    
    bool passed = (count > 0 && count <= 5);
    
    printTestResult(passed, "Optimization Suggestions");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??7: 醫낇빀 由ы룷???앹꽦 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_ComprehensiveReport() {
    printTestHeader("Comprehensive Report Test");
    
    AnalysisReport report = advancedAnalyzer.generateComprehensiveReport();
    
    Serial.printf("??꾩뒪?ы봽: %lu\n", report.timestamp);
    Serial.printf("?꾩옱 嫄닿컯?? %.1f%%\n", report.currentHealth);
    Serial.printf("7???덉륫: %.1f%%\n", report.predictedHealth7d);
    Serial.printf("30???덉륫: %.1f%%\n", report.predictedHealth30d);
    Serial.printf("怨좎옣 ?덉륫 ?? %d\n", report.predictionCount);
    Serial.printf("遺???? %d\n", report.componentCount);
    Serial.printf("理쒖쟻???쒖븞 ?? %d\n", report.suggestionCount);
    
    bool passed = (report.predictionCount >= 0 && 
                   report.componentCount == 5 &&
                   report.suggestionCount > 0);
    
    printTestResult(passed, "Comprehensive Report");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??8: ?⑦꽩 媛먯? ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_PatternDetection() {
    printTestHeader("Pattern Detection Test");
    
    // ?뺣젰 媛뺥븯 ?⑦꽩
    sensorData.pressure = -50.0f;  // ??? ?뺣젰
    config.targetPressure = -90.0f;
    
    bool pressureDrop = advancedAnalyzer.detectAbnormalPattern("pressure_drop");
    Serial.printf("?뺣젰 媛뺥븯 ?⑦꽩: %s\n", pressureDrop ? "媛먯??? : "?놁쓬");
    
    // ?⑤룄 ?곸듅 ?⑦꽩
    sensorData.temperature = 55.0f;
    bool tempRise = advancedAnalyzer.detectAbnormalPattern("temp_rise");
    Serial.printf("?⑤룄 ?곸듅 ?⑦꽩: %s\n", tempRise ? "媛먯??? : "?놁쓬");
    
    // ?꾨쪟 ?ㅽ뙆?댄겕 ?⑦꽩
    sensorData.current = 5.2f;
    bool currentSpike = advancedAnalyzer.detectAbnormalPattern("current_spike");
    Serial.printf("?꾨쪟 ?ㅽ뙆?댄겕 ?⑦꽩: %s\n", currentSpike ? "媛먯??? : "?놁쓬");
    
    bool passed = (pressureDrop || tempRise || currentSpike);
    
    printTestResult(passed, "Pattern Detection");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??9: 嫄닿컯????섏쑉 怨꾩궛 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_DegradationRate() {
    printTestHeader("Degradation Rate Test");
    
    // 湲곗????ㅼ젙
    advancedAnalyzer.setBaseline();
    
    // ?쒓컙 寃쎄낵 ?쒕??덉씠??(?ㅼ젣濡쒕뒗 ?쒓컙???꾩슂?섎?濡?怨꾩궛留??뺤씤)
    float rate = advancedAnalyzer.calculateDegradationRate();
    
    Serial.printf("嫄닿컯????섏쑉: %.4f%%/hour\n", rate);
    
    bool passed = (rate >= 0.0f);  // ?뚯닔媛 ?꾨땲?댁빞 ??
    
    printTestResult(passed, "Degradation Rate");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??10: 鍮꾩슜 遺꾩꽍 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_CostAnalysis() {
    printTestHeader("Cost Analysis Test");
    
    float maintCost = advancedAnalyzer.estimateMaintenanceCost();
    Serial.printf("?덉긽 ?좎?蹂댁닔 鍮꾩슜: $%.2f\n", maintCost);
    
    float downtimeCost = advancedAnalyzer.estimateDowntimeCost(8);
    Serial.printf("媛??以묐떒 鍮꾩슜 (8?쒓컙): $%.2f\n", downtimeCost);
    
    float timingROI = advancedAnalyzer.calculateROI("timing_optimization");
    Serial.printf("??대컢 理쒖쟻??ROI: %.1f%%\n", timingROI);
    
    float powerROI = advancedAnalyzer.calculateROI("power_reduction");
    Serial.printf("?꾨젰 ?덇컧 ROI: %.1f%%\n", powerROI);
    
    bool passed = (maintCost > 0.0f && downtimeCost > 0.0f);
    
    printTestResult(passed, "Cost Analysis");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??11: SD 由ы룷??????뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_SDReportExport() {
    printTestHeader("SD Report Export Test");
    
    if (!sdReady) {
        Serial.println("[SKIP] SD 移대뱶 ?놁쓬");
        return true;  // ?ㅽ궢? ?듦낵濡?泥섎━
    }
    
    // ?뚯뒪???뚯씪紐?
    advancedAnalyzer.exportReportToSD("test_report.txt");
    
    // ?뚯씪 議댁옱 ?뺤씤
    bool fileExists = SD.exists("/reports/test_report.txt");
    
    if (fileExists) {
        Serial.println("由ы룷???뚯씪 ?앹꽦 ?깃났");
        
        // ?뚯씪 ?댁슜 誘몃━蹂닿린
        File file = SD.open("/reports/test_report.txt", FILE_READ);
        if (file) {
            Serial.println("\n--- 由ы룷??誘몃━蹂닿린 (泥섏쓬 10以? ---");
            int lineCount = 0;
            while (file.available() && lineCount < 10) {
                char lineBuf[256];
                int len = 0;
                while (file.available() && len < (int)sizeof(lineBuf) - 1) {
                    char c = file.read();
                    if (c == '\n') break;
                    lineBuf[len++] = c;
                }
                lineBuf[len] = '\0';
                Serial.println(lineBuf);
                lineCount++;
            }
            Serial.println("--- 誘몃━蹂닿린 ??---\n");
            file.close();
        }
    } else {
        Serial.println("由ы룷???뚯씪 ?앹꽦 ?ㅽ뙣");
    }
    
    bool passed = fileExists;
    
    printTestResult(passed, "SD Report Export");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??12: 踰ㅼ튂留덊궧 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_Benchmarking() {
    printTestHeader("Benchmarking Test");
    
    // 湲곗????ㅼ젙
    advancedAnalyzer.setBaseline();
    
    // 嫄닿컯??蹂???쒕??덉씠??(?뚯뒪?몄슜)
    // ?ㅼ젣濡쒕뒗 ?쒓컙??吏?섏빞 ?섏?留? 諛붾줈 鍮꾧탳
    float difference = advancedAnalyzer.compareWithBaseline();
    
    Serial.printf("湲곗????鍮?李⑥씠: %.1f%%\n", difference);
    
    bool passed = true;  // ??긽 ?깃났 (媛믩쭔 ?뺤씤)
    
    printTestResult(passed, "Benchmarking");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??13: ?듦퀎 ?⑥닔 ?뚯뒪??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_StatisticalFunctions() {
    printTestHeader("Statistical Functions Test");
    
    // ?뚯뒪???곗씠??
    float data1[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float data2[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f};
    
    // 異붿꽭 湲곗슱湲?(?곸듅 異붿꽭)
    float slope = advancedAnalyzer.calculateTrendSlope(data1, 5);
    Serial.printf("異붿꽭 湲곗슱湲? %.2f\n", slope);
    
    // ?곴?怨꾩닔 (?꾨꼍???묒쓽 ?곴?)
    float correlation = advancedAnalyzer.calculateCorrelation(data1, data2, 5);
    Serial.printf("?곴?怨꾩닔: %.2f\n", correlation);
    
    bool passed = (slope > 0.9f && correlation > 0.9f);
    
    printTestResult(passed, "Statistical Functions");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪??14: ?ㅽ듃?덉뒪 ?뚯뒪??(洹뱁븳 媛?
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
bool test_StressTest() {
    printTestHeader("Stress Test (Extreme Values)");
    
    // 洹뱁븳 媛??ㅼ젙
    sensorData.pressure = -120.0f;  // 踰붿쐞 諛?
    sensorData.temperature = 100.0f;  // 留ㅼ슦 ?믪쓬
    sensorData.current = 10.0f;  // 留ㅼ슦 ?믪쓬
    stats.uptime = 50000 * 3600;  // 50,000?쒓컙
    
    // ?щ옒???놁씠 ?ㅽ뻾?섎뒗吏 ?뺤씤
    FailurePrediction pred = advancedAnalyzer.predictFailure();
    ComponentLife pump = advancedAnalyzer.analyzePump();
    float rate = advancedAnalyzer.calculateDegradationRate();
    
    Serial.printf("洹뱁븳 ?곹솴 ?덉륫: %s\n", getFailureTypeName(pred.type));
    Serial.printf("洹뱁븳 ?곹솴 ?뚰봽 嫄닿컯?? %.1f%%\n", pump.healthScore);
    Serial.printf("洹뱁븳 ?곹솴 ??섏쑉: %.4f%%/hour\n", rate);
    
    bool passed = true;  // ?щ옒???놁씠 ?꾨즺?섎㈃ ?듦낵
    
    printTestResult(passed, "Stress Test");
    return passed;
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?꾩껜 ?뚯뒪???ㅽ뻾
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
void runAdvancedAnalyzerTests() {
    Serial.println("\n");
    Serial.println("?붴븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븮");
    Serial.println("?? AdvancedAnalyzer Test Suite v3.8.3   ??);
    Serial.println("?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴");
    Serial.println();
    
    int passedTests = 0;
    int totalTests = 14;
    
    // ?뚯뒪???ㅽ뻾
    if (test_Initialization()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_FailurePrediction()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_MultipleFailurePredictions()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_ComponentLifeAnalysis()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_IndividualComponentAnalysis()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_OptimizationSuggestions()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_ComprehensiveReport()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_PatternDetection()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_DegradationRate()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_CostAnalysis()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_SDReportExport()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_Benchmarking()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_StatisticalFunctions()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (test_StressTest()) passedTests++;
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 寃곌낵 ?붿빟
    Serial.println("\n========================================");
    Serial.println("TEST SUMMARY");
    Serial.println("========================================");
    Serial.printf("Total Tests: %d\n", totalTests);
    Serial.printf("Passed: %d\n", passedTests);
    Serial.printf("Failed: %d\n", totalTests - passedTests);
    Serial.printf("Success Rate: %.1f%%\n", (passedTests * 100.0f) / totalTests);
    Serial.println("========================================\n");
    
    if (passedTests == totalTests) {
        Serial.println("??ALL TESTS PASSED!");
    } else {
        Serial.println("??SOME TESTS FAILED");
    }
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  媛쒕퀎 ?뚯뒪???ㅽ뻾 ?⑥닔 (?쒕━??紐낅졊?댁슜)
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
void runSingleTest(const char* testName) {
    if (strcmp(testName, "init") == 0) {
        test_Initialization();
    }
    else if (strcmp(testName, "failure") == 0) {
        test_FailurePrediction();
    }
    else if (strcmp(testName, "multiple") == 0) {
        test_MultipleFailurePredictions();
    }
    else if (strcmp(testName, "component") == 0) {
        test_ComponentLifeAnalysis();
    }
    else if (strcmp(testName, "individual") == 0) {
        test_IndividualComponentAnalysis();
    }
    else if (strcmp(testName, "optimization") == 0) {
        test_OptimizationSuggestions();
    }
    else if (strcmp(testName, "report") == 0) {
        test_ComprehensiveReport();
    }
    else if (strcmp(testName, "pattern") == 0) {
        test_PatternDetection();
    }
    else if (strcmp(testName, "degradation") == 0) {
        test_DegradationRate();
    }
    else if (strcmp(testName, "cost") == 0) {
        test_CostAnalysis();
    }
    else if (strcmp(testName, "sd") == 0) {
        test_SDReportExport();
    }
    else if (strcmp(testName, "benchmark") == 0) {
        test_Benchmarking();
    }
    else if (strcmp(testName, "stats") == 0) {
        test_StatisticalFunctions();
    }
    else if (strcmp(testName, "stress") == 0) {
        test_StressTest();
    }
    else if (strcmp(testName, "all") == 0) {
        runAdvancedAnalyzerTests();
    }
    else {
        Serial.println("?????녿뒗 ?뚯뒪???대쫫");
        Serial.println("?ъ슜 媛?ν븳 ?뚯뒪??");
        Serial.println("  init, failure, multiple, component, individual");
        Serial.println("  optimization, report, pattern, degradation, cost");
        Serial.println("  sd, benchmark, stats, stress, all");
    }
}

#endif // ENABLE_ADVANCED_ANALYSIS

/*
 * ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
 * ?ъ슜 諛⑸쾿
 * ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
 * 
 * 1. setup()?먯꽌 ?몄텧:
 *    #ifdef ENABLE_ADVANCED_ANALYSIS
 *    runAdvancedAnalyzerTests();
 *    #endif
 * 
 * 2. ?쒕━??紐낅졊?대줈 ?ㅽ뻾:
 *    test:all         - 紐⑤뱺 ?뚯뒪???ㅽ뻾
 *    test:failure     - 怨좎옣 ?덉륫 ?뚯뒪?몃쭔
 *    test:component   - 遺???섎챸 ?뚯뒪?몃쭔
 *    ... ??
 * 
 * 3. main.cpp??handleSerialCommand()??異붽?:
 *    else if (cmd.startsWith("test:")) {
 *      String testName = cmd.substring(5);
 *      runSingleTest(testName.c_str());
 *    }
 * 
 * ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
 */
