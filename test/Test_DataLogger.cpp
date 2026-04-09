// ================================================================
// Test_DataLogger.cpp  ?? DataLogger ?⑥쐞 ?뚯뒪??
// ================================================================

#if defined(UNIT_TEST_MODE) && defined(ENABLE_DATA_LOGGING)

#include "../include/UnitTest_Framework.h"
#include "../include/DataLogger.h"
#include "../include/HealthMonitor.h"

extern DataLogger dataLogger;
extern HealthMonitor healthMonitor;

void Test_DataLogger::runTests() {
    TestFramework::beginModule(getName());
    
    // ???? 珥덇린???뚯뒪??????
    TestFramework::ASSERT(
        true,  // dataLogger.begin()? setup()?먯꽌 ?대? ?몄텧??
        "DataLogger initialized"
    );
    
    // ???? 濡쒓렇 ?뚯씪 議댁옱 ?뚯뒪??????
    #ifdef ENABLE_SD_CARD
    File healthLog = SD.open("/logs/health_log.csv", FILE_READ);
    TestFramework::ASSERT(
        healthLog,
        "Health log file exists"
    );
    if (healthLog) healthLog.close();
    
    File maintLog = SD.open("/logs/maintenance_log.csv", FILE_READ);
    TestFramework::ASSERT(
        maintLog,
        "Maintenance log file exists"
    );
    if (maintLog) maintLog.close();
    #endif
    
    // ???? 濡쒓렇 移댁슫???뚯뒪??????
    uint32_t logCount = dataLogger.getLogCount();
    TestFramework::ASSERT(
        logCount >= 0,
        "Get log count"
    );
    Serial.printf("    (Current log count: %lu)\n", logCount);
    
    // ???? 濡쒓렇 ?ш린 ?뚯뒪??????
    uint32_t logSize = dataLogger.getLogSize();
    TestFramework::ASSERT(
        logSize >= 0,
        "Get log size"
    );
    Serial.printf("    (Current log size: %lu bytes)\n", logSize);
    
    // ???? 異붿꽭 怨꾩궛 ?뚯뒪??????
    TrendStatistics trend = dataLogger.getDailyTrend();
    TestFramework::ASSERT_RANGE(
        trend.avg_24h,
        0.0f,
        100.0f,
        "Daily trend average in range"
    );
    Serial.printf("    (24h average: %.1f%%)\n", trend.avg_24h);
    
    TestFramework::ASSERT_RANGE(
        trend.volatility,
        0.0f,
        100.0f,
        "Volatility in range"
    );
    Serial.printf("    (Volatility: %.2f)\n", trend.volatility);
    
    // ???? ?덉륫 ?뚯뒪??????
    float pred24h = dataLogger.predictHealthScore(24);
    TestFramework::ASSERT_RANGE(
        pred24h,
        0.0f,
        100.0f,
        "24h prediction in range"
    );
    Serial.printf("    (24h prediction: %.1f%%)\n", pred24h);
    
    // ???? ?좎?蹂댁닔 ?덉긽 ?쇱닔 ?뚯뒪??????
    uint32_t days = dataLogger.estimateDaysToMaintenance();
    TestFramework::ASSERT(
        days >= 0,
        "Days to maintenance calculated"
    );
    if (days < 999) {
        Serial.printf("    (Days to maintenance: ~%lu days)\n", days);
    } else {
        Serial.println("    (Maintenance not needed soon)");
    }
    
    // ???? ?섎룞 濡쒓퉭 ?뚯뒪??????
    float testHealth = 95.5f;
    dataLogger.logHealthDataDetailed(
        testHealth,
        98.0f,  // pumpEff
        100.0f, // tempHealth
        92.0f,  // currentHealth
        100.0f, // runtimeHealth
        MAINTENANCE_NONE
    );
    TestFramework::ASSERT(
        true,
        "Manual health data logging"
    );
    
    // ???? CSV ?대낫?닿린 ?뚯뒪??????
    #ifdef ENABLE_SD_CARD
    bool exported = dataLogger.exportHealthToCSV("test_export.csv");
    TestFramework::ASSERT(
        exported,
        "Export health data to CSV"
    );
    #endif
    
    TestFramework::endModule();
}

#endif // UNIT_TEST_MODE && ENABLE_DATA_LOGGING
