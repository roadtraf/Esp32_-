// ================================================================
// UnitTest_Framework.h  —  v3.9 모듈화된 테스트 프레임워크
// ================================================================
#pragma once

#ifdef UNIT_TEST_MODE

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  테스트 프레임워크
// ═══════════════════════════════════════════════════════════════

class TestFramework {
public:
    static uint16_t testsPassed;
    static uint16_t testsFailed;
    static const char* currentModule;
    
    // 테스트 어설션
    static void ASSERT(bool condition, const char* testName);
    static void ASSERT_EQUAL(float expected, float actual, const char* testName, float tolerance = 0.01f);
    static void ASSERT_EQUAL_INT(int expected, int actual, const char* testName);
    static void ASSERT_STRING(const char* expected, const char* actual, const char* testName);
    static void ASSERT_RANGE(float value, float min, float max, const char* testName);
    static void ASSERT_NOT_NULL(void* ptr, const char* testName);
    
    // 테스트 모듈 관리
    static void beginModule(const char* moduleName);
    static void endModule();
    
    // 전체 결과
    static void printSummary();
    static void reset();
};

// ═══════════════════════════════════════════════════════════════
//  테스트 모듈 인터페이스
// ═══════════════════════════════════════════════════════════════

class TestModule {
public:
    virtual const char* getName() = 0;
    virtual void runTests() = 0;
};

// ═══════════════════════════════════════════════════════════════
//  개별 테스트 모듈 선언
// ═══════════════════════════════════════════════════════════════

// 기존 테스트
class Test_PID : public TestModule {
public:
    const char* getName() override { return "PID Controller"; }
    void runTests() override;
};

class Test_Safety : public TestModule {
public:
    const char* getName() override { return "Safety Interlock"; }
    void runTests() override;
};

class Test_Sensor : public TestModule {
public:
    const char* getName() override { return "Sensor"; }
    void runTests() override;
};

class Test_Error : public TestModule {
public:
    const char* getName() override { return "Error Handler"; }
    void runTests() override;
};

class Test_Memory : public TestModule {
public:
    const char* getName() override { return "Memory Management"; }
    void runTests() override;
};

// v3.6+ 테스트
class Test_Health : public TestModule {
public:
    const char* getName() override { return "Health Monitor"; }
    void runTests() override;
};

class Test_MLPredictor : public TestModule {
public:
    const char* getName() override { return "ML Predictor"; }
    void runTests() override;
};

// v3.8+ 테스트
#ifdef ENABLE_DATA_LOGGING
class Test_DataLogger : public TestModule {
public:
    const char* getName() override { return "Data Logger"; }
    void runTests() override;
};
#endif

#ifdef ENABLE_SMART_ALERTS
class Test_SmartAlert : public TestModule {
public:
    const char* getName() override { return "Smart Alert"; }
    void runTests() override;
};
#endif

#ifdef ENABLE_ADVANCED_ANALYSIS
class Test_AdvancedAnalyzer : public TestModule {
public:
    const char* getName() override { return "Advanced Analyzer"; }
    void runTests() override;
};
#endif

// v3.9 테스트
#ifdef ENABLE_VOICE_ALERTS
class Test_VoiceAlert : public TestModule {
public:
    const char* getName() override { return "Voice Alert"; }
    void runTests() override;
};
#endif

// ═══════════════════════════════════════════════════════════════
//  테스트 러너
// ═══════════════════════════════════════════════════════════════

void runAllTests();

#endif // UNIT_TEST_MODE
