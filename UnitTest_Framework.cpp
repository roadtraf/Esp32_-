// ================================================================
// UnitTest_Framework.cpp  —  v3.9 테스트 프레임워크 구현
// ================================================================

#ifdef UNIT_TEST_MODE

#include "../include/UnitTest_Framework.h"

// ═══════════════════════════════════════════════════════════════
//  정적 변수 초기화
// ═══════════════════════════════════════════════════════════════

uint16_t TestFramework::testsPassed = 0;
uint16_t TestFramework::testsFailed = 0;
const char* TestFramework::currentModule = "";

// ═══════════════════════════════════════════════════════════════
//  테스트 어설션
// ═══════════════════════════════════════════════════════════════

void TestFramework::ASSERT(bool condition, const char* testName) {
    if (condition) {
        testsPassed++;
        Serial.printf("  ✓ [PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ✗ [FAIL] %s\n", testName);
    }
}

void TestFramework::ASSERT_EQUAL(float expected, float actual, const char* testName, float tolerance) {
    if (abs(expected - actual) < tolerance) {
        testsPassed++;
        Serial.printf("  ✓ [PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ✗ [FAIL] %s (expected: %.2f, actual: %.2f)\n", 
                     testName, expected, actual);
    }
}

void TestFramework::ASSERT_EQUAL_INT(int expected, int actual, const char* testName) {
    if (expected == actual) {
        testsPassed++;
        Serial.printf("  ✓ [PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ✗ [FAIL] %s (expected: %d, actual: %d)\n", 
                     testName, expected, actual);
    }
}

void TestFramework::ASSERT_STRING(const char* expected, const char* actual, const char* testName) {
    if (strcmp(expected, actual) == 0) {
        testsPassed++;
        Serial.printf("  ✓ [PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ✗ [FAIL] %s (expected: '%s', actual: '%s')\n", 
                     testName, expected, actual);
    }
}

void TestFramework::ASSERT_RANGE(float value, float min, float max, const char* testName) {
    if (value >= min && value <= max) {
        testsPassed++;
        Serial.printf("  ✓ [PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ✗ [FAIL] %s (value: %.2f, range: [%.2f, %.2f])\n", 
                     testName, value, min, max);
    }
}

void TestFramework::ASSERT_NOT_NULL(void* ptr, const char* testName) {
    if (ptr != nullptr) {
        testsPassed++;
        Serial.printf("  ✓ [PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ✗ [FAIL] %s (pointer is NULL)\n", testName);
    }
}

// ═══════════════════════════════════════════════════════════════
//  모듈 관리
// ═══════════════════════════════════════════════════════════════

void TestFramework::beginModule(const char* moduleName) {
    currentModule = moduleName;
    Serial.println();
    Serial.println("═══════════════════════════════════════");
    Serial.printf(" %s\n", moduleName);
    Serial.println("═══════════════════════════════════════");
}

void TestFramework::endModule() {
    // 모듈 종료 (필요시 추가)
}

void TestFramework::printSummary() {
    Serial.println();
    Serial.println("═══════════════════════════════════════");
    Serial.println(" 테스트 결과 요약");
    Serial.println("═══════════════════════════════════════");
    Serial.printf("총 테스트: %d\n", testsPassed + testsFailed);
    Serial.printf("✓ 통과: %d\n", testsPassed);
    Serial.printf("✗ 실패: %d\n", testsFailed);
    
    if (testsFailed == 0) {
        Serial.println("\n🎉 모든 테스트 통과!");
    } else {
        Serial.printf("\n⚠️  %d개 테스트 실패\n", testsFailed);
    }
    Serial.println("═══════════════════════════════════════\n");
}

void TestFramework::reset() {
    testsPassed = 0;
    testsFailed = 0;
    currentModule = "";
}

// ═══════════════════════════════════════════════════════════════
//  테스트 러너
// ═══════════════════════════════════════════════════════════════

void runAllTests() {
    Serial.println("\n\n");
    Serial.println("████████████████████████████████████████");
    Serial.println("█                                      █");
    Serial.println("█   ESP32-S3 진공 제어 시스템 v3.9    █");
    Serial.println("█        단위 테스트 Suite            █");
    Serial.println("█                                      █");
    Serial.println("████████████████████████████████████████");
    
    TestFramework::reset();
    
    // 기존 테스트
    Test_PID().runTests();
    Test_Safety().runTests();
    Test_Sensor().runTests();
    Test_Error().runTests();
    Test_Memory().runTests();
    
    // v3.6+ 테스트
    #ifdef ENABLE_PREDICTIVE_MAINTENANCE
    Test_Health().runTests();
    Test_MLPredictor().runTests();
    #endif
    
    // v3.8+ 테스트
    #ifdef ENABLE_DATA_LOGGING
    Test_DataLogger().runTests();
    #endif
    
    #ifdef ENABLE_SMART_ALERTS
    Test_SmartAlert().runTests();
    #endif
    
    #ifdef ENABLE_ADVANCED_ANALYSIS
    Test_AdvancedAnalyzer().runTests();
    #endif
    
    // v3.9 테스트
    #ifdef ENABLE_VOICE_ALERTS
    Test_VoiceAlert().runTests();
    #endif
    
    // 결과 출력
    TestFramework::printSummary();
}

#endif // UNIT_TEST_MODE
