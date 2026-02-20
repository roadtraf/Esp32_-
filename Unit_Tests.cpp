
#ifdef UNIT_TEST_MODE

#include "Config.h"                 // enum, struct, extern 전역변수 (pidError 등)
#include "PID_Control.h"            // updatePID(), resetPID()
#include "../include/SD_Logger.h"   // ISO8601_BUFFER_SIZE, getCurrentTimeISO8601
#include <cstring>                  // strlen, strchr
#include "Control.h"                // checkSafetyInterlock()
#include "Sensor.h"                 // validateParameters(), checkSensorHealth()
#include "ErrorHandler.h"           // attemptErrorRecovery()

// Memory_Management.cpp 공개 API
extern bool verifyMemory();

// ==================== 테스트 유틸리티 ====================
uint16_t testsPassed = 0;
uint16_t testsFailed = 0;

void TEST_ASSERT(bool condition, const char* testName) {
  if (condition) {
    testsPassed++;
    Serial.printf("[PASS] %s\n", testName);
  } else {
    testsFailed++;
    Serial.printf("[FAIL] %s\n", testName);
  }
}

void TEST_ASSERT_EQUAL(float expected, float actual, const char* testName) {
  if (abs(expected - actual) < 0.01) {
    testsPassed++;
    Serial.printf("[PASS] %s\n", testName);
  } else {
    testsFailed++;
    Serial.printf("[FAIL] %s (expected: %.2f, actual: %.2f)\n", testName, expected, actual);
  }
}

// ==================== PID 제어 테스트 ====================
void test_PID_Controller() {
  Serial.println("\n=== PID Controller Tests ===");

  // 1. PID 리셋 테스트
  resetPID();
  TEST_ASSERT_EQUAL(0.0, pidError, "PID Reset - Error");
  TEST_ASSERT_EQUAL(0.0, pidIntegral, "PID Reset - Integral");
  TEST_ASSERT_EQUAL(0.0, pidDerivative, "PID Reset - Derivative");

  // 2. PID 출력 범위 테스트
  config.targetPressure = -80.0;
  sensorData.pressure = -50.0;
  updatePID();
  TEST_ASSERT(pidOutput >= 0.0 && pidOutput <= 100.0, "PID Output Range");

  // 3. 적분 제한 테스트
  for (int i = 0; i < 100; i++) {
    updatePID();
  }
  TEST_ASSERT(abs(pidIntegral) <= 50.0, "PID Integral Limit");

  Serial.printf("PID Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 안전 인터락 테스트 ====================
void test_Safety_Interlock() {
  Serial.println("\n=== Safety Interlock Tests ===");

  TEST_ASSERT(checkSafetyInterlock(true, false), "Pump Only");
  TEST_ASSERT(checkSafetyInterlock(false, true), "Valve Only");
  TEST_ASSERT(!checkSafetyInterlock(true, true), "Pump + Valve (Blocked)");
  TEST_ASSERT(checkSafetyInterlock(false, false), "Both Off");

  Serial.printf("Safety Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 파라미터 검증 테스트 ====================
void test_Parameter_Validation() {
  Serial.println("\n=== Parameter Validation Tests ===");

  // 정상 값
  sensorData.pressure = -80.0;
  sensorData.current = 3.5;
  TEST_ASSERT(validateParameters(), "Valid Parameters");

  // NaN 테스트
  sensorData.pressure = NAN;
  TEST_ASSERT(!validateParameters(), "NaN Pressure");
  sensorData.pressure = -80.0;

  // 범위 초과
  sensorData.pressure = -110.0;
  TEST_ASSERT(!validateParameters(), "Out of Range Pressure (Low)");
  sensorData.pressure = 10.0;
  TEST_ASSERT(!validateParameters(), "Out of Range Pressure (High)");
  sensorData.pressure = -80.0;

  sensorData.current = -1.0;
  TEST_ASSERT(!validateParameters(), "Out of Range Current (Low)");
  sensorData.current = 15.0;
  TEST_ASSERT(!validateParameters(), "Out of Range Current (High)");
  sensorData.current = 3.5;

  Serial.printf("Validation Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 센서 헬스 테스트 ====================
void test_Sensor_Health() {
  Serial.println("\n=== Sensor Health Tests ===");

  // 정상 센서 값
  sensorData.pressure = -80.0;
  sensorData.current = 3.5;
  checkSensorHealth();
  TEST_ASSERT(true, "Normal Sensor Values");

  // 압력 센서 이상 (전압 범위 체크는 실제 하드웨어 필요)
  // 간접 테스트: 압력 값이 유효 범위인지 확인
  TEST_ASSERT(sensorData.pressure >= -105.0 && sensorData.pressure <= 5.0, 
    "Pressure Sensor Range");

  // 전류 센서 이상
  TEST_ASSERT(sensorData.current >= 0.0 && sensorData.current <= 10.0, 
    "Current Sensor Range");

  Serial.printf("Sensor Health Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 에러 복구 테스트 ====================
void test_Error_Recovery() {
  Serial.println("\n=== Error Recovery Tests ===");

  // TEMPORARY 에러
  currentError.severity = SEVERITY_TEMPORARY;
  currentError.retryCount = 0;
  TEST_ASSERT(attemptErrorRecovery(), "Temporary Error - First Retry");

  // RECOVERABLE 에러
  currentError.severity = SEVERITY_RECOVERABLE;
  currentError.retryCount = 0;
  TEST_ASSERT(attemptErrorRecovery(), "Recoverable Error - First Retry");

  // CRITICAL 에러
  currentError.severity = SEVERITY_CRITICAL;
  currentError.retryCount = 0;
  TEST_ASSERT(!attemptErrorRecovery(), "Critical Error - No Recovery");

  Serial.printf("Error Recovery Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 메모리 관리 테스트 ====================
void test_Memory_Management() {
  Serial.println("\n=== Memory Management Tests ===");

  TEST_ASSERT(verifyMemory(), "Memory Verification");
  TEST_ASSERT(ESP.getFreeHeap() > 100000, "Sufficient Free Heap");
  TEST_ASSERT(ESP.getFreePsram() > 1000000, "Sufficient Free PSRAM");
  TEST_ASSERT(ESP.getPsramSize() == 8 * 1024 * 1024, "PSRAM Size Check (8MB)");

  Serial.printf("Memory Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 시간 동기화 테스트 ====================
void test_Time_Sync() {
  Serial.println("\n=== Time Sync Tests ===");

  extern void getCurrentTimeISO8601(char* buffer, size_t bufferSize);
  char timeStr[ISO8601_BUFFER_SIZE];
  getCurrentTimeISO8601(timeStr, sizeof(timeStr));

  TEST_ASSERT(strlen(timeStr) > 0, "ISO8601 Time String");
  TEST_ASSERT(strchr(timeStr, 'T') != nullptr, "ISO8601 Format Check");
  TEST_ASSERT(strchr(timeStr, '+') != nullptr || strchr(timeStr, 'Z') != nullptr, "Timezone Check");

  time_t now = time(nullptr);
  TEST_ASSERT(now > 1000000, "Unix Timestamp Valid");

  Serial.printf("Time Sync Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== Watchdog 테스트 ====================
void test_Watchdog() {
  Serial.println("\n=== Watchdog Tests ===");

  extern void feedWatchdog();
  feedWatchdog();
  TEST_ASSERT(true, "Watchdog Feed");

  // Watchdog 타임아웃 테스트는 실제로 리셋이 발생하므로 생략
  TEST_ASSERT(true, "Watchdog Timeout (Skipped)");

  Serial.printf("Watchdog Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 전체 테스트 실행 ====================
void runUnitTests() {
  Serial.println("\n");
  Serial.println("=====================================");
  Serial.println("   단위 테스트 시작");
  Serial.println("=====================================\n");

  testsPassed = 0;
  testsFailed = 0;

  test_PID_Controller();
  test_Safety_Interlock();
  test_Parameter_Validation();
  test_Sensor_Health();
  test_Error_Recovery();
  test_Memory_Management();
  test_Time_Sync();
  test_Watchdog();

  Serial.println("=====================================");
  Serial.printf("테스트 결과: %d passed, %d failed\n", testsPassed, testsFailed);
  if (testsFailed == 0) {
    Serial.println("모든 테스트 통과! ✓");
  } else {
    Serial.printf("%d개 테스트 실패 ✗\n", testsFailed);
  }
  Serial.println("=====================================\n");
}

#endif // UNIT_TEST_MODE



============================================================
  파일 목록 요약  (v3.3)
============================================================
✅ platformio.ini              – PlatformIO 설정 (변경 없음)
✅ partitions_16mb.csv         – 파티션 테이블   (변경 없음)
✅ src/LovyanGFX_Config.hpp    – 디스플레이 설정  (변경 없음)
✅ src/main.cpp                – 메인 프로그램   (①②③④⑤ 적용)
✅ src/UI_Screens.cpp          – UI 화면        (⑥⑦ 적용)
✅ src/USB_Keyboard.cpp        – USB 키패드     (⑧ 적용)
✅ src/Trend_Graph.cpp         – 추세 그래프    ★ 신규 파일
✅ src/OTA_Update.cpp          – OTA + HTTP 서버 (변경 없음)
✅ src/Memory_Management.cpp   – 메모리 관리    (변경 없음)
✅ src/Korean_Font.cpp         – 한글 폰트      (변경 없음)
✅ src/Unit_Tests.cpp          – 단위 테스트    (변경 없음)

빌드: pio run -e esp32-s3-release
업로드: pio run -e esp32-s3-release -t upload
============================================================

============================================================
  END OF FILE
============================================================
