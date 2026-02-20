// SensorBuffer.cpp
#include "../include/SensorBuffer.h"
#include "../include/Sensor.h"

// ================================================================
// 전역 버퍼 초기화
// ================================================================
RingBuffer<float, TEMP_BUFFER_SIZE> temperatureBuffer;
RingBuffer<float, PRESSURE_BUFFER_SIZE> pressureBuffer;
RingBuffer<float, CURRENT_BUFFER_SIZE> currentBuffer;
RingBuffer<SensorData, SENSOR_DATA_BUFFER_SIZE> sensorDataBuffer;

// ================================================================
// 센서 데이터를 버퍼에 추가
// ================================================================
void updateSensorBuffers() {
    // 센서 값 읽기
    float temperature = readTemperature();
    float pressure = readPressure();
    float current = readCurrent();
    bool limitSwitch = readLimitSwitch();
    bool photoSensor = readPhotoSensor();
    bool emergencyStop = readEmergencyStop();
    uint32_t timestamp = millis();
    
    // 개별 버퍼에 추가
    temperatureBuffer.push(temperature);
    pressureBuffer.push(pressure);
    currentBuffer.push(current);
    
    // 통합 센서 데이터 버퍼에 추가
    SensorData data(temperature, pressure, current, limitSwitch, photoSensor, emergencyStop, timestamp);
    sensorDataBuffer.push(data);
}

// ================================================================
// 통계 계산
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
// 버퍼 초기화
// ================================================================
void clearSensorBuffers() {
    temperatureBuffer.clear();
    pressureBuffer.clear();
    currentBuffer.clear();
    sensorDataBuffer.clear();
}

// ================================================================
// 버퍼 상태 출력
// ================================================================
void printBufferStatus() {
    Serial.println("\n========== 센서 버퍼 상태 ==========");
    
    Serial.printf("온도 버퍼: %zu/%d (%.1f%%)\n",
                  temperatureBuffer.size(), TEMP_BUFFER_SIZE,
                  (float)temperatureBuffer.size() / TEMP_BUFFER_SIZE * 100);
    
    Serial.printf("압력 버퍼: %zu/%d (%.1f%%)\n",
                  pressureBuffer.size(), PRESSURE_BUFFER_SIZE,
                  (float)pressureBuffer.size() / PRESSURE_BUFFER_SIZE * 100);
    
    Serial.printf("전류 버퍼: %zu/%d (%.1f%%)\n",
                  currentBuffer.size(), CURRENT_BUFFER_SIZE,
                  (float)currentBuffer.size() / CURRENT_BUFFER_SIZE * 100);
    
    Serial.printf("통합 버퍼: %zu/%d (%.1f%%)\n",
                  sensorDataBuffer.size(), SENSOR_DATA_BUFFER_SIZE,
                  (float)sensorDataBuffer.size() / SENSOR_DATA_BUFFER_SIZE * 100);
    
    // 통계 출력
    if (temperatureBuffer.size() > 0) {
        SensorStats stats;
        calculateSensorStats(stats);
        
        Serial.println("\n========== 센서 통계 ==========");
        Serial.printf("온도: %.2f°C (%.2f ~ %.2f) σ=%.2f\n", 
                      stats.avgTemperature, stats.minTemperature, 
                      stats.maxTemperature, stats.tempStdDev);
        Serial.printf("압력: %.2fkPa (%.2f ~ %.2f) σ=%.2f\n", 
                      stats.avgPressure, stats.minPressure, 
                      stats.maxPressure, stats.pressureStdDev);
        Serial.printf("전류: %.2fA (%.2f ~ %.2f) σ=%.2f\n", 
                      stats.avgCurrent, stats.minCurrent, 
                      stats.maxCurrent, stats.currentStdDev);
        Serial.printf("샘플 수: %lu\n", stats.sampleCount);
    } else {
        Serial.println("\n(아직 데이터 없음)");
    }
    
    Serial.println("=====================================\n");
}

// ================================================================
// 빠른 조회 함수들
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