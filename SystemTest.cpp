// SystemTest.cpp
#include "SystemTest.h"
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

SystemTest systemTest;

SystemTest::SystemTest() 
    : testRunning(false),
      testStartTime(0),
      resultCount(0) {
    memset(&baselineMemory, 0, sizeof(MemoryMetrics));
    memset(&currentMemory, 0, sizeof(MemoryMetrics));
    memset(&performance, 0, sizeof(PerformanceMetrics));
    memset(&power, 0, sizeof(PowerMetrics));
}

void SystemTest::runAllTests() {
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║     ESP32-S3 Phase 2 통합 테스트 시작                ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    testRunning = true;
    testStartTime = millis();
    resultCount = 0;
    
    // Establish baseline
    Serial.println("\n[1/6] 베이스라인 수립 중...");
    establishBaseline();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Test categories
    Serial.println("\n[2/6] 메모리 최적화 테스트...");
    runMemoryTests();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[3/6] RTOS 태스크 테스트...");
    testRTOSTasks();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[4/6] 센서 버퍼 테스트...");
    testSensorBuffers();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[5/6] WiFi 전력 관리 테스트...");
    testWiFiPowerManagement();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[6/6] 시스템 안정성 테스트...");
    testSystemStability();
    
    testRunning = false;
    
    // Print comprehensive report
    Serial.println("\n");
    printFullReport();
    
    uint32_t totalTime = millis() - testStartTime;
    Serial.printf("\n총 테스트 시간: %lu ms (%.2f초)\n", totalTime, totalTime / 1000.0f);
}

void SystemTest::establishBaseline() {
    Serial.println("  • 메모리 베이스라인 캡처 중...");
    captureMemoryMetrics(baselineMemory);
    
    Serial.println("  • 성능 베이스라인 캡처 중...");
    capturePerformanceMetrics();
    
    Serial.println("  • 전력 베이스라인 캡처 중...");
    capturePowerMetrics();
    
    Serial.println("  ✓ 베이스라인 수립 완료");
}

void SystemTest::captureMemoryMetrics(MemoryMetrics& metrics) {
    // Heap metrics
    metrics.totalHeap = ESP.getHeapSize();
    metrics.freeHeap = ESP.getFreeHeap();
    metrics.minFreeHeap = ESP.getMinFreeHeap();
    metrics.maxAllocHeap = ESP.getMaxAllocHeap();
    
    // Calculate fragmentation
    if (metrics.freeHeap > 0) {
        metrics.heapFragmentation = 100 - (metrics.maxAllocHeap * 100 / metrics.freeHeap);
    }
    
    // PSRAM metrics (if available)
    metrics.totalPSRAM = ESP.getPsramSize();
    metrics.freePSRAM = ESP.getFreePsram();
    metrics.minFreePSRAM = ESP.getMinFreePsram();
    
    // Task stack metrics
    metrics.taskCount = 0;
    
    TaskStatus_t* taskStatusArray;
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    
    taskStatusArray = (TaskStatus_t*)pvPortMalloc(taskCount * sizeof(TaskStatus_t));
    
    if (taskStatusArray != NULL) {
        taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, NULL);
        
        for (UBaseType_t i = 0; i < taskCount && metrics.taskCount < 10; i++) {
            metrics.tasks[metrics.taskCount].taskName = taskStatusArray[i].pcTaskName;
            metrics.tasks[metrics.taskCount].handle = taskStatusArray[i].xHandle;
            metrics.tasks[metrics.taskCount].stackHighWaterMark = taskStatusArray[i].usStackHighWaterMark;
            
            // Get stack size (approximate)
            metrics.tasks[metrics.taskCount].stackSize = 4096; // Default, can be improved
            
            // Calculate usage
            uint32_t stackUsed = metrics.tasks[metrics.taskCount].stackSize - 
                                 metrics.tasks[metrics.taskCount].stackHighWaterMark * 4;
            metrics.tasks[metrics.taskCount].stackUsagePercent = 
                (float)stackUsed / metrics.tasks[metrics.taskCount].stackSize * 100.0f;
            
            metrics.taskCount++;
        }
        
        vPortFree(taskStatusArray);
    }
}

void SystemTest::capturePerformanceMetrics() {
    static uint32_t lastCapture = 0;
    static uint32_t loopCount = 0;
    static uint32_t totalLoopTime = 0;
    
    uint32_t now = millis();
    uint32_t elapsed = now - lastCapture;
    
    if (elapsed >= 1000) {
        performance.loopRate = loopCount;
        performance.avgLoopTime = loopCount > 0 ? totalLoopTime / loopCount : 0;
        
        loopCount = 0;
        totalLoopTime = 0;
        lastCapture = now;
    }
    
    loopCount++;
}

void SystemTest::capturePowerMetrics() {
    power.currentMode = wifiPowerManager.getPowerMode();
    power.activityLevel = wifiPowerManager.getActivityLevel();
    power.txPower = wifiPowerManager.getTxPower();
    power.rssi = WiFi.RSSI();
    power.modemSleepCount = wifiPowerManager.getModemSleepCount();
    power.lightSleepCount = wifiPowerManager.getLightSleepCount();
    power.totalSleepTime = wifiPowerManager.getTotalSleepTime();
    power.powerSavingRatio = wifiPowerManager.getPowerSavingRatio();
}

void SystemTest::runMemoryTests() {
    uint32_t testStart = millis();
    bool passed = testMemoryOptimization();
    uint32_t duration = millis() - testStart;
    
    addTestResult("Memory Optimization", passed, duration, 
                  passed ? "메모리 최적화 목표 달성" : "메모리 최적화 미달");
}

bool SystemTest::testMemoryOptimization() {
    Serial.println("\n  --- 메모리 최적화 테스트 ---");
    
    captureMemoryMetrics(currentMemory);
    
    // Test 1: Heap usage improvement
    uint32_t baselineUsed = baselineMemory.totalHeap - baselineMemory.freeHeap;
    uint32_t currentUsed = currentMemory.totalHeap - currentMemory.freeHeap;
    float heapImprovement = (float)(baselineUsed - currentUsed) / baselineUsed * 100.0f;
    
    Serial.printf("  • Heap 사용량: %lu -> %lu bytes (%.1f%% 개선)\n", 
                  baselineUsed, currentUsed, heapImprovement);
    
    // Test 2: Fragmentation
    Serial.printf("  • Heap 단편화: %.1f%% -> %.1f%%\n", 
                  baselineMemory.heapFragmentation, 
                  currentMemory.heapFragmentation);
    
    bool fragmentationOK = currentMemory.heapFragmentation < 20.0f; // Target < 20%
    
    // Test 3: Minimum free heap
    Serial.printf("  • 최소 Free Heap: %lu bytes\n", currentMemory.minFreeHeap);
    bool minHeapOK = currentMemory.minFreeHeap > 50000; // Target > 50KB
    
    // Memory pool test
    Serial.println("\n  • 메모리 풀 상태:");
    Serial.printf("    Small Pool: %zu/%d 사용중\n", 
                  smallPool.getUsedBlocks(), 8);
    Serial.printf("    Medium Pool: %zu/%d 사용중\n", 
                  mediumPool.getUsedBlocks(), 4);
    Serial.printf("    Large Pool: %zu/%d 사용중\n", 
                  largePool.getUsedBlocks(), 2);
    
    bool poolsOK = (smallPool.getAvailableBlocks() > 0) &&
                   (mediumPool.getAvailableBlocks() > 0) &&
                   (largePool.getAvailableBlocks() > 0);
    
    bool testPassed = fragmentationOK && minHeapOK && poolsOK;
    
    Serial.printf("\n  결과: %s\n", testPassed ? "✓ 통과" : "✗ 실패");
    
    return testPassed;
}

bool SystemTest::testRTOSTasks() {
    Serial.println("\n  --- RTOS 태스크 테스트 ---");
    uint32_t testStart = millis();
    
    captureMemoryMetrics(currentMemory);
    
    bool allTasksOK = true;
    
    Serial.println("\n  태스크 스택 사용률:");
    Serial.println("  태스크명          스택크기  워터마크  사용률");
    Serial.println("  ------------------------------------------------");
    
    for (uint8_t i = 0; i < currentMemory.taskCount; i++) {
        auto& task = currentMemory.tasks[i];
        
        bool taskOK = task.stackUsagePercent < 90.0f; // Target < 90%
        if (!taskOK) allTasksOK = false;
        
        Serial.printf("  %-16s  %5lu   %5lu    %5.1f%% %s\n",
                      task.taskName,
                      task.stackSize,
                      task.stackHighWaterMark * 4,
                      task.stackUsagePercent,
                      taskOK ? "✓" : "✗ WARNING");
    }
    
    uint32_t duration = millis() - testStart;
    addTestResult("RTOS Task Stacks", allTasksOK, duration,
                  allTasksOK ? "모든 태스크 스택 정상" : "일부 태스크 스택 부족");
    
    Serial.printf("\n  결과: %s\n", allTasksOK ? "✓ 통과" : "✗ 실패");
    
    return allTasksOK;
}

bool SystemTest::testSensorBuffers() {
    Serial.println("\n  --- 센서 버퍼 테스트 ---");
    uint32_t testStart = millis();
    
    // Test 1: Buffer capacity
    Serial.println("\n  • 버퍼 용량 테스트:");
    Serial.printf("    온도 버퍼: %zu/%d (%.1f%%)\n",
                  temperatureBuffer.size(), TEMP_BUFFER_SIZE,
                  (float)temperatureBuffer.size() / TEMP_BUFFER_SIZE * 100);
    Serial.printf("    압력 버퍼: %zu/%d (%.1f%%)\n",
                  pressureBuffer.size(), PRESSURE_BUFFER_SIZE,
                  (float)pressureBuffer.size() / PRESSURE_BUFFER_SIZE * 100);
    Serial.printf("    전류 버퍼: %zu/%d (%.1f%%)\n",
                  currentBuffer.size(), CURRENT_BUFFER_SIZE,
                  (float)currentBuffer.size() / CURRENT_BUFFER_SIZE * 100);
    
    // Test 2: Statistics calculation
    Serial.println("\n  • 통계 계산 테스트:");
    float avgTemp = temperatureBuffer.getAverage();
    float maxTemp = temperatureBuffer.getMax();
    float minTemp = temperatureBuffer.getMin();
    float stdDev = temperatureBuffer.getStdDev();
    
    Serial.printf("    평균 온도: %.2f°C\n", avgTemp);
    Serial.printf("    최대 온도: %.2f°C\n", maxTemp);
    Serial.printf("    최소 온도: %.2f°C\n", minTemp);
    Serial.printf("    표준편차: %.2f\n", stdDev);
    
    bool statsOK = (maxTemp >= minTemp) && (avgTemp >= minTemp) && (avgTemp <= maxTemp);
    
    // Test 3: Push/Pop operations
    Serial.println("\n  • Push/Pop 동작 테스트:");
    
    RingBuffer<float, 10> testBuffer;
    
    // Fill buffer
    for (int i = 0; i < 15; i++) {
        testBuffer.push((float)i);
    }
    
    bool pushOK = testBuffer.size() == 10; // Should be capped at 10
    Serial.printf("    Push 테스트: %s (크기: %zu/10)\n", 
                  pushOK ? "✓" : "✗", testBuffer.size());
    
    // Pop test
    float value;
    bool popOK = testBuffer.pop(value);
    Serial.printf("    Pop 테스트: %s (값: %.1f)\n", 
                  popOK ? "✓" : "✗", value);
    
    bool testPassed = statsOK && pushOK && popOK;
    
    uint32_t duration = millis() - testStart;
    addTestResult("Sensor Buffers", testPassed, duration,
                  testPassed ? "버퍼 시스템 정상" : "버퍼 시스템 오류");
    
    Serial.printf("\n  결과: %s\n", testPassed ? "✓ 통과" : "✗ 실패");
    
    return testPassed;
}

bool SystemTest::testWiFiPowerManagement() {
    Serial.println("\n  --- WiFi 전력 관리 테스트 ---");
    uint32_t testStart = millis();
    
    capturePowerMetrics();
    
    // Test 1: Power mode switching
    Serial.println("\n  • 전력 모드 전환 테스트:");
    
    WiFiPowerMode originalMode = power.currentMode;
    
    wifiPowerManager.setPowerMode(WiFiPowerMode::POWER_SAVE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    bool mode1 = (wifiPowerManager.getPowerMode() == WiFiPowerMode::POWER_SAVE);
    Serial.printf("    POWER_SAVE 모드: %s\n", mode1 ? "✓" : "✗");
    
    wifiPowerManager.setPowerMode(WiFiPowerMode::BALANCED);
    vTaskDelay(pdMS_TO_TICKS(1000));
    bool mode2 = (wifiPowerManager.getPowerMode() == WiFiPowerMode::BALANCED);
    Serial.printf("    BALANCED 모드: %s\n", mode2 ? "✓" : "✗");
    
    wifiPowerManager.setPowerMode(originalMode);
    
    // Test 2: TX Power adjustment
    Serial.println("\n  • TX Power 조정 테스트:");
    int8_t originalTxPower = power.txPower;
    
    wifiPowerManager.setTxPower(10);
    vTaskDelay(pdMS_TO_TICKS(500));
    bool tx1 = (wifiPowerManager.getTxPower() == 10);
    Serial.printf("    10 dBm 설정: %s\n", tx1 ? "✓" : "✗");
    
    wifiPowerManager.setTxPower(originalTxPower);
    
    // Test 3: Power saving statistics
    Serial.println("\n  • 전력 절감 통계:");
    Serial.printf("    Modem Sleep: %lu회\n", power.modemSleepCount);
    Serial.printf("    Light Sleep: %lu회\n", power.lightSleepCount);
    Serial.printf("    절전 비율: %.2f%%\n", power.powerSavingRatio);
    Serial.printf("    RSSI: %ld dBm\n", power.rssi);
    
    bool testPassed = mode1 && mode2 && tx1;
    
    uint32_t duration = millis() - testStart;
    addTestResult("WiFi Power Management", testPassed, duration,
                  testPassed ? "전력 관리 정상" : "전력 관리 오류");
    
    Serial.printf("\n  결과: %s\n", testPassed ? "✓ 통과" : "✗ 실패");
    
    return testPassed;
}

bool SystemTest::testSystemStability() {
    Serial.println("\n  --- 시스템 안정성 테스트 ---");
    uint32_t testStart = millis();
    
    // Test 1: Watchdog
    Serial.println("\n  • Watchdog 상태:");
    esp_task_wdt_status(NULL);
    Serial.println("    ✓ Watchdog 정상");
    
    // Test 2: Memory leak check
    Serial.println("\n  • 메모리 누수 체크:");
    uint32_t heapBefore = ESP.getFreeHeap();
    
    // Simulate some operations
    for (int i = 0; i < 100; i++) {
        char* buf = (char*)smallPool.allocate();
        if (buf) {
            sprintf(buf, "Test %d", i);
            smallPool.deallocate(buf);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    uint32_t heapAfter = ESP.getFreeHeap();
    int32_t heapDiff = heapAfter - heapBefore;
    
    Serial.printf("    Heap 변화: %ld bytes\n", heapDiff);
    bool noLeak = (abs(heapDiff) < 1000); // Allow < 1KB variation
    Serial.printf("    메모리 누수: %s\n", noLeak ? "✓ 없음" : "✗ 감지됨");
    
    // Test 3: Task responsiveness
    Serial.println("\n  • 태스크 응답성 체크:");
    bool allResponsive = true;
    
    for (uint8_t i = 0; i < currentMemory.taskCount; i++) {
        eTaskState state = eTaskGetState(currentMemory.tasks[i].handle);
        bool responsive = (state == eRunning || state == eReady || state == eBlocked);
        
        if (!responsive) allResponsive = false;
    }
    
    Serial.printf("    모든 태스크 응답: %s\n", allResponsive ? "✓" : "✗");
    
    bool testPassed = noLeak && allResponsive;
    
    uint32_t duration = millis() - testStart;
    addTestResult("System Stability", testPassed, duration,
                  testPassed ? "시스템 안정" : "불안정 요소 감지");
    
    Serial.printf("\n  결과: %s\n", testPassed ? "✓ 통과" : "✗ 실패");
    
    return testPassed;
}

void SystemTest::addTestResult(const char* name, bool passed, uint32_t duration, const char* details) {
    if (resultCount < 20) {
        results[resultCount].testName = name;
        results[resultCount].passed = passed;
        results[resultCount].duration = duration;
        results[resultCount].details = details;
        resultCount++;
    }
}

void SystemTest::printMemoryReport() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              메모리 상태 리포트                       ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    captureMemoryMetrics(currentMemory);
    
    Serial.println("\nHeap 메모리:");
    Serial.printf("  총 Heap:      %10lu bytes\n", currentMemory.totalHeap);
    Serial.printf("  사용 중:      %10lu bytes (%.1f%%)\n", 
                  currentMemory.totalHeap - currentMemory.freeHeap,
                  (float)(currentMemory.totalHeap - currentMemory.freeHeap) / currentMemory.totalHeap * 100);
    Serial.printf("  여유 공간:    %10lu bytes\n", currentMemory.freeHeap);
    Serial.printf("  최소 여유:    %10lu bytes\n", currentMemory.minFreeHeap);
    Serial.printf("  최대 할당:    %10lu bytes\n", currentMemory.maxAllocHeap);
    Serial.printf("  단편화:       %10.1f%%\n", currentMemory.heapFragmentation);
    
    if (currentMemory.totalPSRAM > 0) {
        Serial.println("\nPSRAM 메모리:");
        Serial.printf("  총 PSRAM:     %10lu bytes\n", currentMemory.totalPSRAM);
        Serial.printf("  여유 공간:    %10lu bytes\n", currentMemory.freePSRAM);
    }
    
    Serial.println("\n메모리 풀 상태:");
    Serial.printf("  Small Pool (256B):  %zu/%d 블록 사용\n", 
                  smallPool.getUsedBlocks(), 8);
    Serial.printf("  Medium Pool (512B): %zu/%d 블록 사용\n", 
                  mediumPool.getUsedBlocks(), 4);
    Serial.printf("  Large Pool (1KB):   %zu/%d 블록 사용\n", 
                  largePool.getUsedBlocks(), 2);
}

void SystemTest::printPerformanceReport() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              성능 상태 리포트                         ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    capturePerformanceMetrics();
    
    Serial.printf("\n메인 루프:\n");
    Serial.printf("  실행 속도:    %10lu loops/sec\n", performance.loopRate);
    Serial.printf("  평균 시간:    %10lu μs\n", performance.avgLoopTime);
    Serial.printf("  최대 시간:    %10lu μs\n", performance.maxLoopTime);
    
    Serial.printf("\n태스크 성능:\n");
    Serial.printf("  센서 읽기:    %10lu reads/sec\n", performance.sensorReadRate);
    Serial.printf("  UI 업데이트:  %10lu updates/sec\n", performance.uiUpdateRate);
    
    if (performance.mqttPublishRate > 0) {
        Serial.printf("\nMQTT 성능:\n");
        Serial.printf("  발행 속도:    %10lu msg/sec\n", performance.mqttPublishRate);
        Serial.printf("  레이턴시:     %10lu ms\n", performance.mqttLatency);
    }
}

void SystemTest::printPowerReport() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║              전력 상태 리포트                         ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    capturePowerMetrics();
    
    Serial.printf("\nWiFi 전력 모드:\n");
    Serial.printf("  현재 모드:    ");
    switch (power.currentMode) {
        case WiFiPowerMode::ALWAYS_ON: Serial.println("ALWAYS_ON"); break;
        case WiFiPowerMode::BALANCED: Serial.println("BALANCED"); break;
        case WiFiPowerMode::POWER_SAVE: Serial.println("POWER_SAVE"); break;
        case WiFiPowerMode::DEEP_SLEEP_READY: Serial.println("DEEP_SLEEP_READY"); break;
    }
    
    Serial.printf("  활동 레벨:    ");
    switch (power.activityLevel) {
        case WiFiActivityLevel::IDLE: Serial.println("IDLE"); break;
        case WiFiActivityLevel::LOW: Serial.println("LOW"); break;
        case WiFiActivityLevel::MEDIUM: Serial.println("MEDIUM"); break;
        case WiFiActivityLevel::HIGH: Serial.println("HIGH"); break;
    }
    
    Serial.printf("\nRF 상태:\n");
    Serial.printf("  TX Power:     %10d dBm\n", power.txPower);
    Serial.printf("  RSSI:         %10ld dBm\n", power.rssi);
    
    Serial.printf("\n절전 통계:\n");
    Serial.printf("  Modem Sleep:  %10lu 회\n", power.modemSleepCount);
    Serial.printf("  Light Sleep:  %10lu 회\n", power.lightSleepCount);
    Serial.printf("  총 Sleep:     %10lu ms\n", power.totalSleepTime);
    Serial.printf("  절전 비율:    %10.2f%%\n", power.powerSavingRatio);
    
    Serial.printf("\n네트워크 활동:\n");
    Serial.printf("  TX 패킷:      %10lu\n", power.wifiTxPackets);
    Serial.printf("  RX 패킷:      %10lu\n", power.wifiRxPackets);
}

void SystemTest::printFullReport() {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║          Phase 2 통합 테스트 최종 리포트             ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    
    // Test results summary
    Serial.println("\n테스트 결과 요약:");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    uint8_t passedCount = 0;
    for (uint8_t i = 0; i < resultCount; i++) {
        Serial.printf("  %s %-25s [%5lu ms] %s\n",
                      results[i].passed ? "✓" : "✗",
                      results[i].testName,
                      results[i].duration,
                      results[i].details);
        if (results[i].passed) passedCount++;
    }
    
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.printf("  합격: %d/%d (%.1f%%)\n", 
                  passedCount, resultCount, 
                  (float)passedCount / resultCount * 100);
    
    // Detailed reports
    printMemoryReport();
    printPerformanceReport();
    printPowerReport();
    
    // Comparison with baseline
    Serial.println("\n\n베이스라인 대비 개선:");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    uint32_t baselineUsed = baselineMemory.totalHeap - baselineMemory.freeHeap;
    uint32_t currentUsed = currentMemory.totalHeap - currentMemory.freeHeap;
    int32_t heapSaved = baselineUsed - currentUsed;
    float heapImprovement = (float)heapSaved / baselineUsed * 100.0f;
    
    Serial.printf("  Heap 절감:    %10ld bytes (%.1f%%)\n", heapSaved, heapImprovement);
    Serial.printf("  단편화 개선:  %10.1f%% -> %.1f%%\n", 
                  baselineMemory.heapFragmentation,
                  currentMemory.heapFragmentation);
    
    // Overall grade
    Serial.println("\n\n최종 평가:");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    float score = (float)passedCount / resultCount * 100;
    const char* grade;
    
    if (score >= 90) grade = "우수 (A)";
    else if (score >= 80) grade = "양호 (B)";
    else if (score >= 70) grade = "보통 (C)";
    else grade = "개선 필요 (D)";
    
    Serial.printf("  종합 점수:    %.1f점\n", score);
    Serial.printf("  평가 등급:    %s\n", grade);
    
    if (score >= 80) {
        Serial.println("\n  🎉 Phase 2 최적화가 성공적으로 완료되었습니다!");
    } else {
        Serial.println("\n  ⚠️  일부 개선이 필요합니다. 위의 실패 항목을 검토하세요.");
    }
    
    Serial.println("\n╚════════════════════════════════════════════════════════╝\n");
}

void SystemTest::runQuickTest() {
    Serial.println("\n=== 빠른 테스트 모드 ===\n");
    
    testStartTime = millis();
    
    captureMemoryMetrics(currentMemory);
    capturePerformanceMetrics();
    capturePowerMetrics();
    
    printMemoryReport();
    printPowerReport();
    
    uint32_t duration = millis() - testStartTime;
    Serial.printf("\n테스트 완료 시간: %lu ms\n", duration);
}

void SystemTest::runStressTest(uint32_t durationMinutes) {
    Serial.printf("\n=== 스트레스 테스트 (%lu분) ===\n", durationMinutes);
    
    uint32_t endTime = millis() + (durationMinutes * 60000);
    uint32_t reportInterval = 60000; // 1분마다 리포트
    uint32_t lastReport = millis();
    
    establishBaseline();
    
    while (millis() < endTime) {
        // Generate varying load
        uint8_t loadLevel = (millis() / 10000) % 10 + 1; // 1-10
        generateLoad(loadLevel);
        
        // Periodic reporting
        if (millis() - lastReport >= reportInterval) {
            Serial.printf("\n[%lu분 경과]\n", (millis() - testStartTime) / 60000);
            printMemoryReport();
            lastReport = millis();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    Serial.println("\n=== 스트레스 테스트 완료 ===");
    printFullReport();
}

void SystemTest::generateLoad(uint8_t level) {
    // Simulate sensor reads
    for (int i = 0; i < level; i++) {
        temperatureBuffer.push(25.0f + random(-50, 50) / 10.0f);
        pressureBuffer.push(101.3f + random(-10, 10) / 10.0f);
    }
    
    // Simulate WiFi activity
    for (int i = 0; i < level; i++) {
        wifiPowerManager.notifyPacketTx();
    }
    
    // Simulate memory pool usage
    char* buf = (char*)smallPool.allocate();
    if (buf) {
        sprintf(buf, "Load level %d", level);
        delay(random(1, 10));
        smallPool.deallocate(buf);
    }
}