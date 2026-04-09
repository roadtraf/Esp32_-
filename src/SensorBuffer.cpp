// SensorBuffer.cpp
#include "SensorBuffer.h"
#include "Sensor.h"

// ================================================================
//   
// ================================================================
RingBuffer<float, TEMP_BUFFER_SIZE> temperatureBuffer;
RingBuffer<float, PRESSURE_BUFFER_SIZE> pressureBuffer;
RingBuffer<float, CURRENT_BUFFER_SIZE> currentBuffer;
RingBuffer<SensorData, SENSOR_DATA_BUFFER_SIZE> sensorDataBuffer;

// ================================================================
//    
// ================================================================
void updateSensorBuffers() {
    //   
    float temperature = readTemperature();
    float pressure = readPressure();
    float current = readCurrent();
    bool limitSwitch = readLimitSwitch();
    bool photoSensor = readPhotoSensor();
    bool emergencyStop = readEmergencyStop();
    uint32_t timestamp = millis();
    
    //   
    temperatureBuffer.push(temperature);
    pressureBuffer.push(pressure);
    currentBuffer.push(current);
    
    //     
    SensorData data = {pressure, current, temperature, limitSwitch, photoSensor, emergencyStop, timestamp};
    sensorDataBuffer.push(data);
}

// ================================================================
//  
// ================================================================
void calculateSensorStats(SensorStats& stats) {
    stats.avgTemperature = temperatureBuffer.getAverage();
    stats.maxTemperature = temperatureBuffer.getMax();
    stats.minTemperature = temperatureBuffer.getMin();
    stats.tempStdDev = temperatureBuffer.getStdDev();
    
    stats.avgPressure = pressureBuffer.getAverage();
    stats.maxPressure = pressureBuffer.getMax();
    stats.minPressure = pressureBuffer.getMin();
    stats.pressureStdDev = pressureBuffer.getStdDev();
    
    stats.avgCurrent = currentBuffer.getAverage();
    stats.maxCurrent = currentBuffer.getMax();
    stats.minCurrent = currentBuffer.getMin();
    stats.currentStdDev = currentBuffer.getStdDev();
    
    stats.sampleCount = temperatureBuffer.size();
}

// ================================================================
//  
// ================================================================
void clearSensorBuffers() {
    temperatureBuffer.clear();
    pressureBuffer.clear();
    currentBuffer.clear();
    sensorDataBuffer.clear();
}

// ================================================================
//   
// ================================================================
void printBufferStatus() {
    Serial.println("\n==========    ==========");
    
    Serial.printf(" : %zu/%d (%.1f%%)\n",
                  temperatureBuffer.size(), TEMP_BUFFER_SIZE,
                  (float)temperatureBuffer.size() / TEMP_BUFFER_SIZE * 100);
    
    Serial.printf(" : %zu/%d (%.1f%%)\n",
                  pressureBuffer.size(), PRESSURE_BUFFER_SIZE,
                  (float)pressureBuffer.size() / PRESSURE_BUFFER_SIZE * 100);
    
    Serial.printf(" : %zu/%d (%.1f%%)\n",
                  currentBuffer.size(), CURRENT_BUFFER_SIZE,
                  (float)currentBuffer.size() / CURRENT_BUFFER_SIZE * 100);
    
    Serial.printf(" : %zu/%d (%.1f%%)\n",
                  sensorDataBuffer.size(), SENSOR_DATA_BUFFER_SIZE,
                  (float)sensorDataBuffer.size() / SENSOR_DATA_BUFFER_SIZE * 100);
    
    //  
    if (temperatureBuffer.size() > 0) {
        SensorStats stats;
        calculateSensorStats(stats);
        
        Serial.println("\n==========   ==========");
        Serial.printf(": %.2fC (%.2f ~ %.2f) =%.2f\n", 
                      stats.avgTemperature, stats.minTemperature, 
                      stats.maxTemperature, stats.tempStdDev);
        Serial.printf(": %.2fkPa (%.2f ~ %.2f) =%.2f\n", 
                      stats.avgPressure, stats.minPressure, 
                      stats.maxPressure, stats.pressureStdDev);
        Serial.printf(": %.2fA (%.2f ~ %.2f) =%.2f\n", 
                      stats.avgCurrent, stats.minCurrent, 
                      stats.maxCurrent, stats.currentStdDev);
        Serial.printf(" : %lu\n", stats.sampleCount);
    } else {
        Serial.println("\n(  )");
    }
    
    Serial.println("=====================================\n");
}

// ================================================================
//   
// ================================================================
float getAvgTemperature() {
    return temperatureBuffer.getAverage();
}

float getAvgPressure() {
    return pressureBuffer.getAverage();
}

float getAvgCurrent() {
    return currentBuffer.getAverage();
}

float getMaxTemperature() {
    return temperatureBuffer.getMax();
}

float getMaxPressure() {
    return pressureBuffer.getMax();
}

float getMaxCurrent() {
    return currentBuffer.getMax();
}

float getMinTemperature() {
    return temperatureBuffer.getMin();
}

float getMinPressure() {
    return pressureBuffer.getMin();
}

float getMinCurrent() {
    return currentBuffer.getMin();
}