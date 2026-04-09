/*
 * MLPredictor.h
 * ESP32-S3 Vacuum Control System v3.6
 * Machine Learning Anomaly Detection
 */

#ifndef ML_PREDICTOR_H
#define ML_PREDICTOR_H

#include <Arduino.h>

//   
enum AnomalyType {
    ANOMALY_NONE = 0,
    ANOMALY_PRESSURE = 1,
    ANOMALY_TEMPERATURE = 2,
    ANOMALY_CURRENT = 3,
    ANOMALY_PATTERN = 4
};

//   
#include "SensorManager.h"  // SensorData 

//  
struct MLStatistics {
    float mean;
    float stdDev;
    float min;
    float max;
};

class MLPredictor {
private:
    //   
    static const int BUFFER_SIZE = 60;  // 60 
    SensorData dataBuffer[BUFFER_SIZE];
    int bufferIndex;
    int sampleCount;
    
    //   
    MLStatistics pressureStats;
    MLStatistics temperatureStats;
    MLStatistics currentStats;
    
    //    ( )
    float anomalyThreshold;
    
    //   
    AnomalyType lastAnomaly;
    unsigned long lastAnomalyTime;
    
    //  
    void updateStatistics();
    bool isOutlier(float value, MLStatistics stats);
    
    // v3.9.1:   
    float calculatePressureMean();
    float calculatePressureStdDev(float mean);
    void calculatePressureMinMax(float& min, float& max);
    
    float calculateTemperatureMean();
    float calculateTemperatureStdDev(float mean);
    void calculateTemperatureMinMax(float& min, float& max);
    
    float calculateCurrentMean();
    float calculateCurrentStdDev(float mean);
    void calculateCurrentMinMax(float& min, float& max);
    
    //   ( ,  )
    // float calculateMean(float* data, int count);
    // float calculateStdDev(float* data

public:
    MLPredictor();
    
    // 
    void begin();
    
    //    
    void addSample(float vacuumPressure, float temperature, float current);
    
    //  
    AnomalyType detectAnomaly(float vacuumPressure, float temperature, float current);
    
    // 
    float predictNextValue(float* recentValues, int count);
    
    // Getter
    AnomalyType getLastAnomaly() { return lastAnomaly; }
    unsigned long getLastAnomalyTime() { return lastAnomalyTime; }
    bool isLearned() { return sampleCount >= BUFFER_SIZE; }
    int getSampleCount() { return sampleCount; }
    
    //  
    MLStatistics getPressureStats() { return pressureStats; }
    MLStatistics getTemperatureStats() { return temperatureStats; }
    MLStatistics getCurrentStats() { return currentStats; }
    
    //    (v3.9.3: const char* )
    const char* getAnomalyMessage(AnomalyType type);
};

#endif // ML_PREDICTOR_H
