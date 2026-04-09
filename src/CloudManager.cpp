// CloudManager.cpp - CloudManager.h  
#include "CloudManager.h"
#include "Config.h"

CloudManager::CloudManager() : lastUpdateTime(0), isConnected(false) {}

bool CloudManager::begin() {
    Serial.println("[CloudManager]  ");
    return true;
}

bool CloudManager::uploadExtendedData() { return true; }
bool CloudManager::uploadTrendData()    { return true; }

bool CloudManager::uploadAlertData(MaintenanceLevel level, float healthScore, const char* message) {
    Serial.printf("[CloudManager] Alert: level=%d score=%.1f %s\n", level, healthScore, message);
    return true;
}

bool CloudManager::uploadData(float p, float t, float c, float h, int mode, int err, float uptime, int code) {
    #ifdef ENABLE_THINGSPEAK
    Serial.printf("[CloudManager] Upload: p=%.2f t=%.2f c=%.2f h=%.2f\n", p, t, c, h);
    #endif
    return true;
}

bool CloudManager::shouldUpdate() {
    return (millis() - lastUpdateTime) >= UPDATE_INTERVAL;
}

bool CloudManager::isCloudConnected() { return isConnected; }

void CloudManager::bufferData(float p, float t, float c, float h) {
    dataBuffer.pressure    = p;
    dataBuffer.temperature = t;
    dataBuffer.current     = c;
    dataBuffer.healthScore = h;
    dataBuffer.timestamp   = millis();
}

CloudDataPoint CloudManager::getBufferedData() { return dataBuffer; }

void CloudManager::printStatistics() {
    Serial.println("[CloudManager]  ");
}

void CloudManager::getSystemStatusString(char* buffer, size_t size) {
    snprintf(buffer, size, "Cloud:%s", isConnected ? "OK" : "DISC");
}
