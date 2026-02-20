// ================================================================
// SmartAlert.cpp  —  v3.8.2 스마트 알림 시스템
// ================================================================
#include "../include/SmartAlert.h"
#include "../include/Config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <base64.h>

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SensorManager.h"  // SensorManager 통합

// 전역 인스턴스
SmartAlert smartAlert;

// ─────────────────── 생성자 ─────────────────────────────────
// SensorManager 외부 참조
extern SensorManager sensorManager;

SmartAlert::SmartAlert() {
    initialized = false;
    historyCount = 0;
    historyIndex = 0;
    totalAlerts = 0;
    emailsSent = 0;
    smsSent = 0;
    
    memset(lastAlertTime, 0, sizeof(lastAlertTime));
    memset(history, 0, sizeof(history));
    
    // 기본 설정
    config.timeFilterEnabled = true;
    config.startHour = DEFAULT_START_HOUR;
    config.endHour = DEFAULT_END_HOUR;
    config.weekendAlert = false;
    config.urgentAlways = true;
    config.criticalAlways = true;
    config.buzzerEnabled = true;
    config.displayEnabled = true;
    config.emailEnabled = false;
    config.smsEnabled = false;
    config.minAlertInterval = DEFAULT_MIN_ALERT_INTERVAL;
    strcpy(config.smtpServer, "smtp.gmail.com");
    config.smtpPort = 587;
}

// ─────────────────── 초기화 ─────────────────────────────────
void SmartAlert::begin() {
    loadConfig();
    initialized = true;
    Serial.println("[SmartAlert] 초기화 완료");
}

void SmartAlert::loadConfig() {
    // Preferences에서 설정 로드
    preferences.begin("smartalert", true);
    
    config.timeFilterEnabled = preferences.getBool("time_filter", true);
    config.startHour = preferences.getUChar("start_hour", DEFAULT_START_HOUR);
    config.endHour = preferences.getUChar("end_hour", DEFAULT_END_HOUR);
    config.weekendAlert = preferences.getBool("weekend", false);
    config.urgentAlways = preferences.getBool("urgent_always", true);
    config.criticalAlways = preferences.getBool("critical_always", true);
    config.buzzerEnabled = preferences.getBool("buzzer", true);
    config.displayEnabled = preferences.getBool("display", true);
    config.emailEnabled = preferences.getBool("email", false);
    
    preferences.getString("smtp_server", config.smtpServer, sizeof(config.smtpServer));
    config.smtpPort = preferences.getUShort("smtp_port", 587);
    preferences.getString("email_from", config.emailFrom, sizeof(config.emailFrom));
    preferences.getString("email_pwd", config.emailPassword, sizeof(config.emailPassword));
    preferences.getString("email_to", config.emailTo, sizeof(config.emailTo));
    
    preferences.end();
    
    Serial.println("[SmartAlert] 설정 로드 완료");
}

void SmartAlert::saveConfig() {
    preferences.begin("smartalert", false);
    
    preferences.putBool("time_filter", config.timeFilterEnabled);
    preferences.putUChar("start_hour", config.startHour);
    preferences.putUChar("end_hour", config.endHour);
    preferences.putBool("weekend", config.weekendAlert);
    preferences.putBool("urgent_always", config.urgentAlways);
    preferences.putBool("critical_always", config.criticalAlways);
    preferences.putBool("buzzer", config.buzzerEnabled);
    preferences.putBool("display", config.displayEnabled);
    preferences.putBool("email", config.emailEnabled);
    
    preferences.putString("smtp_server", config.smtpServer);
    preferences.putUShort("smtp_port", config.smtpPort);
    preferences.putString("email_from", config.emailFrom);
    preferences.putString("email_pwd", config.emailPassword);
    preferences.putString("email_to", config.emailTo);
    
    preferences.end();
    
    Serial.println("[SmartAlert] 설정 저장 완료");
}

void SmartAlert::setConfig(const AlertConfig& cfg) {
    config = cfg;
    saveConfig();
}

// ─────────────────── 알림 체크 ──────────────────────────────
bool SmartAlert::shouldAlert(MaintenanceLevel level, ErrorCode error) {
    if (!initialized) return false;
    
    // URGENT나 CRITICAL은 항상 알림 (설정에 따라)
    if (level == MAINTENANCE_URGENT && config.urgentAlways) {
        return canSendAlert(level);
    }
    
    if (error != ERROR_NONE && 
        (error == ERROR_EMERGENCY_STOP || error == ERROR_OVERCURRENT || error == ERROR_OVERHEAT) &&
        config.criticalAlways) {
        return true;
    }
    
    // 시간 필터 체크
    if (config.timeFilterEnabled) {
        if (!isWorkingHours()) {
            Serial.println("[SmartAlert] 작업 시간 외 - 알림 억제");
            return false;
        }
        
        if (!config.weekendAlert && isWeekend()) {
            Serial.println("[SmartAlert] 주말 - 알림 억제");
            return false;
        }
    }
    
    return canSendAlert(level);
}

bool SmartAlert::canSendAlert(MaintenanceLevel level) {
    // 최소 간격 체크
    uint32_t now = millis();
    uint8_t idx = (uint8_t)level;
    
    if (idx < 5 && lastAlertTime[idx] > 0) {
        if (now - lastAlertTime[idx] < config.minAlertInterval) {
            Serial.println("[SmartAlert] 최소 간격 미달 - 알림 억제");
            return false;
        }
    }
    
    return true;
}

// ─────────────────── 알림 발송 ──────────────────────────────
void SmartAlert::sendAlert(MaintenanceLevel level, float healthScore, const char* message) {
    if (!initialized) return;
    
    Serial.printf("[SmartAlert] 알림 발송: Level=%d, Health=%.1f%%\n", level, healthScore);
    
    // 부저
    if (config.buzzerEnabled) {
        sendBuzzerAlert(level);
    }
    
    // 화면 팝업
    if (config.displayEnabled) {
        sendDisplayAlert(level, healthScore, message);
    }
    
    // 이메일
    if (config.emailEnabled && strlen(config.emailTo) > 0) {
        char subject[64];
        snprintf(subject, sizeof(subject), "[ESP32] Maintenance Alert - Level %d", level);
        
        char emailBody[1024];
        formatEmailBody(emailBody, sizeof(emailBody), level, healthScore, message);
        
        if (sendEmail(subject, emailBody)) { 
            emailsSent++;
        }
    }
    
    // SMS (선택)
    if (config.smsEnabled && strlen(config.phoneNumber) > 0) {
        char smsBody[160];
        formatSmsMessage(smsBody, sizeof(smsBody), level, healthScore);
        
        if (sendSMS(smsBody)) {  
            smsSent++;
        }
    }
    
    // 통계 업데이트
    totalAlerts++;
    lastAlertTime[(uint8_t)level] = millis();
    
    // 이력 추가
    addToHistory(level, ERROR_NONE, message);
}

void SmartAlert::sendErrorAlert(ErrorCode error, const char* message) {
    if (!initialized) return;
    
    Serial.printf("[SmartAlert] 에러 알림: Code=%d\n", error);
    
    // 부저 (긴급)
    if (config.buzzerEnabled) {
        digitalWrite(PIN_BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(1000));
        digitalWrite(PIN_BUZZER, LOW);
    }
    
    // 이메일
    if (config.emailEnabled && strlen(config.emailTo) > 0) {
        char subject[64];
        snprintf(subject, sizeof(subject), "[ESP32] ERROR - Code %d", error);
        
        // 타임스탬프 생성 (미정의 변수 timeStr 수정)
        time_t errNow = time(nullptr);
        struct tm errTm;
        localtime_r(&errNow, &errTm);
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &errTm);

        char emailBody[512];
        snprintf(emailBody, sizeof(emailBody),
         "Error Detected!\n\n"
         "Error Code: %d\n"
         "Message: %s\n"
         "Time: %s\n"
         "Pressure: %.2f kPa\n"
         "Temperature: %.1f C\n"
         "Current: %.2f A\n",
         (int)error,
         message ? message : "None",
         timeStr,
         sensorManager.getPressure(),
         sensorManager.getTemperature(),
         sensorManager.getCurrent());

        sendEmail(subject, emailBody);
    }
    
    // 이력 추가
    addToHistory(MAINTENANCE_NONE, error, message);
}

// ─────────────────── 개별 알림 채널 ─────────────────────────
void SmartAlert::sendBuzzerAlert(MaintenanceLevel level) {
    // 레벨에 따라 다른 패턴
    switch (level) {
        case MAINTENANCE_REQUIRED:
            // 짧은 삐 2번
            digitalWrite(PIN_BUZZER, HIGH);
            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(PIN_BUZZER, LOW);
            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(PIN_BUZZER, HIGH);
            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(PIN_BUZZER, LOW);
            break;
            
        case MAINTENANCE_URGENT:
            // 긴 삐 3번
            for (int i = 0; i < 3; i++) {
                digitalWrite(PIN_BUZZER, HIGH);
                vTaskDelay(pdMS_TO_TICKS(300));
                digitalWrite(PIN_BUZZER, LOW);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            break;
            
        default:
            break;
    }
}

void SmartAlert::sendDisplayAlert(MaintenanceLevel level,
                                   float healthScore,
                                   const char* message) {
    // 실제 팝업은 UI_Screen_Health.cpp의 showMaintenanceAlert()에서 처리
    // [R3] 현재 보고 있는 화면이 관련 있을 때만 redraw 트리거
    if (currentScreen == SCREEN_HEALTH ||
        currentScreen == SCREEN_MAIN) {
        screenNeedsRedraw = true;
    }
    // 그 외 화면: 화면 전환 시 최신값 자동 반영되므로 즉시 redraw 불필요
}

// ─────────────────── 이메일 전송 ────────────────────────────
bool SmartAlert::sendEmail(const char* subject, const char* body) {
    if (!wifiConnected) {
        Serial.println("[SmartAlert] WiFi 미연결 - 이메일 전송 실패");
        return false;
    }
    
    Serial.printf("[SmartAlert] 이메일 전송: %s\n", subject);
    
    WiFiClientSecure client;
    client.setInsecure();  // SSL 검증 스킵 (간단한 구현)
    
    if (!connectToSMTP(client)) {
        return false;
    }
    
    // SMTP 명령 전송
    if (!sendSMTPCommand(client, "", "220")) return false;
    
    char cmd[256];
    
    // EHLO
    snprintf(cmd, sizeof(cmd), "EHLO ESP32\r\n");
    if (!sendSMTPCommand(client, cmd, "250")) return false;
    
    // AUTH LOGIN
    if (!sendSMTPCommand(client, "AUTH LOGIN\r\n", "334")) return false;
    
    // Username (Base64) - String 없이 char 버퍼 사용
    String _u64 = base64::encode(config.emailFrom);   // base64 라이브러리는 String 반환
    char user64[128];
    snprintf(user64, sizeof(user64), "%s\r\n", _u64.c_str());
    if (!sendSMTPCommand(client, user64, "334")) return false;

    // Password (Base64)
    String _p64 = base64::encode(config.emailPassword);
    char pwd64[128];
    snprintf(pwd64, sizeof(pwd64), "%s\r\n", _p64.c_str());
    if (!sendSMTPCommand(client, pwd64, "235")) return false;
    
    // MAIL FROM
    snprintf(cmd, sizeof(cmd), "MAIL FROM:<%s>\r\n", config.emailFrom);
    if (!sendSMTPCommand(client, cmd, "250")) return false;
    
    // RCPT TO
    snprintf(cmd, sizeof(cmd), "RCPT TO:<%s>\r\n", config.emailTo);
    if (!sendSMTPCommand(client, cmd, "250")) return false;
    
    // DATA
    if (!sendSMTPCommand(client, "DATA\r\n", "354")) return false;
    
    // 헤더
    snprintf(cmd, sizeof(cmd), "From: <%s>\r\n", config.emailFrom);
    client.print(cmd);
    
    snprintf(cmd, sizeof(cmd), "To: <%s>\r\n", config.emailTo);
    client.print(cmd);
    
    snprintf(cmd, sizeof(cmd), "Subject: %s\r\n", subject);
    client.print(cmd);
    
    client.print("Content-Type: text/plain; charset=UTF-8\r\n");
    client.print("\r\n");
    
    // 본문
    client.print(body);
    client.print("\r\n.\r\n");
    
    // 응답 대기
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // QUIT
    client.print("QUIT\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    client.stop();
    
    Serial.println("[SmartAlert] 이메일 전송 완료");
    return true;
}

bool SmartAlert::connectToSMTP(WiFiClientSecure& client) {
    Serial.printf("[SmartAlert] SMTP 연결: %s:%d\n", config.smtpServer, config.smtpPort);
    
    if (!client.connect(config.smtpServer, config.smtpPort)) {
        Serial.println("[SmartAlert] SMTP 연결 실패");
        return false;
    }
    
    Serial.println("[SmartAlert] SMTP 연결 성공");
    return true;
}

bool SmartAlert::sendSMTPCommand(WiFiClientSecure& client, const char* command, const char* expectedResponse) {
    if (strlen(command) > 0) {
        client.print(command);
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));

    char response[128] = {0};
    int rpos = 0;
    while (client.available() && rpos < (int)sizeof(response) - 1) {
        response[rpos++] = (char)client.read();
    }
    response[rpos] = '\0';

    if (strstr(response, expectedResponse) != nullptr) {
        return true;
    }

    Serial.printf("[SmartAlert] SMTP 명령 실패: %s\n", response);
    return false;
}

// ─────────────────── SMS 전송 (선택) ────────────────────────
bool SmartAlert::sendSMS(const char* message) {
    // TODO: SMS API 구현 (Twilio, AWS SNS 등)
    Serial.printf("[SmartAlert] SMS 전송 (미구현): %s\n", message);
    return false;
}

// ─────────────────── 시간 체크 ──────────────────────────────
bool SmartAlert::isWorkingHours() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    uint8_t hour = timeinfo.tm_hour;
    
    return (hour >= config.startHour && hour < config.endHour);
}

bool SmartAlert::isWeekend() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    int wday = timeinfo.tm_wday;  // 0=일요일, 6=토요일
    
    return (wday == 0 || wday == 6);
}

// ─────────────────── 이력 관리 ──────────────────────────────
void SmartAlert::addToHistory(MaintenanceLevel level, ErrorCode error, const char* message) {
    AlertHistory& entry = history[historyIndex];
    
    entry.timestamp = time(nullptr);
    entry.level = level;
    entry.errorCode = error;
    entry.emailSent = config.emailEnabled;
    entry.smsSent = config.smsEnabled;
    
    if (message) {
        strncpy(entry.message, message, sizeof(entry.message) - 1);
    } else {
        entry.message[0] = '\0';
    }
    
    historyIndex = (historyIndex + 1) % MAX_ALERT_HISTORY;
    if (historyCount < MAX_ALERT_HISTORY) {
        historyCount++;
    }
}

AlertHistory* SmartAlert::getHistory(uint8_t& count) {
    count = historyCount;
    return history;
}

void SmartAlert::clearHistory() {
    historyCount = 0;
    historyIndex = 0;
    memset(history, 0, sizeof(history));
}

// ─────────────────── 통계 ───────────────────────────────────
uint32_t SmartAlert::getTotalAlertsSent() {
    return totalAlerts;
}

uint32_t SmartAlert::getEmailsSent() {
    return emailsSent;
}

uint32_t SmartAlert::getSmsSent() {
    return smsSent;
}

uint32_t SmartAlert::getLastAlertTime() {
    uint32_t latest = 0;
    for (int i = 0; i < 5; i++) {
        if (lastAlertTime[i] > latest) {
            latest = lastAlertTime[i];
        }
    }
    return latest;
}

// ─────────────────── 포맷 헬퍼 ──────────────────────────────
void SmartAlert::formatEmailBody(char* buffer, size_t size, 
                                  MaintenanceLevel level, 
                                  float healthScore, 
                                  const char* message) {
    if (!buffer || size == 0) return;
    
    int pos = 0;
    
    // 제목
    pos += snprintf(buffer + pos, size - pos,
        "ESP32 Vacuum Control System Alert\n\n");
    if (pos >= size - 1) return;
    
    // 시간
    time_t now = time(nullptr);
    char timeStr[64];
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    pos += snprintf(buffer + pos, size - pos,
        "Time: %s\n\n", timeStr);
    if (pos >= size - 1) return;
    
    // 유지보수 레벨
    const char* levelStr;
    switch (level) {
        case MAINTENANCE_REQUIRED:
            levelStr = "REQUIRED";
            break;
        case MAINTENANCE_URGENT:
            levelStr = "URGENT";
            break;
        default:
            levelStr = "Unknown";
            break;
    }
    
    pos += snprintf(buffer + pos, size - pos,
        "Maintenance Level: %s\n", levelStr);
    if (pos >= size - 1) return;
    
    // 건강도
    pos += snprintf(buffer + pos, size - pos,
        "Health Score: %.1f%%\n\n", healthScore);
    if (pos >= size - 1) return;
    
    // 메시지
    if (message) {
        pos += snprintf(buffer + pos, size - pos,
            "Message: %s\n\n", message);
        if (pos >= size - 1) return;
    }
    
    // 센서 데이터
    pos += snprintf(buffer + pos, size - pos,
        "Sensor Data:\n"
        "  Pressure: %.2f kPa\n"
        "  Temperature: %.1f °C\n"
        "  Current: %.2f A\n\n",
        sensorManager.getPressure(),
        sensorManager.getTemperature(),
        sensorManager.getCurrent());
    if (pos >= size - 1) return;
    
    // 마무리
    pos += snprintf(buffer + pos, size - pos,
        "Please check the system and perform maintenance if needed.\n");
}

void SmartAlert::formatSmsMessage(char* buffer, size_t size,
                                   MaintenanceLevel level, 
                                   float healthScore) {
    if (!buffer || size == 0) return;
    
    if (level == MAINTENANCE_URGENT) {
        snprintf(buffer, size, 
            "[ESP32] URGENT! Health: %.0f%%. Maintenance needed.", 
            healthScore);
    } else {
        snprintf(buffer, size, 
            "[ESP32] Health: %.0f%%. Maintenance needed.", 
            healthScore);
    }
}
