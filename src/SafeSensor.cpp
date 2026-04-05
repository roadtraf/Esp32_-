// ================================================================
// SafeSensor.cpp - DS18B20 비동기 구현
// v3.9.4 Hardened Edition
// ================================================================
#include "SafeSensor.h"
#include "../include/Config.h"

// 전역 인스턴스 (PIN_TEMP_SENSOR = 14, Config.h 참조)
SafeDS18B20 safeDS18B20(PIN_TEMP_SENSOR);

// ================================================================
// FreeRTOS 태스크 함수
// ================================================================
void ds18b20Task(void* param) {
    // WDT에 태스크 등록
    enhancedWatchdog.registerTask("DS18B20", 5000);

    Serial.println("[DS18B20Task] 시작");

    for (;;) {
        safeDS18B20.step();

        WDT_CHECKIN("DS18B20");

        // 100ms 주기 (낮은 우선순위로 다른 태스크에 양보)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
