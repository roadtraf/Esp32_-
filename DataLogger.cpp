// ================================================================
// DataLogger.cpp - 데이터 로거 (v3.9.2 수정)
// sensorData → sensorManager 변경
// ================================================================

#include "../include/DataLogger.h"
#include "../include/Config.h"
#include "SensorManager.h"  // ← 추가

extern SensorManager sensorManager;

namespace DataLogger {
    static uint32_t lastLogTime = 0;
    static uint32_t logCount = 0;
    
    void init() {
        Serial.println("[DataLogger] 초기화 완료");
        lastLogTime = 0;
        logCount = 0;
    }
    
    void log() {
        uint32_t currentTime = millis();
        
        // 1초마다 로그
        if (currentTime - lastLogTime < 1000) return;
        
        // SensorManager에서 데이터 가져오기
        const SensorData& data = sensorManager.getData();
        
        Serial.printf("[DataLogger] #%lu: P=%.2f T=%.2f I=%.2f\n",
            logCount++,
            data.pressure,
            data.temperature,
            data.current
        );
        
        lastLogTime = currentTime;
    }
    
    void save() {
        Serial.println("[DataLogger] 데이터 저장");
    }
}
