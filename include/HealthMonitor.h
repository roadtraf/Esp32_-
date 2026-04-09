/*
 * HealthMonitor.h
 * ESP32-S3 Vacuum Control System v3.9.1 Phase 1
 * System Health Monitoring & Predictive Maintenance
 * String  char[] 
 */

#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <Arduino.h>

//  
#define HEALTH_EXCELLENT    90.0f
#define HEALTH_GOOD         75.0f
#define HEALTH_WARNING      50.0f
#define HEALTH_CRITICAL     25.0f

//   
enum MaintenanceLevel {
    MAINTENANCE_NONE = 0,
    MAINTENANCE_SOON = 1,
    MAINTENANCE_REQUIRED = 2,
    MAINTENANCE_URGENT = 3
};

//  
struct HealthFactors {
    float pumpEfficiency;      //   (0-100%)
    float temperatureHealth;   //   (0-100%)
    float currentHealth;       //   (0-100%)
    float runtimeHealth;       //   (0-100%)
};

class HealthMonitor {
private:
    float currentHealthScore;
    HealthFactors factors;
    MaintenanceLevel maintenanceLevel;
    
    //  
    unsigned long totalRuntime;        //   ()
    unsigned long lastMaintenanceTime; //   
    
    //  
    float avgVacuumAchieveTime;        //    
    float avgCurrentConsumption;       //   
    float peakTemperature;             //  
    float peakCurrent;
    //  
    int overTempCount;
    int overCurrentCount;
    int lowVacuumCount;

public:
    HealthMonitor();
    float getPeakTemperature() const { return peakTemperature; }
    float getPeakCurrent()     const { return peakCurrent; }
    // 
    void begin();
    void update(float pressure, float temperature, float current, uint8_t pwm, int state);
    void reset();
    void printStatus() const;
    const char* getMaintenanceLevelString() const;
    
    //  
    float calculateHealthScore(
        float vacuumPressure,
        float targetPressure,
        float temperature,
        float current,
        unsigned long runtime
    );
    
    //    
    float calculatePumpEfficiency(float vacuumPressure, float targetPressure);
    float calculateTemperatureHealth(float temperature);
    float calculateCurrentHealth(float current);
    float calculateRuntimeHealth(unsigned long runtime);
    
    //   
    MaintenanceLevel determineMaintenanceLevel();
    
    //  
    void updateRuntime(unsigned long seconds);
    void recordTemperature(float temp);
    void recordCurrent(float curr);
    void recordVacuumAchieveTime(float seconds);
    
    //  
    void recordOverTemp();
    void recordOverCurrent();
    void recordLowVacuum();
    
    //   
    void performMaintenance();
    
    // Getter
    float getHealthScore() const;
    HealthFactors getHealthFactors() { return factors; }
    MaintenanceLevel getMaintenanceLevel() const;
    unsigned long getTotalRuntime() { return totalRuntime; }
    unsigned long getTimeSinceLastMaintenance();
    
    // v3.9.1: String  const char* 
    const char* getMaintenanceMessage();
    void getDetailedMaintenanceAdvice(char* buffer, size_t size);
};

#endif // HEALTH_MONITOR_H
