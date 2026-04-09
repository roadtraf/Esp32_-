// ================================================================
// UnitTest_Framework.cpp  ?? v3.9 ?뚯뒪???꾨젅?꾩썙??援ы쁽
// ================================================================

#ifdef UNIT_TEST_MODE

#include "../include/UnitTest_Framework.h"

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뺤쟻 蹂??珥덇린??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

uint16_t TestFramework::testsPassed = 0;
uint16_t TestFramework::testsFailed = 0;
const char* TestFramework::currentModule = "";

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪???댁꽕??
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void TestFramework::ASSERT(bool condition, const char* testName) {
    if (condition) {
        testsPassed++;
        Serial.printf("  ??[PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ??[FAIL] %s\n", testName);
    }
}

void TestFramework::ASSERT_EQUAL(float expected, float actual, const char* testName, float tolerance) {
    if (abs(expected - actual) < tolerance) {
        testsPassed++;
        Serial.printf("  ??[PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ??[FAIL] %s (expected: %.2f, actual: %.2f)\n", 
                     testName, expected, actual);
    }
}

void TestFramework::ASSERT_EQUAL_INT(int expected, int actual, const char* testName) {
    if (expected == actual) {
        testsPassed++;
        Serial.printf("  ??[PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ??[FAIL] %s (expected: %d, actual: %d)\n", 
                     testName, expected, actual);
    }
}

void TestFramework::ASSERT_STRING(const char* expected, const char* actual, const char* testName) {
    if (strcmp(expected, actual) == 0) {
        testsPassed++;
        Serial.printf("  ??[PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ??[FAIL] %s (expected: '%s', actual: '%s')\n", 
                     testName, expected, actual);
    }
}

void TestFramework::ASSERT_RANGE(float value, float min, float max, const char* testName) {
    if (value >= min && value <= max) {
        testsPassed++;
        Serial.printf("  ??[PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ??[FAIL] %s (value: %.2f, range: [%.2f, %.2f])\n", 
                     testName, value, min, max);
    }
}

void TestFramework::ASSERT_NOT_NULL(void* ptr, const char* testName) {
    if (ptr != nullptr) {
        testsPassed++;
        Serial.printf("  ??[PASS] %s\n", testName);
    } else {
        testsFailed++;
        Serial.printf("  ??[FAIL] %s (pointer is NULL)\n", testName);
    }
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  紐⑤뱢 愿由?
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void TestFramework::beginModule(const char* moduleName) {
    currentModule = moduleName;
    Serial.println();
    Serial.println("?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??);
    Serial.printf(" %s\n", moduleName);
    Serial.println("?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??);
}

void TestFramework::endModule() {
    // 紐⑤뱢 醫낅즺 (?꾩슂??異붽?)
}

void TestFramework::printSummary() {
    Serial.println();
    Serial.println("?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??);
    Serial.println(" ?뚯뒪??寃곌낵 ?붿빟");
    Serial.println("?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??);
    Serial.printf("珥??뚯뒪?? %d\n", testsPassed + testsFailed);
    Serial.printf("???듦낵: %d\n", testsPassed);
    Serial.printf("???ㅽ뙣: %d\n", testsFailed);
    
    if (testsFailed == 0) {
        Serial.println("\n?럦 紐⑤뱺 ?뚯뒪???듦낵!");
    } else {
        Serial.printf("\n?좑툘  %d媛??뚯뒪???ㅽ뙣\n", testsFailed);
    }
    Serial.println("?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??n");
}

void TestFramework::reset() {
    testsPassed = 0;
    testsFailed = 0;
    currentModule = "";
}

// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??
//  ?뚯뒪???щ꼫
// ?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧??

void runAllTests() {
    Serial.println("\n\n");
    Serial.println("?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽");
    Serial.println("??                                     ??);
    Serial.println("??  ESP32-S3 吏꾧났 ?쒖뼱 ?쒖뒪??v3.9    ??);
    Serial.println("??       ?⑥쐞 ?뚯뒪??Suite            ??);
    Serial.println("??                                     ??);
    Serial.println("?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽?댿뻽");
    
    TestFramework::reset();
    
    // 湲곗〈 ?뚯뒪??
    Test_PID().runTests();
    Test_Safety().runTests();
    Test_Sensor().runTests();
    Test_Error().runTests();
    Test_Memory().runTests();
    
    // v3.6+ ?뚯뒪??
    #ifdef ENABLE_PREDICTIVE_MAINTENANCE
    Test_Health().runTests();
    Test_MLPredictor().runTests();
    #endif
    
    // v3.8+ ?뚯뒪??
    #ifdef ENABLE_DATA_LOGGING
    Test_DataLogger().runTests();
    #endif
    
    #ifdef ENABLE_SMART_ALERTS
    Test_SmartAlert().runTests();
    #endif
    
    #ifdef ENABLE_ADVANCED_ANALYSIS
    Test_AdvancedAnalyzer().runTests();
    #endif
    
    // v3.9 ?뚯뒪??
    #ifdef ENABLE_VOICE_ALERTS
    Test_VoiceAlert().runTests();
    #endif
    
    // 寃곌낵 異쒕젰
    TestFramework::printSummary();
}

#endif // UNIT_TEST_MODE
