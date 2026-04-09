// ================================================================
// OTA_Hardened.cpp - OTA   /   / NTP 
// v3.9.4 Hardened Edition
// ================================================================
// [G] OTA: onStart /  OFF
// [E] Stack:  uxTaskGetStackHighWaterMark 
// [K] NTP:    1970 
// ================================================================

#include "Config.h"
#include "Tasks.h"
extern TaskHandle_t ds18b20TaskHandle;
#include "AdditionalHardening.h"
#include "SharedState.h"
#include "EnhancedWatchdog.h"
#include "SafeSD.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

// ================================================================
// [G] OTA   
// OTA        OFF
// ================================================================
static void emergencyHardwareShutdown(const char* reason) {
    Serial.printf("[OTA-SAFE]   : %s\n", reason);

    //  PWM  0
    ledcWrite(PWM_CHANNEL_PUMP, 0);

    //   OFF
    digitalWrite(PIN_PUMP_PWM,      LOW);
    digitalWrite(PIN_VALVE,         LOW);
    digitalWrite(PIN_12V_MAIN,      LOW);
    digitalWrite(PIN_12V_EMERGENCY, LOW);

    //   OFF
    digitalWrite(PIN_BUZZER,    LOW);
    digitalWrite(PIN_LED_RED,   LOW);
    digitalWrite(PIN_LED_GREEN, LOW);

    Serial.println("[OTA-SAFE]    OFF ");
}

// ================================================================
// OTA  ( )
// ================================================================
void initOTA() {
    if (!wifiConnected) {
        Serial.println("[OTA] WiFi , ");
        return;
    }

    ArduinoOTA.setHostname("VacuumControl-v394");
    ArduinoOTA.setPassword("admin");  // :     !

    // [G] OTA     
    ArduinoOTA.onStart([]() {
        const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "" : "";
        Serial.printf("\n[OTA] =====  : %s =====\n", type);

        // !! : OTA      !!
        emergencyHardwareShutdown("OTA  ");

        //  IDLE   (  )
        // VacuumCtrl Task   
        currentState = STATE_IDLE;

        // SD    (OTA  SD  )
        SD.end();
        Serial.println("[OTA] SD   ");

        // WDT  (OTA  WDT  )
        enhancedWatchdog.disable();
        esp_task_wdt_delete(NULL);
        Serial.println("[OTA] WDT  ");

        delay(OTA_SAFE_SHUTDOWN_DELAY_MS);
        Serial.println("[OTA]       ");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] =====   =====");
        Serial.println("[OTA] 3  ...");
        delay(3000);
        //   
        enhancedWatchdog.forceRestart(RESTART_OTA, "OTA_Update");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint8_t lastPercent = 0;
        uint8_t percent = (progress / (total / 100));
        if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("[OTA] : %u%%\n", percent);
            lastPercent = percent;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        const char* errStr;
        switch(error) {
            case OTA_AUTH_ERROR:    errStr = " "; break;
            case OTA_BEGIN_ERROR:   errStr = " "; break;
            case OTA_CONNECT_ERROR: errStr = " "; break;
            case OTA_RECEIVE_ERROR: errStr = " "; break;
            case OTA_END_ERROR:     errStr = " "; break;
            default:                errStr = "  "; break;
        }
        Serial.printf("[OTA]  : %s\n", errStr);

        // OTA :   OFF    
        Serial.println("[OTA]  ...");
        delay(2000);
        ESP.restart();
    });

    ArduinoOTA.begin();
    Serial.println("[OTA]  ArduinoOTA  (  )");
    Serial.println("[OTA]       !");
}

// ================================================================
// [E]   
//      
// ================================================================
void checkStackWatermarks() {
    static const struct {
        const char*  name;
        TaskHandle_t* handle;
    } tasks[] = {
        { "VacuumCtrl",  &vacuumTaskHandle   },
        { "SensorRead",  &sensorTaskHandle   },
        { "UIUpdate",    &uiTaskHandle       },
        { "WiFiMgr",     &wifiTaskHandle     },
        { "MQTTHandler", &mqttTaskHandle     },
        { "DataLogger",  &loggerTaskHandle   },
        { "HealthMon",   &healthTaskHandle   },
        { "Predictor",   &predictorTaskHandle },
        { "DS18B20",     &ds18b20TaskHandle  },
    };

    bool warnFound = false;
    Serial.println("[Stack] ===    ===");

    for (size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
        if (!*(tasks[i].handle)) continue;

        UBaseType_t highWater = uxTaskGetStackHighWaterMark(*(tasks[i].handle));

        const char* status;
        if (highWater < STACK_WARN_WORDS) {
            status = " ";
            warnFound = true;
        } else if (highWater < STACK_WARN_WORDS * 2) {
            status = " ";
        } else {
            status = " ";
        }

        Serial.printf("[Stack] %-14s: %4u words   %s\n",
                      tasks[i].name, highWater, status);
    }

    if (warnFound) {
        Serial.println("[Stack]      !    ");
        Serial.println("[Stack]    AdditionalHardening.h STACK_*  ");
    }
    Serial.println("[Stack] ========================");
}

// ================================================================
// [K] NTP  
//   1970   
// ================================================================
bool isNTPSynced() {
    time_t now = time(nullptr);
    return (now > (time_t)NTP_VALID_THRESHOLD);
}

void getSafeFilename(char* buf, size_t bufSize, const char* prefix,
                     const char* ext) {
    if (isNTPSynced()) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char datePart[32];
        strftime(datePart, sizeof(datePart), "%Y%m%d_%H%M%S", &timeinfo);
        snprintf(buf, bufSize, "/reports/%s_%s.%s", prefix, datePart, ext);
    } else {
        // NTP : uptime  
        uint32_t uptimeSec = millis() / 1000;
        snprintf(buf, bufSize, "/reports/%s_%s_%lus.%s",
                 prefix, NTP_FALLBACK_PREFIX, uptimeSec, ext);
        Serial.printf("[NTP]     : %s\n", buf);
    }
}

// getSafeISO8601: NTP   BOOT+uptime  
void getSafeISO8601(char* buf, size_t bufSize) {
    if (isNTPSynced()) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        strftime(buf, bufSize, "%Y-%m-%dT%H:%M:%S+09:00", &timeinfo);
    } else {
        uint32_t ms = millis();
        snprintf(buf, bufSize, "BOOT+%02lu:%02lu:%02lu",
                 ms/3600000, (ms%3600000)/60000, (ms%60000)/1000);
    }
}
