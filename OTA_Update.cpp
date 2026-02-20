// ================================================================
// OTA_Hardened.cpp - OTA 안전 정지 / 스택 모니터 / NTP 보호
// v3.9.4 Hardened Edition
// ================================================================
// [G] OTA: onStart에서 펌프/밸브 강제 OFF
// [E] Stack: 주기적 uxTaskGetStackHighWaterMark 모니터링
// [K] NTP: 미동기화 시 파일명 1970년 방지
// ================================================================

#include "Config.h"
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
// [G] OTA 안전 정지 함수
// OTA 플래시 작업 중 모든 출력 하드웨어 강제 OFF
// ================================================================
static void emergencyHardwareShutdown(const char* reason) {
    Serial.printf("[OTA-SAFE] 하드웨어 강제 정지: %s\n", reason);

    // 펌프 PWM 즉시 0
    ledcWrite(PWM_CHANNEL_PUMP, 0);

    // 모든 출력 OFF
    digitalWrite(PIN_PUMP_PWM,      LOW);
    digitalWrite(PIN_VALVE,         LOW);
    digitalWrite(PIN_12V_MAIN,      LOW);
    digitalWrite(PIN_12V_EMERGENCY, LOW);

    // 알람 출력 OFF
    digitalWrite(PIN_BUZZER,    LOW);
    digitalWrite(PIN_LED_RED,   LOW);
    digitalWrite(PIN_LED_GREEN, LOW);

    Serial.println("[OTA-SAFE] ✅ 모든 출력 OFF 완료");
}

// ================================================================
// OTA 초기화 (하드닝 버전)
// ================================================================
void initOTA() {
    if (!wifiConnected) {
        Serial.println("[OTA] WiFi 미연결, 건너뜀");
        return;
    }

    ArduinoOTA.setHostname("VacuumControl-v394");
    ArduinoOTA.setPassword("admin");  // 주의: 실사용 시 강력한 비밀번호로 변경!

    // [G] OTA 시작 시 하드웨어 안전 정지
    ArduinoOTA.onStart([]() {
        const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "펌웨어" : "파일시스템";
        Serial.printf("\n[OTA] ===== 업데이트 시작: %s =====\n", type);

        // !! 핵심: OTA 전 모든 물리적 출력 정지 !!
        emergencyHardwareShutdown("OTA 업데이트 시작");

        // 상태를 IDLE로 강제 변경 (진공 사이클 중단)
        // VacuumCtrl Task도 멈추므로 직접 변경
        currentState = STATE_IDLE;

        // SD 카드 마운트 해제 (OTA 중 SD 접근 금지)
        SD.end();
        Serial.println("[OTA] SD 마운트 해제 완료");

        // WDT 비활성화 (OTA 중 WDT 트리거 방지)
        enhancedWatchdog.disable();
        esp_task_wdt_delete(NULL);
        Serial.println("[OTA] WDT 비활성화 완료");

        delay(OTA_SAFE_SHUTDOWN_DELAY_MS);
        Serial.println("[OTA] 안전 정지 완료 → 플래시 쓰기 시작");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] ===== 업데이트 완료 =====");
        Serial.println("[OTA] 3초 후 재시작...");
        delay(3000);
        // 재시작 이유 저장
        enhancedWatchdog.forceRestart(RESTART_OTA, "OTA_Update");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint8_t lastPercent = 0;
        uint8_t percent = (progress / (total / 100));
        if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("[OTA] 진행: %u%%\n", percent);
            lastPercent = percent;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        const char* errStr;
        switch(error) {
            case OTA_AUTH_ERROR:    errStr = "인증 실패"; break;
            case OTA_BEGIN_ERROR:   errStr = "시작 실패"; break;
            case OTA_CONNECT_ERROR: errStr = "연결 실패"; break;
            case OTA_RECEIVE_ERROR: errStr = "수신 실패"; break;
            case OTA_END_ERROR:     errStr = "종료 실패"; break;
            default:                errStr = "알 수 없음"; break;
        }
        Serial.printf("[OTA] ❌ 오류: %s\n", errStr);

        // OTA 실패: 하드웨어는 이미 OFF 상태 → 재시작으로 복구
        Serial.println("[OTA] 재시작으로 복구...");
        delay(2000);
        ESP.restart();
    });

    ArduinoOTA.begin();
    Serial.println("[OTA] ✅ ArduinoOTA 활성화 (안전 정지 포함)");
    Serial.println("[OTA] ⚠️  비밀번호를 실사용 전 반드시 변경하세요!");
}

// ================================================================
// [E] 스택 오버플로우 모니터
// 주기적으로 각 태스크의 스택 여유 확인
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
    Serial.println("[Stack] === 스택 여유량 체크 ===");

    for (size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); i++) {
        if (!*(tasks[i].handle)) continue;

        UBaseType_t highWater = uxTaskGetStackHighWaterMark(*(tasks[i].handle));

        const char* status;
        if (highWater < STACK_WARN_WORDS) {
            status = "⚠️ 위험";
            warnFound = true;
        } else if (highWater < STACK_WARN_WORDS * 2) {
            status = "⚡ 주의";
        } else {
            status = "✅ 정상";
        }

        Serial.printf("[Stack] %-14s: %4u words 여유  %s\n",
                      tasks[i].name, highWater, status);
    }

    if (warnFound) {
        Serial.println("[Stack] ⚠️  스택 부족 태스크 발견! 스택 크기 증가 필요");
        Serial.println("[Stack]    AdditionalHardening.h의 STACK_* 상수 조정");
    }
    Serial.println("[Stack] ========================");
}

// ================================================================
// [K] NTP 안전 타임스탬프
// 미동기화 시 1970년 파일명 생성 방지
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
        // NTP 미동기화: uptime 기반 이름
        uint32_t uptimeSec = millis() / 1000;
        snprintf(buf, bufSize, "/reports/%s_%s_%lus.%s",
                 prefix, NTP_FALLBACK_PREFIX, uptimeSec, ext);
        Serial.printf("[NTP] ⚠️  미동기화 파일명 사용: %s\n", buf);
    }
}

// getSafeISO8601: NTP 미동기화 시 BOOT+uptime 형식 반환
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
