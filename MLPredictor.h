/*
 * MLPredictor.h
 * ESP32-S3 Vacuum Control System v3.6
 * Machine Learning Anomaly Detection
 */

#ifndef ML_PREDICTOR_H
#define ML_PREDICTOR_H

#include <Arduino.h>

// 이상 감지 타입
enum AnomalyType {
    ANOMALY_NONE = 0,
    ANOMALY_PRESSURE = 1,
    ANOMALY_TEMPERATURE = 2,
    ANOMALY_CURRENT = 3,
    ANOMALY_PATTERN = 4
};

// 센서 데이터 구조
struct SensorData {
    float vacuumPressure;
    float temperature;
    float current;
    unsigned long timestamp;
};

// 통계 데이터
struct Statistics {
    float mean;
    float stdDev;
    float min;
    float max;
};

class MLPredictor {
private:
    // 학습 데이터 버퍼
    static const int BUFFER_SIZE = 60;  // 60개 샘플
    SensorData dataBuffer[BUFFER_SIZE];
    int bufferIndex;
    int sampleCount;
    
    // 정상 범위 통계
    Statistics pressureStats;
    Statistics temperatureStats;
    Statistics currentStats;
    
    // 이상 감지 임계값 (표준편차 배수)
    float anomalyThreshold;
    
    // 최근 이상 감지
    AnomalyType lastAnomaly;
    unsigned long lastAnomalyTime;
    
    // 내부 함수
    void updateStatistics();
    bool isOutlier(float value, Statistics stats);
    
    // v3.9.1: 제로카피 헬퍼 함수
    float calculatePressureMean();
    float calculatePressureStdDev(float mean);
    void calculatePressureMinMax(float& min, float& max);
    
    float calculateTemperatureMean();
    float calculateTemperatureStdDev(float mean);
    void calculateTemperatureMinMax(float& min, float& max);
    
    float calculateCurrentMean();
    float calculateCurrentStdDev(float mean);
    void calculateCurrentMinMax(float& min, float& max);
    
    // 레거시 함수 (사용 안함, 제거 가능)
    // float calculateMean(float* data, int count);
    // float calculateStdDev(float* data

public:
    MLPredictor();
    
    // 초기화
    void begin();
    
    // 데이터 추가 및 학습
    void addSample(float vacuumPressure, float temperature, float current);
    
    // 이상 감지
    AnomalyType detectAnomaly(float vacuumPressure, float temperature, float current);
    
    // 예측
    float predictNextValue(float* recentValues, int count);
    
    // Getter
    AnomalyType getLastAnomaly() { return lastAnomaly; }
    unsigned long getLastAnomalyTime() { return lastAnomalyTime; }
    bool isLearned() { return sampleCount >= BUFFER_SIZE; }
    int getSampleCount() { return sampleCount; }
    
    // 통계 정보
    Statistics getPressureStats() { return pressureStats; }
    Statistics getTemperatureStats() { return temperatureStats; }
    Statistics getCurrentStats() { return currentStats; }
    
    // 이상 감지 메시지 (v3.9.3: const char* 반환)
    const char* getAnomalyMessage(AnomalyType type);
};

#endif // ML_PREDICTOR_H
