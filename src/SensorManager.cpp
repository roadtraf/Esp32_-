// ================================================================
// SensorManager.cpp - 센서 관리 모듈 구현 (v3.9.2 Phase 3-1 - 개선)
// ================================================================

#include "SensorManager.h"
#include "../include/Config.h"
#include "Sensor.h"  // 기존 센서 함수들

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 전역 인스턴스 정의
SensorManager sensorManager;

// ================================================================
// 초기화
// ================================================================
void SensorManager::begin() {
    Serial.println("[SensorManager] 초기화 시작...");
    
    // 센서 데이터 초기화
    sensorData.pressure = 0.0f;
    sensorData.current = 0.0f;
    sensorData.temperature = 0.0f;
    sensorData.limitSwitch = false;
    sensorData.photoSensor = false;
    sensorData.emergencyStop = false;
    sensorData.timestamp = millis();
    
    // 버퍼 초기화
    pressureBuffer.reserve(100);
    temperatureBuffer.reserve(100);
    currentBuffer.reserve(100);
    
    // 오프셋 초기화
    pressureOffset = 0.0f;
    currentOffset = 0.0f;
    
    // 기존 센서 초기화 함수 호출
    initSensor();
    
    Serial.println("[SensorManager] 초기화 완료");
}

// ================================================================
// 센서 읽기
// ================================================================
void SensorManager::readAllSensors() {
    // 개별 센서 읽기
    sensorData.pressure = readPressureSensor();
    sensorData.current = readCurrentSensor();
    sensorData.temperature = readTemperatureSensor();
    sensorData.limitSwitch = readLimitSwitchSensor();
    sensorData.photoSensor = readPhotoSensorInput();
    sensorData.emergencyStop = readEmergencyStopInput();
    sensorData.timestamp = millis();
}

// ================================================================
// 내부 센서 읽기 함수들
// ================================================================
float SensorManager::readPressureSensor() {
    // 기존 함수 호출
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
// 버퍼 관리
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
// 캘리브레이션
// ================================================================
void SensorManager::calibratePressure() {
    Serial.println("[SensorManager] 압력 센서 캘리브레이션...");
    
    // 여러 번 읽어서 평균
    float sum = 0.0f;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += readPressure();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    pressureOffset = sum / samples;
    
    Serial.printf("[SensorManager] 압력 오프셋: %.2f\n", pressureOffset);
    Serial.println("[SensorManager] 압력 캘리브레이션 완료");
}

void SensorManager::calibrateCurrent() {
    Serial.println("[SensorManager] 전류 센서 캘리브레이션...");
    
    float sum = 0.0f;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += readCurrent();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    currentOffset = sum / samples;
    
    Serial.printf("[SensorManager] 전류 오프셋: %.2f\n", currentOffset);
    Serial.println("[SensorManager] 전류 캘리브레이션 완료");
}

// ================================================================
// 센서 건강 체크
// ================================================================
bool SensorManager::checkSensorHealth() {
    bool healthy = true;
    
    // 압력 센서 체크
    if (isnan(sensorData.pressure) || sensorData.pressure < -200.0f || sensorData.pressure > 200.0f) {
        Serial.println("[SensorManager] 압력 센서 이상");
        healthy = false;
    }
    
    // 전류 센서 체크
    if (isnan(sensorData.current) || sensorData.current < 0.0f || sensorData.current > 10.0f) {
        Serial.println("[SensorManager] 전류 센서 이상");
        healthy = false;
    }
    
    // 온도 센서 체크
    if (isnan(sensorData.temperature) || sensorData.temperature < -50.0f || sensorData.temperature > 100.0f) {
        Serial.println("[SensorManager] 온도 센서 이상");
        healthy = false;
    }
    
    return healthy;
}

// ================================================================
// 통계 계산
// ================================================================
float SensorManager::calculateAverage(const std::vector<float>& buffer, uint8_t samples) {
    if (buffer.empty()) return 0.0f;
    
    size_t count = min((size_t)samples, buffer.size());
    float sum = 0.0f;
    
    // 최근 N개 샘플의 평균
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
// 상태 출력
// ================================================================
void SensorManager::printStatus() const {
    Serial.println("\n=== 센서 상태 ===");
    Serial.printf("압력:     %.2f kPa\n", sensorData.pressure);
    Serial.printf("전류:     %.2f A\n", sensorData.current);
    Serial.printf("온도:     %.2f °C\n", sensorData.temperature);
    Serial.printf("리밋SW:   %s\n", sensorData.limitSwitch ? "ON" : "OFF");
    Serial.printf("광센서:   %s\n", sensorData.photoSensor ? "감지" : "없음");
    Serial.printf("비상정지: %s\n", sensorData.emergencyStop ? "눌림" : "정상");
    Serial.printf("타임스탬프: %lu ms\n", sensorData.timestamp);
    Serial.println("==================\n");
}
