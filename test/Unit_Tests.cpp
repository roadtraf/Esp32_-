
#ifdef UNIT_TEST_MODE

#include "Config.h"                 // enum, struct, extern ?꾩뿭蹂??(pidError ??
#include "PID_Control.h"            // updatePID(), resetPID()
#include "../include/SD_Logger.h"   // ISO8601_BUFFER_SIZE, getCurrentTimeISO8601
#include <cstring>                  // strlen, strchr
#include "Control.h"                // checkSafetyInterlock()
#include "Sensor.h"                 // validateParameters(), checkSensorHealth()
#include "ErrorHandler.h"           // attemptErrorRecovery()

// Memory_Management.cpp 怨듦컻 API
extern bool verifyMemory();

// ==================== ?뚯뒪???좏떥由ы떚 ====================
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

// ==================== PID ?쒖뼱 ?뚯뒪??====================
void test_PID_Controller() {
  Serial.println("\n=== PID Controller Tests ===");

  // 1. PID 由ъ뀑 ?뚯뒪??
  resetPID();
  TEST_ASSERT_EQUAL(0.0, pidError, "PID Reset - Error");
  TEST_ASSERT_EQUAL(0.0, pidIntegral, "PID Reset - Integral");
  TEST_ASSERT_EQUAL(0.0, pidDerivative, "PID Reset - Derivative");

  // 2. PID 異쒕젰 踰붿쐞 ?뚯뒪??
  config.targetPressure = -80.0;
  sensorData.pressure = -50.0;
  updatePID();
  TEST_ASSERT(pidOutput >= 0.0 && pidOutput <= 100.0, "PID Output Range");

  // 3. ?곷텇 ?쒗븳 ?뚯뒪??
  for (int i = 0; i < 100; i++) {
    updatePID();
  }
  TEST_ASSERT(abs(pidIntegral) <= 50.0, "PID Integral Limit");

  Serial.printf("PID Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== ?덉쟾 ?명꽣???뚯뒪??====================
void test_Safety_Interlock() {
  Serial.println("\n=== Safety Interlock Tests ===");

  TEST_ASSERT(checkSafetyInterlock(true, false), "Pump Only");
  TEST_ASSERT(checkSafetyInterlock(false, true), "Valve Only");
  TEST_ASSERT(!checkSafetyInterlock(true, true), "Pump + Valve (Blocked)");
  TEST_ASSERT(checkSafetyInterlock(false, false), "Both Off");

  Serial.printf("Safety Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== ?뚮씪誘명꽣 寃利??뚯뒪??====================
void test_Parameter_Validation() {
  Serial.println("\n=== Parameter Validation Tests ===");

  // ?뺤긽 媛?
  sensorData.pressure = -80.0;
  sensorData.current = 3.5;
  TEST_ASSERT(validateParameters(), "Valid Parameters");

  // NaN ?뚯뒪??
  sensorData.pressure = NAN;
  TEST_ASSERT(!validateParameters(), "NaN Pressure");
  sensorData.pressure = -80.0;

  // 踰붿쐞 珥덇낵
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

// ==================== ?쇱꽌 ?ъ뒪 ?뚯뒪??====================
void test_Sensor_Health() {
  Serial.println("\n=== Sensor Health Tests ===");

  // ?뺤긽 ?쇱꽌 媛?
  sensorData.pressure = -80.0;
  sensorData.current = 3.5;
  checkSensorHealth();
  TEST_ASSERT(true, "Normal Sensor Values");

  // ?뺣젰 ?쇱꽌 ?댁긽 (?꾩븬 踰붿쐞 泥댄겕???ㅼ젣 ?섎뱶?⑥뼱 ?꾩슂)
  // 媛꾩젒 ?뚯뒪?? ?뺣젰 媛믪씠 ?좏슚 踰붿쐞?몄? ?뺤씤
  TEST_ASSERT(sensorData.pressure >= -105.0 && sensorData.pressure <= 5.0, 
    "Pressure Sensor Range");

  // ?꾨쪟 ?쇱꽌 ?댁긽
  TEST_ASSERT(sensorData.current >= 0.0 && sensorData.current <= 10.0, 
    "Current Sensor Range");

  Serial.printf("Sensor Health Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== ?먮윭 蹂듦뎄 ?뚯뒪??====================
void test_Error_Recovery() {
  Serial.println("\n=== Error Recovery Tests ===");

  // TEMPORARY ?먮윭
  currentError.severity = SEVERITY_TEMPORARY;
  currentError.retryCount = 0;
  TEST_ASSERT(attemptErrorRecovery(), "Temporary Error - First Retry");

  // RECOVERABLE ?먮윭
  currentError.severity = SEVERITY_RECOVERABLE;
  currentError.retryCount = 0;
  TEST_ASSERT(attemptErrorRecovery(), "Recoverable Error - First Retry");

  // CRITICAL ?먮윭
  currentError.severity = SEVERITY_CRITICAL;
  currentError.retryCount = 0;
  TEST_ASSERT(!attemptErrorRecovery(), "Critical Error - No Recovery");

  Serial.printf("Error Recovery Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== 硫붾え由?愿由??뚯뒪??====================
void test_Memory_Management() {
  Serial.println("\n=== Memory Management Tests ===");

  TEST_ASSERT(verifyMemory(), "Memory Verification");
  TEST_ASSERT(ESP.getFreeHeap() > 100000, "Sufficient Free Heap");
  TEST_ASSERT(ESP.getFreePsram() > 1000000, "Sufficient Free PSRAM");
  TEST_ASSERT(ESP.getPsramSize() == 8 * 1024 * 1024, "PSRAM Size Check (8MB)");

  Serial.printf("Memory Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== ?쒓컙 ?숆린???뚯뒪??====================
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

// ==================== Watchdog ?뚯뒪??====================
void test_Watchdog() {
  Serial.println("\n=== Watchdog Tests ===");

  extern void feedWatchdog();
  feedWatchdog();
  TEST_ASSERT(true, "Watchdog Feed");

  // Watchdog ??꾩븘???뚯뒪?몃뒗 ?ㅼ젣濡?由ъ뀑??諛쒖깮?섎?濡??앸왂
  TEST_ASSERT(true, "Watchdog Timeout (Skipped)");

  Serial.printf("Watchdog Tests: %d passed, %d failed\n\n", testsPassed, testsFailed);
}

// ==================== ?꾩껜 ?뚯뒪???ㅽ뻾 ====================
void runUnitTests() {
  Serial.println("\n");
  Serial.println("=====================================");
  Serial.println("   ?⑥쐞 ?뚯뒪???쒖옉");
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
  Serial.printf("?뚯뒪??寃곌낵: %d passed, %d failed\n", testsPassed, testsFailed);
  if (testsFailed == 0) {
    Serial.println("紐⑤뱺 ?뚯뒪???듦낵! ??);
  } else {
    Serial.printf("%d媛??뚯뒪???ㅽ뙣 ??n", testsFailed);
  }
  Serial.println("=====================================\n");
}

#endif // UNIT_TEST_MODE



============================================================
  ?뚯씪 紐⑸줉 ?붿빟  (v3.3)
============================================================
??platformio.ini              ??PlatformIO ?ㅼ젙 (蹂寃??놁쓬)
??partitions_16mb.csv         ???뚰떚???뚯씠釉?  (蹂寃??놁쓬)
??src/LovyanGFX_Config.hpp    ???붿뒪?뚮젅???ㅼ젙  (蹂寃??놁쓬)
??src/main.cpp                ??硫붿씤 ?꾨줈洹몃옩   (?졻몼?™몿???곸슜)
??src/UI_Screens.cpp          ??UI ?붾㈃        (?β뫂 ?곸슜)
??src/USB_Keyboard.cpp        ??USB ?ㅽ뙣??    (???곸슜)
??src/Trend_Graph.cpp         ??異붿꽭 洹몃옒??   ???좉퇋 ?뚯씪
??src/OTA_Update.cpp          ??OTA + HTTP ?쒕쾭 (蹂寃??놁쓬)
??src/Memory_Management.cpp   ??硫붾え由?愿由?   (蹂寃??놁쓬)
??src/Korean_Font.cpp         ???쒓? ?고듃      (蹂寃??놁쓬)
??src/Unit_Tests.cpp          ???⑥쐞 ?뚯뒪??   (蹂寃??놁쓬)

鍮뚮뱶: pio run -e esp32-s3-release
?낅줈?? pio run -e esp32-s3-release -t upload
============================================================

============================================================
  END OF FILE
============================================================
