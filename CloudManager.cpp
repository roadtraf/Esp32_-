// ================================================================
// CloudManager.cpp - 클라우드 관리 (v3.9.2 수정)
// sensorData → sensorManager 변경
// ================================================================

#include "../include/CloudManager.h"
#include "../include/Config.h"
#include "SensorManager.h"  // ← 추가

extern SensorManager sensorManager;

CloudManager::CloudManager() {
    lastUpdate = 0;
}

void CloudManager::begin() {
    Serial.println("[CloudManager] 초기화 완료");
}

// ================================================================
// update() - SensorData를 const 참조로 받음
// ================================================================
void CloudManager::update(
    const SensorData& sensorData,  // ← const 참조로 받음
    SystemState state,
    const Statistics& stats
) {
    uint32_t currentTime = millis();
    
    // 업데이트 간격 체크 (15초)
    if (currentTime - lastUpdate < 15000) return;
    
    #ifdef ENABLE_THINGSPEAK
    // ThingSpeak 업데이트
    Serial.println("[CloudManager] ThingSpeak 업데이트...");
    Serial.printf("  압력: %.2f kPa\n", sensorData.pressure);
    Serial.printf("  전류: %.2f A\n", sensorData.current);
    Serial.printf("  온도: %.2f C\n", sensorData.temperature);
    #endif
    
    lastUpdate = currentTime;
}

void CloudManager::uploadData(const SensorData& data) {
    Serial.println("[CloudManager] 데이터 업로드");
    Serial.printf("  압력: %.2f\n", data.pressure);
    Serial.printf("  온도: %.2f\n", data.temperature);
}
