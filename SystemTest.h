// SystemTest.h
#ifndef SYSTEM_TEST_H
#define SYSTEM_TEST_H

#include <Arduino.h>
#include "TaskConfig.h"
#include "MemoryPool.h"
#include "SensorBuffer.h"
#include "WiFiPowerManager.h"

// Test Result Structure
struct TestResult {
    const char* testName;
    bool passed;
    uint32_t duration;
    const char* details;
};

// Memory Metrics
struct MemoryMetrics {
    uint32_t totalHeap;
    uint32_t freeHeap;
    uint32_t minFreeHeap;
    uint32_t maxAllocHeap;
    uint32_t heapFragmentation;
    
    // PSRAM (if available)
    uint32_t totalPSRAM;
    uint32_t freePSRAM;
    uint32_t minFreePSRAM;
    
    // Stack metrics per task
    struct TaskStack {
        const char* taskName;
        TaskHandle_t handle;
        uint32_t stackSize;
        uint32_t stackHighWaterMark;
        float stackUsagePercent;
    };
    
    TaskStack tasks[10];
    uint8_t taskCount;
};

// Performance Metrics
struct PerformanceMetrics {
    uint32_t loopRate;           // loops per second
    uint32_t avgLoopTime;        // microseconds
    uint32_t maxLoopTime;        // microseconds
    
    uint32_t sensorReadRate;     // reads per second
    uint32_t uiUpdateRate;       // updates per second
    
    uint32_t mqttPublishRate;    // messages per second
    uint32_t mqttLatency;        // milliseconds
    
    float cpuUsage;              // percentage
    float coreUsage[2];          // percentage per core
};

// Power Metrics
struct PowerMetrics {
    WiFiPowerMode currentMode;
    WiFiActivityLevel activityLevel;
    
    int8_t txPower;              // dBm
    int32_t rssi;                // dBm
    
    uint32_t modemSleepCount;
    uint32_t lightSleepCount;
    uint32_t totalSleepTime;
    float powerSavingRatio;
    
    uint32_t wifiTxPackets;
    uint32_t wifiRxPackets;
};

// System Test Class
class SystemTest {
private:
    bool testRunning;
    uint32_t testStartTime;
    
    MemoryMetrics baselineMemory;
    MemoryMetrics currentMemory;
    
    PerformanceMetrics performance;
    PowerMetrics power;
    
    TestResult results[20];
    uint8_t resultCount;
    
    // Test methods
    bool testMemoryOptimization();
    bool testRTOSTasks();
    bool testMemoryPools();
    bool testSensorBuffers();
    bool testWiFiPowerManagement();
    bool testSystemStability();
    
    // Utility methods
    void captureMemoryMetrics(MemoryMetrics& metrics);
    void capturePerformanceMetrics();
    void capturePowerMetrics();
    void addTestResult(const char* name, bool passed, uint32_t duration, const char* details);
    
public:
    SystemTest();
    
    // Main test functions
    void runAllTests();
    void runQuickTest();
    void runStressTest(uint32_t durationMinutes);
    
    // Individual test categories
    void runMemoryTests();
    void runPerformanceTests();
    void runPowerTests();
    
    // Baseline establishment
    void establishBaseline();
    
    // Metrics capture
    void captureCurrentMetrics();
    
    // Comparison and reporting
    void compareWithBaseline();
    void printMemoryReport();
    void printPerformanceReport();
    void printPowerReport();
    void printFullReport();
    
    // Continuous monitoring
    void startMonitoring(uint32_t intervalSeconds);
    void stopMonitoring();
    void updateMonitoring();
    
    // Stress test
    void generateLoad(uint8_t level); // 1-10
};

extern SystemTest systemTest;

#endif // SYSTEM_TEST_H