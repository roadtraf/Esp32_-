// ================================================================
// SafeSensor.cpp - DS18B20  
// v3.9.4 Hardened Edition
// ================================================================
#include "SafeSensor.h"
#include "Config.h"

//   (PIN_TEMP_SENSOR = 14, Config.h )
SafeDS18B20 safeDS18B20(PIN_TEMP_SENSOR);

// ================================================================
// FreeRTOS  
// ================================================================
void ds18b20Task(void* param) {
    // WDT  
    enhancedWatchdog.registerTask("DS18B20", 5000);

    Serial.println("[DS18B20Task] ");

    for (;;) {
        safeDS18B20.step();

        WDT_CHECKIN("DS18B20");

        // 100ms  (    )
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
