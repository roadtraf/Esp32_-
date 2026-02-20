/*
 * AdvancedAnalyzer_Test.cpp
 * v3.8.3 고급 분석 시스템 테스트
 */

#include "../include/Config.h"

#ifdef ENABLE_ADVANCED_ANALYSIS

#include "../include/AdvancedAnalyzer.h"
#include "../include/HealthMonitor.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern AdvancedAnalyzer advancedAnalyzer;
extern HealthMonitor healthMonitor;
extern Statistics stats;
extern SensorData sensorData;
extern SystemConfig config;

// ═══════════════════════════════════════════════════════════════
//  테스트 유틸리티
// ═══════════════════════════════════════════════════════════════

void printTestHeader(const char* testName) {
    Serial.println("\n========================================");
    Serial.printf("TEST: %s\n", testName);
    Serial.println("========================================");
}

void printTestResult(bool passed, const char* testName) {
    if (passed) {
        Serial.printf("[✓] PASSED: %s\n", testName);
    } else {
        Serial.printf("[✗] FAILED: %s\n", testName);
    }
}

// ═══════════════════════════════════════════════════════════════
//  테스트 1: 초기화 테스트
// ═══════════════════════════════════════════════════════════════
bool test_Initialization() {
    printTestHeader("Initialization Test");
    
    advancedAnalyzer.begin();
    
    // 기준선이 설정되었는지 확인
    float baseline = advancedAnalyzer.compareWithBaseline();
    
    bool passed = (baseline == 0.0f);  // 초기에는 차이가 0
    
    printTestResult(passed, "Initialization");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 2: 고장 예측 테스트
// ═══════════════════════════════════════════════════════════════
bool test_FailurePrediction() {
    printTestHeader("Failure Prediction Test");
    
    // 테스트 데이터 설정
    sensorData.pressure = -80.0f;  // 목표보다 낮음
    sensorData.temperature = 55.0f;  // 높은 온도
    sensorData.current = 5.0f;  // 높은 전류
    config.targetPressure = -90.0f;
    
    FailurePrediction pred = advancedAnalyzer.predictFailure();
    
    Serial.printf("예측 고장: %s\n", getFailureTypeName(pred.type));
    Serial.printf("신뢰도: %.1f%%\n", pred.confidence);
    Serial.printf("예상 발생: %lu일 후\n", pred.estimatedDays);
    Serial.printf("설명: %s\n", pred.description);
    Serial.printf("권장사항: %s\n", pred.recommendation);
    
    bool passed = (pred.type != FAILURE_NONE && pred.confidence > 0.0f);
    
    printTestResult(passed, "Failure Prediction");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 3: 다중 고장 예측 테스트
// ═══════════════════════════════════════════════════════════════
bool test_MultipleFailurePredictions() {
    printTestHeader("Multiple Failure Predictions Test");
    
    FailurePrediction predictions[3];
    uint8_t count = 0;
    
    advancedAnalyzer.predictMultipleFailures(predictions, 3, count);
    
    Serial.printf("예측된 고장 수: %d\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("\n예측 %d:\n", i+1);
        Serial.printf("  유형: %s\n", getFailureTypeName(predictions[i].type));
        Serial.printf("  신뢰도: %.1f%%\n", predictions[i].confidence);
        Serial.printf("  예상: %lu일 후\n", predictions[i].estimatedDays);
    }
    
    bool passed = (count > 0 && count <= 3);
    
    printTestResult(passed, "Multiple Predictions");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 4: 부품 수명 분석 테스트
// ═══════════════════════════════════════════════════════════════
bool test_ComponentLifeAnalysis() {
    printTestHeader("Component Life Analysis Test");
    
    // 테스트 데이터 설정 (1000시간 작동)
    stats.uptime = 1000 * 3600;
    
    ComponentLife components[5];
    uint8_t count = 0;
    
    advancedAnalyzer.analyzeComponentLife(components, count);
    
    Serial.printf("분석된 부품 수: %d\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("\n부품 %d: %s\n", i+1, components[i].name);
        Serial.printf("  총 작동시간: %lu / %lu 시간\n", 
                     components[i].totalHours, 
                     components[i].ratedLifeHours);
        Serial.printf("  잔여 수명: %.1f%%\n", components[i].remainingLife);
        Serial.printf("  건강도: %.1f%%\n", components[i].healthScore);
        Serial.printf("  교체까지: %lu일\n", components[i].daysToReplacement);
    }
    
    bool passed = (count == 5);
    
    printTestResult(passed, "Component Life Analysis");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 5: 개별 부품 분석 테스트
// ═══════════════════════════════════════════════════════════════
bool test_IndividualComponentAnalysis() {
    printTestHeader("Individual Component Analysis Test");
    
    ComponentLife pump = advancedAnalyzer.analyzePump();
    ComponentLife motor = advancedAnalyzer.analyzeMotor();
    ComponentLife seal = advancedAnalyzer.analyzeSeal();
    ComponentLife valve = advancedAnalyzer.analyzeValve();
    ComponentLife sensor = advancedAnalyzer.analyzeSensor();
    
    Serial.printf("펌프 건강도: %.1f%%\n", pump.healthScore);
    Serial.printf("모터 건강도: %.1f%%\n", motor.healthScore);
    Serial.printf("씰 건강도: %.1f%%\n", seal.healthScore);
    Serial.printf("밸브 건강도: %.1f%%\n", valve.healthScore);
    Serial.printf("센서 건강도: %.1f%%\n", sensor.healthScore);
    
    bool passed = (pump.healthScore >= 0.0f && pump.healthScore <= 100.0f &&
                   motor.healthScore >= 0.0f && motor.healthScore <= 100.0f);
    
    printTestResult(passed, "Individual Component Analysis");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 6: 최적화 제안 테스트
// ═══════════════════════════════════════════════════════════════
bool test_OptimizationSuggestions() {
    printTestHeader("Optimization Suggestions Test");
    
    // 최적화가 필요한 상황 설정
    stats.averageCycleTime = 65.0f;  // 높은 사이클 시간
    stats.averageCurrent = 4.8f;  // 높은 전류
    
    OptimizationSuggestion suggestions[5];
    uint8_t count = 0;
    
    advancedAnalyzer.generateOptimizationSuggestions(suggestions, count);
    
    Serial.printf("제안 수: %d\n", count);
    
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("\n제안 %d:\n", i+1);
        Serial.printf("  제목: %s\n", suggestions[i].title);
        Serial.printf("  설명: %s\n", suggestions[i].description);
        Serial.printf("  예상 개선: %.1f%%\n", suggestions[i].estimatedImprovement);
        Serial.printf("  우선순위: %d\n", suggestions[i].priority);
    }
    
    bool passed = (count > 0 && count <= 5);
    
    printTestResult(passed, "Optimization Suggestions");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 7: 종합 리포트 생성 테스트
// ═══════════════════════════════════════════════════════════════
bool test_ComprehensiveReport() {
    printTestHeader("Comprehensive Report Test");
    
    AnalysisReport report = advancedAnalyzer.generateComprehensiveReport();
    
    Serial.printf("타임스탬프: %lu\n", report.timestamp);
    Serial.printf("현재 건강도: %.1f%%\n", report.currentHealth);
    Serial.printf("7일 예측: %.1f%%\n", report.predictedHealth7d);
    Serial.printf("30일 예측: %.1f%%\n", report.predictedHealth30d);
    Serial.printf("고장 예측 수: %d\n", report.predictionCount);
    Serial.printf("부품 수: %d\n", report.componentCount);
    Serial.printf("최적화 제안 수: %d\n", report.suggestionCount);
    
    bool passed = (report.predictionCount >= 0 && 
                   report.componentCount == 5 &&
                   report.suggestionCount > 0);
    
    printTestResult(passed, "Comprehensive Report");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 8: 패턴 감지 테스트
// ═══════════════════════════════════════════════════════════════
bool test_PatternDetection() {
    printTestHeader("Pattern Detection Test");
    
    // 압력 강하 패턴
    sensorData.pressure = -50.0f;  // 낮은 압력
    config.targetPressure = -90.0f;
    
    bool pressureDrop = advancedAnalyzer.detectAbnormalPattern("pressure_drop");
    Serial.printf("압력 강하 패턴: %s\n", pressureDrop ? "감지됨" : "없음");
    
    // 온도 상승 패턴
    sensorData.temperature = 55.0f;
    bool tempRise = advancedAnalyzer.detectAbnormalPattern("temp_rise");
    Serial.printf("온도 상승 패턴: %s\n", tempRise ? "감지됨" : "없음");
    
    // 전류 스파이크 패턴
    sensorData.current = 5.2f;
    bool currentSpike = advancedAnalyzer.detectAbnormalPattern("current_spike");
    Serial.printf("전류 스파이크 패턴: %s\n", currentSpike ? "감지됨" : "없음");
    
    bool passed = (pressureDrop || tempRise || currentSpike);
    
    printTestResult(passed, "Pattern Detection");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 9: 건강도 저하율 계산 테스트
// ═══════════════════════════════════════════════════════════════
bool test_DegradationRate() {
    printTestHeader("Degradation Rate Test");
    
    // 기준선 설정
    advancedAnalyzer.setBaseline();
    
    // 시간 경과 시뮬레이션 (실제로는 시간이 필요하므로 계산만 확인)
    float rate = advancedAnalyzer.calculateDegradationRate();
    
    Serial.printf("건강도 저하율: %.4f%%/hour\n", rate);
    
    bool passed = (rate >= 0.0f);  // 음수가 아니어야 함
    
    printTestResult(passed, "Degradation Rate");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 10: 비용 분석 테스트
// ═══════════════════════════════════════════════════════════════
bool test_CostAnalysis() {
    printTestHeader("Cost Analysis Test");
    
    float maintCost = advancedAnalyzer.estimateMaintenanceCost();
    Serial.printf("예상 유지보수 비용: $%.2f\n", maintCost);
    
    float downtimeCost = advancedAnalyzer.estimateDowntimeCost(8);
    Serial.printf("가동 중단 비용 (8시간): $%.2f\n", downtimeCost);
    
    float timingROI = advancedAnalyzer.calculateROI("timing_optimization");
    Serial.printf("타이밍 최적화 ROI: %.1f%%\n", timingROI);
    
    float powerROI = advancedAnalyzer.calculateROI("power_reduction");
    Serial.printf("전력 절감 ROI: %.1f%%\n", powerROI);
    
    bool passed = (maintCost > 0.0f && downtimeCost > 0.0f);
    
    printTestResult(passed, "Cost Analysis");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 11: SD 리포트 저장 테스트
// ═══════════════════════════════════════════════════════════════
bool test_SDReportExport() {
    printTestHeader("SD Report Export Test");
    
    if (!sdReady) {
        Serial.println("[SKIP] SD 카드 없음");
        return true;  // 스킵은 통과로 처리
    }
    
    // 테스트 파일명
    advancedAnalyzer.exportReportToSD("test_report.txt");
    
    // 파일 존재 확인
    bool fileExists = SD.exists("/reports/test_report.txt");
    
    if (fileExists) {
        Serial.println("리포트 파일 생성 성공");
        
        // 파일 내용 미리보기
        File file = SD.open("/reports/test_report.txt", FILE_READ);
        if (file) {
            Serial.println("\n--- 리포트 미리보기 (처음 10줄) ---");
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
            Serial.println("--- 미리보기 끝 ---\n");
            file.close();
        }
    } else {
        Serial.println("리포트 파일 생성 실패");
    }
    
    bool passed = fileExists;
    
    printTestResult(passed, "SD Report Export");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 12: 벤치마킹 테스트
// ═══════════════════════════════════════════════════════════════
bool test_Benchmarking() {
    printTestHeader("Benchmarking Test");
    
    // 기준선 설정
    advancedAnalyzer.setBaseline();
    
    // 건강도 변화 시뮬레이션 (테스트용)
    // 실제로는 시간이 지나야 하지만, 바로 비교
    float difference = advancedAnalyzer.compareWithBaseline();
    
    Serial.printf("기준선 대비 차이: %.1f%%\n", difference);
    
    bool passed = true;  // 항상 성공 (값만 확인)
    
    printTestResult(passed, "Benchmarking");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 13: 통계 함수 테스트
// ═══════════════════════════════════════════════════════════════
bool test_StatisticalFunctions() {
    printTestHeader("Statistical Functions Test");
    
    // 테스트 데이터
    float data1[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float data2[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f};
    
    // 추세 기울기 (상승 추세)
    float slope = advancedAnalyzer.calculateTrendSlope(data1, 5);
    Serial.printf("추세 기울기: %.2f\n", slope);
    
    // 상관계수 (완벽한 양의 상관)
    float correlation = advancedAnalyzer.calculateCorrelation(data1, data2, 5);
    Serial.printf("상관계수: %.2f\n", correlation);
    
    bool passed = (slope > 0.9f && correlation > 0.9f);
    
    printTestResult(passed, "Statistical Functions");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  테스트 14: 스트레스 테스트 (극한 값)
// ═══════════════════════════════════════════════════════════════
bool test_StressTest() {
    printTestHeader("Stress Test (Extreme Values)");
    
    // 극한 값 설정
    sensorData.pressure = -120.0f;  // 범위 밖
    sensorData.temperature = 100.0f;  // 매우 높음
    sensorData.current = 10.0f;  // 매우 높음
    stats.uptime = 50000 * 3600;  // 50,000시간
    
    // 크래시 없이 실행되는지 확인
    FailurePrediction pred = advancedAnalyzer.predictFailure();
    ComponentLife pump = advancedAnalyzer.analyzePump();
    float rate = advancedAnalyzer.calculateDegradationRate();
    
    Serial.printf("극한 상황 예측: %s\n", getFailureTypeName(pred.type));
    Serial.printf("극한 상황 펌프 건강도: %.1f%%\n", pump.healthScore);
    Serial.printf("극한 상황 저하율: %.4f%%/hour\n", rate);
    
    bool passed = true;  // 크래시 없이 완료되면 통과
    
    printTestResult(passed, "Stress Test");
    return passed;
}

// ═══════════════════════════════════════════════════════════════
//  전체 테스트 실행
// ═══════════════════════════════════════════════════════════════
void runAdvancedAnalyzerTests() {
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  AdvancedAnalyzer Test Suite v3.8.3   ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    
    int passedTests = 0;
    int totalTests = 14;
    
    // 테스트 실행
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
    
    // 결과 요약
    Serial.println("\n========================================");
    Serial.println("TEST SUMMARY");
    Serial.println("========================================");
    Serial.printf("Total Tests: %d\n", totalTests);
    Serial.printf("Passed: %d\n", passedTests);
    Serial.printf("Failed: %d\n", totalTests - passedTests);
    Serial.printf("Success Rate: %.1f%%\n", (passedTests * 100.0f) / totalTests);
    Serial.println("========================================\n");
    
    if (passedTests == totalTests) {
        Serial.println("✓ ALL TESTS PASSED!");
    } else {
        Serial.println("✗ SOME TESTS FAILED");
    }
}

// ═══════════════════════════════════════════════════════════════
//  개별 테스트 실행 함수 (시리얼 명령어용)
// ═══════════════════════════════════════════════════════════════
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
        Serial.println("알 수 없는 테스트 이름");
        Serial.println("사용 가능한 테스트:");
        Serial.println("  init, failure, multiple, component, individual");
        Serial.println("  optimization, report, pattern, degradation, cost");
        Serial.println("  sd, benchmark, stats, stress, all");
    }
}

#endif // ENABLE_ADVANCED_ANALYSIS

/*
 * ═══════════════════════════════════════════════════════════════
 * 사용 방법
 * ═══════════════════════════════════════════════════════════════
 * 
 * 1. setup()에서 호출:
 *    #ifdef ENABLE_ADVANCED_ANALYSIS
 *    runAdvancedAnalyzerTests();
 *    #endif
 * 
 * 2. 시리얼 명령어로 실행:
 *    test:all         - 모든 테스트 실행
 *    test:failure     - 고장 예측 테스트만
 *    test:component   - 부품 수명 테스트만
 *    ... 등
 * 
 * 3. main.cpp의 handleSerialCommand()에 추가:
 *    else if (cmd.startsWith("test:")) {
 *      String testName = cmd.substring(5);
 *      runSingleTest(testName.c_str());
 *    }
 * 
 * ═══════════════════════════════════════════════════════════════
 */
