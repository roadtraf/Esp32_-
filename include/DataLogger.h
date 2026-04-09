// ================================================================
// DataLogger.h    v3.9.1 Phase 1 
//      
// String  char[] 
// ================================================================
#pragma once
#include "Config.h"
#include "HealthMonitor.h"
#include <SD.h>

//     
struct HealthLogEntry {
    uint32_t timestamp;           // Unix timestamp
    float healthScore;            //  
    float pumpEfficiency;         //  
    float temperatureHealth;      //  
    float currentHealth;          //  
    float runtimeHealth;          //  
    float pressure;               //  
    float temperature;            //  
    float current;                //  
    MaintenanceLevel maintenanceLevel;  //  
};

struct TrendStatistics {
    float avg_24h;                // 24 
    float avg_7d;                 // 7 
    float avg_30d;                // 30 
    float trend;                  //  (=, =)
    float volatility;             // 
    uint32_t lastUpdate;          //   
};

//  DataLogger  
class DataLogger {
public:
    DataLogger();
    
    // 
    void begin();
    
    //  
    void logHealthData(const HealthMonitor& healthMonitor);
    void logHealthDataDetailed(float health, float pumpEff, float tempHealth, 
                               float currentHealth, float runtimeHealth,
                               MaintenanceLevel level);
    
    //   
    void logMaintenance(float healthBefore, float healthAfter, const char* note);
    
    //  
    TrendStatistics calculateTrend(uint16_t hours);  //  N 
    TrendStatistics getDailyTrend();                 // 24 
    TrendStatistics getWeeklyTrend();                // 7 
    TrendStatistics getMonthlyTrend();               // 30 
    
    //  
    bool getWeeklyHealthHistory(float* values, int& count);
    bool readHealthHistory(HealthLogEntry* buffer, uint16_t maxCount, uint16_t& actualCount);
    bool readMaintenanceHistory(char* buffer, uint16_t maxSize);
    
    // CSV 
    bool exportHealthToCSV(const char* filename);
    bool exportMaintenanceToCSV(const char* filename);
    
    //  
    float predictHealthScore(uint8_t hoursAhead);   // N   
    uint32_t estimateDaysToMaintenance();           //   
    
    //  
    void clearOldLogs(uint16_t daysToKeep);
    uint32_t getLogSize();
    uint32_t getLogCount();
    
private:
    bool initialized;
    uint32_t lastLogTime;
    uint32_t logInterval;         //   ( 1)
    
    //  
    void ensureDirectories();
    void writeHealthEntry(const HealthLogEntry& entry);
    bool appendToMaintenanceLog(uint32_t timestamp, float before, float after, const char* note);
    
    //   
    float calculateLinearTrend(float* values, uint16_t count);
    float calculateVolatility(float* values, uint16_t count);
    float calculateAverage(float* values, uint16_t count);
    
    //  
    static const char* HEALTH_LOG_FILE;
    static const char* MAINTENANCE_LOG_FILE;
    static const char* TREND_DATA_FILE;
};

//    
extern DataLogger dataLogger;

//    (v3.9.1: String  char[]) 
void getTimestampString(uint32_t timestamp, char* buffer, size_t size);
uint32_t parseTimestamp(const char* timeStr);
