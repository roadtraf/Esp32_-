// ================================================================
// SensorManager.cpp -     (v3.9.2 Phase 3-1 - )
// ================================================================

#include "SensorManager.h"
#include "Config.h"
#include "Sensor.h"  //   

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//   
SensorManager sensorManager;

// ================================================================
// 
// ================================================================
void SensorManager::begin() {
    Serial.println("[SensorManager]  ...");
    
    //   
    sensorData.pressure = 0.0f;
    sensorData.current = 0.0f;
    sensorData.temperature = 0.0f;
    sensorData.limitSwitch = false;
    sensorData.photoSensor = false;
    sensorData.emergencyStop = false;
    sensorData.timestamp = millis();
    
    //  
    pressureBuffer.reserve(100);
    temperatureBuffer.reserve(100);
    currentBuffer.reserve(100);
    
    //  
    pressureOffset = 0.0f;
    currentOffset = 0.0f;
    
    //     
    initSensors();
    
    Serial.println("[SensorManager]  ");
}

// ================================================================
//  
// ================================================================
void SensorManager::readAllSensors() {
    //   
    sensorData.pressure = readPressureSensor();
    sensorData.current = readCurrentSensor();
    sensorData.temperature = readTemperatureSensor();
    sensorData.limitSwitch = readLimitSwitchSensor();
    sensorData.photoSensor = readPhotoSensorInput();
    sensorData.emergencyStop = readEmergencyStopInput();
    sensorData.timestamp = millis();
}

// ================================================================
//    
// ================================================================
float SensorManager::readPressureSensor() {
    //   
    float raw = readPressure();
    return raw - pressureOffset;
}

float SensorManager::readCurrentSensor() {
    float raw = readCurrent();
    return raw - currentOffset;
}

float SensorManager::readTemperatureSensor() {
    return readTemperature();
}

bool SensorManager::readLimitSwitchSensor() {
    return readLimitSwitch();
}

bool SensorManager::readPhotoSensorInput() {
    return readPhotoSensor();
}

bool SensorManager::readEmergencyStopInput() {
    return readEmergencyStop();
}

// ================================================================
//  
// ================================================================
void SensorManager::updateBuffers() {
    addToBuffer(pressureBuffer, sensorData.pressure, 100);
    addToBuffer(temperatureBuffer, sensorData.temperature, 100);
    addToBuffer(currentBuffer, sensorData.current, 100);
}

void SensorManager::clearBuffers() {
    pressureBuffer.clear();
    temperatureBuffer.clear();
    currentBuffer.clear();
}

void SensorManager::addToBuffer(std::vector<float>& buffer, float value, size_t maxSize) {
    buffer.push_back(value);
    if (buffer.size() > maxSize) {
        buffer.erase(buffer.begin());
    }
}

// ================================================================
// 
// ================================================================
void SensorManager::calibratePressure() {
    Serial.println("[SensorManager]   ...");
    
    //    
    float sum = 0.0f;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += readPressure();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    pressureOffset = sum / samples;
    
    Serial.printf("[SensorManager]  : %.2f\n", pressureOffset);
    Serial.println("[SensorManager]   ");
}

void SensorManager::calibrateCurrent() {
    Serial.println("[SensorManager]   ...");
    
    float sum = 0.0f;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += readCurrent();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    currentOffset = sum / samples;
    
    Serial.printf("[SensorManager]  : %.2f\n", currentOffset);
    Serial.println("[SensorManager]   ");
}

// ================================================================
//   
// ================================================================
bool SensorManager::checkSensorHealth() {
    bool healthy = true;
    
    //   
    if (isnan(sensorData.pressure) || sensorData.pressure < -200.0f || sensorData.pressure > 200.0f) {
        Serial.println("[SensorManager]   ");
        healthy = false;
    }
    
    //   
    if (isnan(sensorData.current) || sensorData.current < 0.0f || sensorData.current > 10.0f) {
        Serial.println("[SensorManager]   ");
        healthy = false;
    }
    
    //   
    if (isnan(sensorData.temperature) || sensorData.temperature < -50.0f || sensorData.temperature > 100.0f) {
        Serial.println("[SensorManager]   ");
        healthy = false;
    }
    
    return healthy;
}

// ================================================================
//  
// ================================================================
float SensorManager::calculateAverage(const std::vector<float>& buffer, uint8_t samples) {
    if (buffer.empty()) return 0.0f;
    
    size_t count = min((size_t)samples, buffer.size());
    float sum = 0.0f;
    
    //  N  
    for (size_t i = buffer.size() - count; i < buffer.size(); i++) {
        sum += buffer[i];
    }
    
    return sum / count;
}

float SensorManager::getPressureAverage(uint8_t samples) {
    return calculateAverage(pressureBuffer, samples);
}

float SensorManager::getTemperatureAverage(uint8_t samples) {
    return calculateAverage(temperatureBuffer, samples);
}

float SensorManager::getCurrentAverage(uint8_t samples) {
    return calculateAverage(currentBuffer, samples);
}

// ================================================================
//  
// ================================================================
void SensorManager::printStatus() const {
    Serial.println("\n===   ===");
    Serial.printf(":     %.2f kPa\n", sensorData.pressure);
    Serial.printf(":     %.2f A\n", sensorData.current);
    Serial.printf(":     %.2f C\n", sensorData.temperature);
    Serial.printf("SW:   %s\n", sensorData.limitSwitch ? "ON" : "OFF");
    Serial.printf(":   %s\n", sensorData.photoSensor ? "" : "");
    Serial.printf(": %s\n", sensorData.emergencyStop ? "" : "");
    Serial.printf(": %lu ms\n", sensorData.timestamp);
    Serial.println("==================\n");
}
