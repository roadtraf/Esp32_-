// ================================================================
// MLPredictor.cpp - ML 예측기 (v3.9.2 수정)
// sensorData → sensorManager 변경
// ================================================================

#include "../include/MLPredictor.h"
#include "../include/Config.h"
#include "SensorManager.h"  // ← 추가

extern SensorManager sensorManager;

MLPredictor::MLPredictor() {
    sampleCount = 0;
    predictionReady = false;
}

void MLPredictor::begin() {
    Serial.println("[MLPredictor] 초기화 완료");
    sampleCount = 0;
    predictionReady = false;
}

void MLPredictor::addSample(float pressure, float temperature, float current) {
    if (sampleCount < MAX_SAMPLES) {
        samples[sampleCount].pressure = pressure;
        samples[sampleCount].temperature = temperature;
        samples[sampleCount].current = current;
        samples[sampleCount].timestamp = millis();
        sampleCount++;
        
        // 충분한 샘플이 모이면 예측 가능
        if (sampleCount >= 10) {
            predictionReady = true;
        }
    }
}

float MLPredictor::predict() {
    if (!predictionReady || sampleCount < 10) {
        return -1.0f;  // 예측 불가
    }
    
    // 간단한 선형 회귀 예측
    float avgPressure = 0;
    float avgCurrent = 0;
    
    for (int i = 0; i < sampleCount; i++) {
        avgPressure += samples[i].pressure;
        avgCurrent += samples[i].current;
    }
    
    avgPressure /= sampleCount;
    avgCurrent /= sampleCount;
    
    // 압력과 전류의 추세로 고장 가능성 예측
    float prediction = 100.0f;
    
    if (avgPressure > -70.0f) prediction -= 20.0f;
    if (avgCurrent > 4.0f) prediction -= 30.0f;
    
    return max(0.0f, prediction);
}

void MLPredictor::reset() {
    sampleCount = 0;
    predictionReady = false;
}

bool MLPredictor::isPredictionReady() const {
    return predictionReady;
}

void MLPredictor::printStatus() const {
    Serial.println("\n=== ML 예측기 ===");
    Serial.printf("샘플 수: %d/%d\n", sampleCount, MAX_SAMPLES);
    Serial.printf("예측 가능: %s\n", predictionReady ? "예" : "아니오");
    
    if (predictionReady) {
        Serial.printf("예측 신뢰도: %.1f%%\n", predict());
    }
    Serial.println("==================\n");
}

// ================================================================
// 이상 감지 메시지 반환 (v3.9.3: const char* 반환)
// ================================================================
const char* MLPredictor::getAnomalyMessage(AnomalyType type) {
    switch (type) {
        case ANOMALY_PRESSURE:    return "압력 이상 감지";
        case ANOMALY_TEMPERATURE: return "온도 이상 감지";
        case ANOMALY_CURRENT:     return "전류 이상 감지";
        case ANOMALY_VACUUM:      return "진공 이상 감지";
        default:                  return "알 수 없는 이상";
    }
}
