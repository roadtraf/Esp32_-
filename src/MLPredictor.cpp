// MLPredictor.cpp - MLPredictor.h   
#include "MLPredictor.h"
#include "Config.h"
#include "SensorBuffer.h"

MLPredictor::MLPredictor()
    : sampleCount(0), anomalyThreshold(3.0f),
      lastAnomaly(ANOMALY_NONE), lastAnomalyTime(0) {
    pressureStats    = {};
    temperatureStats = {};
    currentStats     = {};
}

void MLPredictor::begin() {
    sampleCount = 0;
    lastAnomaly = ANOMALY_NONE;
    Serial.println("[MLPredictor]  ");
}

void MLPredictor::addSample(float vacuumPressure, float temperature, float current) {
    if (sampleCount < BUFFER_SIZE) {
        sampleCount++;
    }
    updateStatistics();
}

AnomalyType MLPredictor::detectAnomaly(float vacuumPressure, float temperature, float current) {
    if (sampleCount < 10) return ANOMALY_NONE;
    if (isOutlier(vacuumPressure, pressureStats))    return ANOMALY_PRESSURE;
    if (isOutlier(temperature, temperatureStats))     return ANOMALY_TEMPERATURE;
    if (isOutlier(current, currentStats))             return ANOMALY_CURRENT;
    return ANOMALY_NONE;
}

float MLPredictor::predictNextValue(float* recentValues, int count) {
    if (count < 2) return 0.0f;
    return recentValues[count-1] + (recentValues[count-1] - recentValues[count-2]);
}

void MLPredictor::updateStatistics() {
    float pMean = calculatePressureMean();
    float pStd  = calculatePressureStdDev(pMean);
    pressureStats.mean   = pMean;
    pressureStats.stdDev = pStd;
    calculatePressureMinMax(pressureStats.min, pressureStats.max);

    float tMean = calculateTemperatureMean();
    float tStd  = calculateTemperatureStdDev(tMean);
    temperatureStats.mean   = tMean;
    temperatureStats.stdDev = tStd;

    float cMean = calculateCurrentMean();
    float cStd  = calculateCurrentStdDev(cMean);
    currentStats.mean   = cMean;
    currentStats.stdDev = cStd;
}

bool MLPredictor::isOutlier(float value, MLStatistics stats) {
    if (stats.stdDev < 0.001f) return false;
    return abs(value - stats.mean) > anomalyThreshold * stats.stdDev;
}

float MLPredictor::calculatePressureMean()                   { return pressureBuffer.getMax(); }
float MLPredictor::calculatePressureStdDev(float mean)       { return pressureBuffer.getStdDev(); }
void  MLPredictor::calculatePressureMinMax(float& mn, float& mx) { mn = pressureBuffer.getMin(); mx = pressureBuffer.getMax(); }

float MLPredictor::calculateTemperatureMean()                 { return temperatureBuffer.getMax(); }
float MLPredictor::calculateTemperatureStdDev(float mean)     { return temperatureBuffer.getStdDev(); }
void  MLPredictor::calculateTemperatureMinMax(float& mn, float& mx) { mn = temperatureBuffer.getMin(); mx = temperatureBuffer.getMax(); }

float MLPredictor::calculateCurrentMean()                     { return currentBuffer.getMax(); }
float MLPredictor::calculateCurrentStdDev(float mean)         { return currentBuffer.getStdDev(); }
void  MLPredictor::calculateCurrentMinMax(float& mn, float& mx) { mn = currentBuffer.getMin(); mx = currentBuffer.getMax(); }

const char* MLPredictor::getAnomalyMessage(AnomalyType type) {
    switch (type) {
        case ANOMALY_PRESSURE:    return "  ";
        case ANOMALY_TEMPERATURE: return "  ";
        case ANOMALY_CURRENT:     return "  ";
        case ANOMALY_PATTERN:     return "  ";
        default:                  return "";
    }
}
