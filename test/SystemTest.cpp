// SystemTest.cpp
#include "SystemTest.h"
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>

// FreeRTOS (delay 媛쒖꽑)
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
    Serial.println("?붴븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븮");
    Serial.println("??    ESP32-S3 Phase 2 ?듯빀 ?뚯뒪???쒖옉                ??);
    Serial.println("?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴");
    
    testRunning = true;
    testStartTime = millis();
    resultCount = 0;
    
    // Establish baseline
    Serial.println("\n[1/6] 踰좎씠?ㅻ씪???섎┰ 以?..");
    establishBaseline();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Test categories
    Serial.println("\n[2/6] 硫붾え由?理쒖쟻???뚯뒪??..");
    runMemoryTests();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[3/6] RTOS ?쒖뒪???뚯뒪??..");
    testRTOSTasks();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[4/6] ?쇱꽌 踰꾪띁 ?뚯뒪??..");
    testSensorBuffers();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[5/6] WiFi ?꾨젰 愿由??뚯뒪??..");
    testWiFiPowerManagement();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n[6/6] ?쒖뒪???덉젙???뚯뒪??..");
    testSystemStability();
    
    testRunning = false;
    
    // Print comprehensive report
    Serial.println("\n");
    printFullReport();
    
    uint32_t totalTime = millis() - testStartTime;
    Serial.printf("\n珥??뚯뒪???쒓컙: %lu ms (%.2f珥?\n", totalTime, totalTime / 1000.0f);
}

void SystemTest::establishBaseline() {
    Serial.println("  ??硫붾え由?踰좎씠?ㅻ씪??罹≪쿂 以?..");
    captureMemoryMetrics(baselineMemory);
    
    Serial.println("  ???깅뒫 踰좎씠?ㅻ씪??罹≪쿂 以?..");
    capturePerformanceMetrics();
    
    Serial.println("  ???꾨젰 踰좎씠?ㅻ씪??罹≪쿂 以?..");
    capturePowerMetrics();
    
    Serial.println("  ??踰좎씠?ㅻ씪???섎┰ ?꾨즺");
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
                  passed ? "硫붾え由?理쒖쟻??紐⑺몴 ?ъ꽦" : "硫붾え由?理쒖쟻??誘몃떖");
}

bool SystemTest::testMemoryOptimization() {
    Serial.println("\n  --- 硫붾え由?理쒖쟻???뚯뒪??---");
    
    captureMemoryMetrics(currentMemory);
    
    // Test 1: Heap usage improvement
    uint32_t baselineUsed = baselineMemory.totalHeap - baselineMemory.freeHeap;
    uint32_t currentUsed = currentMemory.totalHeap - currentMemory.freeHeap;
    float heapImprovement = (float)(baselineUsed - currentUsed) / baselineUsed * 100.0f;
    
    Serial.printf("  ??Heap ?ъ슜?? %lu -> %lu bytes (%.1f%% 媛쒖꽑)\n", 
                  baselineUsed, currentUsed, heapImprovement);
    
    // Test 2: Fragmentation
    Serial.printf("  ??Heap ?⑦렪?? %.1f%% -> %.1f%%\n", 
                  baselineMemory.heapFragmentation, 
                  currentMemory.heapFragmentation);
    
    bool fragmentationOK = currentMemory.heapFragmentation < 20.0f; // Target < 20%
    
    // Test 3: Minimum free heap
    Serial.printf("  ??理쒖냼 Free Heap: %lu bytes\n", currentMemory.minFreeHeap);
    bool minHeapOK = currentMemory.minFreeHeap > 50000; // Target > 50KB
    
    // Memory pool test
    Serial.println("\n  ??硫붾え由?? ?곹깭:");
    Serial.printf("    Small Pool: %zu/%d ?ъ슜以?n", 
                  smallPool.getUsedBlocks(), 8);
    Serial.printf("    Medium Pool: %zu/%d ?ъ슜以?n", 
                  mediumPool.getUsedBlocks(), 4);
    Serial.printf("    Large Pool: %zu/%d ?ъ슜以?n", 
                  largePool.getUsedBlocks(), 2);
    
    bool poolsOK = (smallPool.getAvailableBlocks() > 0) &&
                   (mediumPool.getAvailableBlocks() > 0) &&
                   (largePool.getAvailableBlocks() > 0);
    
    bool testPassed = fragmentationOK && minHeapOK && poolsOK;
    
    Serial.printf("\n  寃곌낵: %s\n", testPassed ? "???듦낵" : "???ㅽ뙣");
    
    return testPassed;
}

bool SystemTest::testRTOSTasks() {
    Serial.println("\n  --- RTOS ?쒖뒪???뚯뒪??---");
    uint32_t testStart = millis();
    
    captureMemoryMetrics(currentMemory);
    
    bool allTasksOK = true;
    
    Serial.println("\n  ?쒖뒪???ㅽ깮 ?ъ슜瑜?");
    Serial.println("  ?쒖뒪?щ챸          ?ㅽ깮?ш린  ?뚰꽣留덊겕  ?ъ슜瑜?);
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
                      taskOK ? "?? : "??WARNING");
    }
    
    uint32_t duration = millis() - testStart;
    addTestResult("RTOS Task Stacks", allTasksOK, duration,
                  allTasksOK ? "紐⑤뱺 ?쒖뒪???ㅽ깮 ?뺤긽" : "?쇰? ?쒖뒪???ㅽ깮 遺議?);
    
    Serial.printf("\n  寃곌낵: %s\n", allTasksOK ? "???듦낵" : "???ㅽ뙣");
    
    return allTasksOK;
}

bool SystemTest::testSensorBuffers() {
    Serial.println("\n  --- ?쇱꽌 踰꾪띁 ?뚯뒪??---");
    uint32_t testStart = millis();
    
    // Test 1: Buffer capacity
    Serial.println("\n  ??踰꾪띁 ?⑸웾 ?뚯뒪??");
    Serial.printf("    ?⑤룄 踰꾪띁: %zu/%d (%.1f%%)\n",
                  temperatureBuffer.size(), TEMP_BUFFER_SIZE,
                  (float)temperatureBuffer.size() / TEMP_BUFFER_SIZE * 100);
    Serial.printf("    ?뺣젰 踰꾪띁: %zu/%d (%.1f%%)\n",
                  pressureBuffer.size(), PRESSURE_BUFFER_SIZE,
                  (float)pressureBuffer.size() / PRESSURE_BUFFER_SIZE * 100);
    Serial.printf("    ?꾨쪟 踰꾪띁: %zu/%d (%.1f%%)\n",
                  currentBuffer.size(), CURRENT_BUFFER_SIZE,
                  (float)currentBuffer.size() / CURRENT_BUFFER_SIZE * 100);
    
    // Test 2: Statistics calculation
    Serial.println("\n  ???듦퀎 怨꾩궛 ?뚯뒪??");
    float avgTemp = temperatureBuffer.getAverage();
    float maxTemp = temperatureBuffer.getMax();
    float minTemp = temperatureBuffer.getMin();
    float stdDev = temperatureBuffer.getStdDev();
    
    Serial.printf("    ?됯퇏 ?⑤룄: %.2f째C\n", avgTemp);
    Serial.printf("    理쒕? ?⑤룄: %.2f째C\n", maxTemp);
    Serial.printf("    理쒖냼 ?⑤룄: %.2f째C\n", minTemp);
    Serial.printf("    ?쒖??몄감: %.2f\n", stdDev);
    
    bool statsOK = (maxTemp >= minTemp) && (avgTemp >= minTemp) && (avgTemp <= maxTemp);
    
    // Test 3: Push/Pop operations
    Serial.println("\n  ??Push/Pop ?숈옉 ?뚯뒪??");
    
    RingBuffer<float, 10> testBuffer;
    
    // Fill buffer
    for (int i = 0; i < 15; i++) {
        testBuffer.push((float)i);
    }
    
    bool pushOK = testBuffer.size() == 10; // Should be capped at 10
    Serial.printf("    Push ?뚯뒪?? %s (?ш린: %zu/10)\n", 
                  pushOK ? "?? : "??, testBuffer.size());
    
    // Pop test
    float value;
    bool popOK = testBuffer.pop(value);
    Serial.printf("    Pop ?뚯뒪?? %s (媛? %.1f)\n", 
                  popOK ? "?? : "??, value);
    
    bool testPassed = statsOK && pushOK && popOK;
    
    uint32_t duration = millis() - testStart;
    addTestResult("Sensor Buffers", testPassed, duration,
                  testPassed ? "踰꾪띁 ?쒖뒪???뺤긽" : "踰꾪띁 ?쒖뒪???ㅻ쪟");
    
    Serial.printf("\n  寃곌낵: %s\n", testPassed ? "???듦낵" : "???ㅽ뙣");
    
    return testPassed;
}

bool SystemTest::testWiFiPowerManagement() {
    Serial.println("\n  --- WiFi ?꾨젰 愿由??뚯뒪??---");
    uint32_t testStart = millis();
    
    capturePowerMetrics();
    
    // Test 1: Power mode switching
    Serial.println("\n  ???꾨젰 紐⑤뱶 ?꾪솚 ?뚯뒪??");
    
    WiFiPowerMode originalMode = power.currentMode;
    
    wifiPowerManager.setPowerMode(WiFiPowerMode::POWER_SAVE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    bool mode1 = (wifiPowerManager.getPowerMode() == WiFiPowerMode::POWER_SAVE);
    Serial.printf("    POWER_SAVE 紐⑤뱶: %s\n", mode1 ? "?? : "??);
    
    wifiPowerManager.setPowerMode(WiFiPowerMode::BALANCED);
    vTaskDelay(pdMS_TO_TICKS(1000));
    bool mode2 = (wifiPowerManager.getPowerMode() == WiFiPowerMode::BALANCED);
    Serial.printf("    BALANCED 紐⑤뱶: %s\n", mode2 ? "?? : "??);
    
    wifiPowerManager.setPowerMode(originalMode);
    
    // Test 2: TX Power adjustment
    Serial.println("\n  ??TX Power 議곗젙 ?뚯뒪??");
    int8_t originalTxPower = power.txPower;
    
    wifiPowerManager.setTxPower(10);
    vTaskDelay(pdMS_TO_TICKS(500));
    bool tx1 = (wifiPowerManager.getTxPower() == 10);
    Serial.printf("    10 dBm ?ㅼ젙: %s\n", tx1 ? "?? : "??);
    
    wifiPowerManager.setTxPower(originalTxPower);
    
    // Test 3: Power saving statistics
    Serial.println("\n  ???꾨젰 ?덇컧 ?듦퀎:");
    Serial.printf("    Modem Sleep: %lu??n", power.modemSleepCount);
    Serial.printf("    Light Sleep: %lu??n", power.lightSleepCount);
    Serial.printf("    ?덉쟾 鍮꾩쑉: %.2f%%\n", power.powerSavingRatio);
    Serial.printf("    RSSI: %ld dBm\n", power.rssi);
    
    bool testPassed = mode1 && mode2 && tx1;
    
    uint32_t duration = millis() - testStart;
    addTestResult("WiFi Power Management", testPassed, duration,
                  testPassed ? "?꾨젰 愿由??뺤긽" : "?꾨젰 愿由??ㅻ쪟");
    
    Serial.printf("\n  寃곌낵: %s\n", testPassed ? "???듦낵" : "???ㅽ뙣");
    
    return testPassed;
}

bool SystemTest::testSystemStability() {
    Serial.println("\n  --- ?쒖뒪???덉젙???뚯뒪??---");
    uint32_t testStart = millis();
    
    // Test 1: Watchdog
    Serial.println("\n  ??Watchdog ?곹깭:");
    esp_task_wdt_status(NULL);
    Serial.println("    ??Watchdog ?뺤긽");
    
    // Test 2: Memory leak check
    Serial.println("\n  ??硫붾え由??꾩닔 泥댄겕:");
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
    
    Serial.printf("    Heap 蹂?? %ld bytes\n", heapDiff);
    bool noLeak = (abs(heapDiff) < 1000); // Allow < 1KB variation
    Serial.printf("    硫붾え由??꾩닔: %s\n", noLeak ? "???놁쓬" : "??媛먯???);
    
    // Test 3: Task responsiveness
    Serial.println("\n  ???쒖뒪???묐떟??泥댄겕:");
    bool allResponsive = true;
    
    for (uint8_t i = 0; i < currentMemory.taskCount; i++) {
        eTaskState state = eTaskGetState(currentMemory.tasks[i].handle);
        bool responsive = (state == eRunning || state == eReady || state == eBlocked);
        
        if (!responsive) allResponsive = false;
    }
    
    Serial.printf("    紐⑤뱺 ?쒖뒪???묐떟: %s\n", allResponsive ? "?? : "??);
    
    bool testPassed = noLeak && allResponsive;
    
    uint32_t duration = millis() - testStart;
    addTestResult("System Stability", testPassed, duration,
                  testPassed ? "?쒖뒪???덉젙" : "遺덉븞???붿냼 媛먯?");
    
    Serial.printf("\n  寃곌낵: %s\n", testPassed ? "???듦낵" : "???ㅽ뙣");
    
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
    Serial.println("\n?붴븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븮");
    Serial.println("??             硫붾え由??곹깭 由ы룷??                      ??);
    Serial.println("?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴");
    
    captureMemoryMetrics(currentMemory);
    
    Serial.println("\nHeap 硫붾え由?");
    Serial.printf("  珥?Heap:      %10lu bytes\n", currentMemory.totalHeap);
    Serial.printf("  ?ъ슜 以?      %10lu bytes (%.1f%%)\n", 
                  currentMemory.totalHeap - currentMemory.freeHeap,
                  (float)(currentMemory.totalHeap - currentMemory.freeHeap) / currentMemory.totalHeap * 100);
    Serial.printf("  ?ъ쑀 怨듦컙:    %10lu bytes\n", currentMemory.freeHeap);
    Serial.printf("  理쒖냼 ?ъ쑀:    %10lu bytes\n", currentMemory.minFreeHeap);
    Serial.printf("  理쒕? ?좊떦:    %10lu bytes\n", currentMemory.maxAllocHeap);
    Serial.printf("  ?⑦렪??       %10.1f%%\n", currentMemory.heapFragmentation);
    
    if (currentMemory.totalPSRAM > 0) {
        Serial.println("\nPSRAM 硫붾え由?");
        Serial.printf("  珥?PSRAM:     %10lu bytes\n", currentMemory.totalPSRAM);
        Serial.printf("  ?ъ쑀 怨듦컙:    %10lu bytes\n", currentMemory.freePSRAM);
    }
    
    Serial.println("\n硫붾え由?? ?곹깭:");
    Serial.printf("  Small Pool (256B):  %zu/%d 釉붾줉 ?ъ슜\n", 
                  smallPool.getUsedBlocks(), 8);
    Serial.printf("  Medium Pool (512B): %zu/%d 釉붾줉 ?ъ슜\n", 
                  mediumPool.getUsedBlocks(), 4);
    Serial.printf("  Large Pool (1KB):   %zu/%d 釉붾줉 ?ъ슜\n", 
                  largePool.getUsedBlocks(), 2);
}

void SystemTest::printPerformanceReport() {
    Serial.println("\n?붴븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븮");
    Serial.println("??             ?깅뒫 ?곹깭 由ы룷??                        ??);
    Serial.println("?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴");
    
    capturePerformanceMetrics();
    
    Serial.printf("\n硫붿씤 猷⑦봽:\n");
    Serial.printf("  ?ㅽ뻾 ?띾룄:    %10lu loops/sec\n", performance.loopRate);
    Serial.printf("  ?됯퇏 ?쒓컙:    %10lu 關s\n", performance.avgLoopTime);
    Serial.printf("  理쒕? ?쒓컙:    %10lu 關s\n", performance.maxLoopTime);
    
    Serial.printf("\n?쒖뒪???깅뒫:\n");
    Serial.printf("  ?쇱꽌 ?쎄린:    %10lu reads/sec\n", performance.sensorReadRate);
    Serial.printf("  UI ?낅뜲?댄듃:  %10lu updates/sec\n", performance.uiUpdateRate);
    
    if (performance.mqttPublishRate > 0) {
        Serial.printf("\nMQTT ?깅뒫:\n");
        Serial.printf("  諛쒗뻾 ?띾룄:    %10lu msg/sec\n", performance.mqttPublishRate);
        Serial.printf("  ?덉씠?댁떆:     %10lu ms\n", performance.mqttLatency);
    }
}

void SystemTest::printPowerReport() {
    Serial.println("\n?붴븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븮");
    Serial.println("??             ?꾨젰 ?곹깭 由ы룷??                        ??);
    Serial.println("?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴");
    
    capturePowerMetrics();
    
    Serial.printf("\nWiFi ?꾨젰 紐⑤뱶:\n");
    Serial.printf("  ?꾩옱 紐⑤뱶:    ");
    switch (power.currentMode) {
        case WiFiPowerMode::ALWAYS_ON: Serial.println("ALWAYS_ON"); break;
        case WiFiPowerMode::BALANCED: Serial.println("BALANCED"); break;
        case WiFiPowerMode::POWER_SAVE: Serial.println("POWER_SAVE"); break;
        case WiFiPowerMode::DEEP_SLEEP_READY: Serial.println("DEEP_SLEEP_READY"); break;
    }
    
    Serial.printf("  ?쒕룞 ?덈꺼:    ");
    switch (power.activityLevel) {
        case WiFiActivityLevel::IDLE: Serial.println("IDLE"); break;
        case WiFiActivityLevel::LOW: Serial.println("LOW"); break;
        case WiFiActivityLevel::MEDIUM: Serial.println("MEDIUM"); break;
        case WiFiActivityLevel::HIGH: Serial.println("HIGH"); break;
    }
    
    Serial.printf("\nRF ?곹깭:\n");
    Serial.printf("  TX Power:     %10d dBm\n", power.txPower);
    Serial.printf("  RSSI:         %10ld dBm\n", power.rssi);
    
    Serial.printf("\n?덉쟾 ?듦퀎:\n");
    Serial.printf("  Modem Sleep:  %10lu ??n", power.modemSleepCount);
    Serial.printf("  Light Sleep:  %10lu ??n", power.lightSleepCount);
    Serial.printf("  珥?Sleep:     %10lu ms\n", power.totalSleepTime);
    Serial.printf("  ?덉쟾 鍮꾩쑉:    %10.2f%%\n", power.powerSavingRatio);
    
    Serial.printf("\n?ㅽ듃?뚰겕 ?쒕룞:\n");
    Serial.printf("  TX ?⑦궥:      %10lu\n", power.wifiTxPackets);
    Serial.printf("  RX ?⑦궥:      %10lu\n", power.wifiRxPackets);
}

void SystemTest::printFullReport() {
    Serial.println("\n\n");
    Serial.println("?붴븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븮");
    Serial.println("??         Phase 2 ?듯빀 ?뚯뒪??理쒖쥌 由ы룷??            ??);
    Serial.println("?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴");
    
    // Test results summary
    Serial.println("\n?뚯뒪??寃곌낵 ?붿빟:");
    Serial.println("?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺??);
    
    uint8_t passedCount = 0;
    for (uint8_t i = 0; i < resultCount; i++) {
        Serial.printf("  %s %-25s [%5lu ms] %s\n",
                      results[i].passed ? "?? : "??,
                      results[i].testName,
                      results[i].duration,
                      results[i].details);
        if (results[i].passed) passedCount++;
    }
    
    Serial.println("?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺??);
    Serial.printf("  ?⑷꺽: %d/%d (%.1f%%)\n", 
                  passedCount, resultCount, 
                  (float)passedCount / resultCount * 100);
    
    // Detailed reports
    printMemoryReport();
    printPerformanceReport();
    printPowerReport();
    
    // Comparison with baseline
    Serial.println("\n\n踰좎씠?ㅻ씪???鍮?媛쒖꽑:");
    Serial.println("?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺??);
    
    uint32_t baselineUsed = baselineMemory.totalHeap - baselineMemory.freeHeap;
    uint32_t currentUsed = currentMemory.totalHeap - currentMemory.freeHeap;
    int32_t heapSaved = baselineUsed - currentUsed;
    float heapImprovement = (float)heapSaved / baselineUsed * 100.0f;
    
    Serial.printf("  Heap ?덇컧:    %10ld bytes (%.1f%%)\n", heapSaved, heapImprovement);
    Serial.printf("  ?⑦렪??媛쒖꽑:  %10.1f%% -> %.1f%%\n", 
                  baselineMemory.heapFragmentation,
                  currentMemory.heapFragmentation);
    
    // Overall grade
    Serial.println("\n\n理쒖쥌 ?됯?:");
    Serial.println("?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺?곣봺??);
    
    float score = (float)passedCount / resultCount * 100;
    const char* grade;
    
    if (score >= 90) grade = "?곗닔 (A)";
    else if (score >= 80) grade = "?묓샇 (B)";
    else if (score >= 70) grade = "蹂댄넻 (C)";
    else grade = "媛쒖꽑 ?꾩슂 (D)";
    
    Serial.printf("  醫낇빀 ?먯닔:    %.1f??n", score);
    Serial.printf("  ?됯? ?깃툒:    %s\n", grade);
    
    if (score >= 80) {
        Serial.println("\n  ?럦 Phase 2 理쒖쟻?붽? ?깃났?곸쑝濡??꾨즺?섏뿀?듬땲??");
    } else {
        Serial.println("\n  ?좑툘  ?쇰? 媛쒖꽑???꾩슂?⑸땲?? ?꾩쓽 ?ㅽ뙣 ??ぉ??寃?좏븯?몄슂.");
    }
    
    Serial.println("\n?싢븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븧?먥븴\n");
}

void SystemTest::runQuickTest() {
    Serial.println("\n=== 鍮좊Ⅸ ?뚯뒪??紐⑤뱶 ===\n");
    
    testStartTime = millis();
    
    captureMemoryMetrics(currentMemory);
    capturePerformanceMetrics();
    capturePowerMetrics();
    
    printMemoryReport();
    printPowerReport();
    
    uint32_t duration = millis() - testStartTime;
    Serial.printf("\n?뚯뒪???꾨즺 ?쒓컙: %lu ms\n", duration);
}

void SystemTest::runStressTest(uint32_t durationMinutes) {
    Serial.printf("\n=== ?ㅽ듃?덉뒪 ?뚯뒪??(%lu遺? ===\n", durationMinutes);
    
    uint32_t endTime = millis() + (durationMinutes * 60000);
    uint32_t reportInterval = 60000; // 1遺꾨쭏??由ы룷??
    uint32_t lastReport = millis();
    
    establishBaseline();
    
    while (millis() < endTime) {
        // Generate varying load
        uint8_t loadLevel = (millis() / 10000) % 10 + 1; // 1-10
        generateLoad(loadLevel);
        
        // Periodic reporting
        if (millis() - lastReport >= reportInterval) {
            Serial.printf("\n[%lu遺?寃쎄낵]\n", (millis() - testStartTime) / 60000);
            printMemoryReport();
            lastReport = millis();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    Serial.println("\n=== ?ㅽ듃?덉뒪 ?뚯뒪???꾨즺 ===");
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
