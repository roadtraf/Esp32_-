// ================================================================
// RemoteManager.cpp - MQTT 원격 관리 구현
// ================================================================
#include "../include/RemoteManager.h"
#include "../include/Config.h"
#include "../include/Sensor.h"
#include "../include/SensorBuffer.h"
#include <ArduinoJson.h>

RemoteManager remoteManager;

// ================================================================
// 생성자
// ================================================================
RemoteManager::RemoteManager()
    : mqttClient(nullptr),
      remoteSessionActive(false),
      remoteSessionStart(0),
      remoteSessionTimeout(600000),  // 10분
      remoteLoginAttempts(0),
      remoteLockoutEnd(0)
{
    memset(remoteClientId, 0, sizeof(remoteClientId));
}

// ================================================================
// 초기화
// ================================================================
void RemoteManager::begin(PubSubClient* client) {
    mqttClient = client;
    
    Serial.println("[RemoteManager] 초기화 완료");
    Serial.println("[RemoteManager] 원격 관리 준비됨");
}

// ================================================================
// MQTT 메시지 처리
// ================================================================
void RemoteManager::handleMQTTMessage(const char* topic, const char* payload) {
    if (strcmp(topic, MQTT_TOPIC_REMOTE_CMD) != 0) {
        return;  // 원격 명령 토픽이 아님
    }
    
    Serial.printf("[RemoteManager] 명령 수신: %s\n", payload);
    
    // JSON 파싱
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.println("[RemoteManager] JSON 파싱 실패");
        sendResponse("JSON 파싱 실패", false);
        return;
    }
    
    // 명령 추출
    const char* action = doc["action"] | "";
    const char* password = doc["password"] | "";
    const char* parameter = doc["parameter"] | "";
    const char* clientId = doc["client_id"] | "";
    
    if (strlen(action) == 0) {
        sendResponse("액션 없음", false);
        return;
    }
    
    // 로그인 처리 (특별 처리)
    if (strcmp(action, "login") == 0) {
        handleRemoteLogin(password, clientId);
        return;
    }
    
    // 로그아웃 처리
    if (strcmp(action, "logout") == 0) {
        handleRemoteLogout();
        return;
    }
    
    // 세션 확인 (로그인/로그아웃 외 모든 명령)
    if (!remoteSessionActive) {
        sendResponse("원격 세션 없음. 먼저 로그인하세요.", false);
        return;
    }
    
    // 명령 처리
    processRemoteCommand(action, parameter);
}

// ================================================================
// 원격 로그인 처리
// ================================================================
void RemoteManager::handleRemoteLogin(const char* password, const char* clientId) {
    // 잠금 확인
    if (isRemoteLocked()) {
        uint32_t remaining = (remoteLockoutEnd - millis()) / 1000;
        char msg[64];
        snprintf(msg, sizeof(msg), "계정 잠금: %lu초 남음", remaining);
        sendResponse(msg, false);
        return;
    }
    
    // 비밀번호 확인
    if (!verifyRemotePassword(password)) {
        remoteLoginAttempts++;
        
        if (remoteLoginAttempts >= 3) {
            remoteLockoutEnd = millis() + 60000;  // 1분 잠금
            sendResponse("로그인 실패 3회. 계정 잠김 (60초)", false);
        } else {
            char msg[64];
            snprintf(msg, sizeof(msg), "비밀번호 오류 (%d/3)", remoteLoginAttempts);
            sendResponse(msg, false);
        }
        return;
    }
    
    // 로그인 성공
    resetRemoteAttempts();
    remoteSessionActive = true;
    remoteSessionStart = millis();
    strncpy(remoteClientId, clientId, sizeof(remoteClientId) - 1);
    
    // 로컬 시스템도 관리자 모드로 전환
    systemController.enterManagerMode(password);
    
    Serial.printf("[RemoteManager] 원격 로그인 성공: %s\n", clientId);
    sendResponse("원격 관리자 모드 진입 성공");
    
    // 상태 발행
    publishStatus();
}

// ================================================================
// 원격 로그아웃 처리
// ================================================================
void RemoteManager::handleRemoteLogout() {
    if (!remoteSessionActive) {
        sendResponse("활성 세션 없음", false);
        return;
    }
    
    terminateRemoteSession();
    sendResponse("원격 세션 종료");
}

// ================================================================
// 원격 세션 종료
// ================================================================
void RemoteManager::terminateRemoteSession() {
    remoteSessionActive = false;
    memset(remoteClientId, 0, sizeof(remoteClientId));
    
    // 로컬 시스템도 작업자 모드로
    systemController.enterOperatorMode();
    
    Serial.println("[RemoteManager] 원격 세션 종료");
    publishStatus();
}

// ================================================================
// 명령 처리
// ================================================================
void RemoteManager::processRemoteCommand(const char* action, const char* parameter) {
    if (strcmp(action, "status") == 0) {
        handleRemoteStatus();
    }
    else if (strcmp(action, "sensor_data") == 0) {
        handleRemoteSensorData();
    }
    else if (strcmp(action, "calibrate") == 0) {
        handleRemoteCalibrate(parameter);
    }
    else if (strcmp(action, "setting") == 0) {
        // parameter 형식: "key=value"
        char key[32], value[32];
        if (sscanf(parameter, "%[^=]=%s", key, value) == 2) {
            handleRemoteSettings(key, value);
        } else {
            sendResponse("설정 형식 오류 (key=value 필요)", false);
        }
    }
    else {
        char msg[64];
        snprintf(msg, sizeof(msg), "알 수 없는 명령: %s", action);
        sendResponse(msg, false);
    }
}

// ================================================================
// 상태 조회
// ================================================================
void RemoteManager::handleRemoteStatus() {
    StaticJsonDocument<512> doc;
    
    doc["mode"] = systemController.getModeString();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["mqtt_connected"] = mqttClient->connected();
    
    // 센서 상태
    doc["temp_sensor"] = isTemperatureSensorConnected();
    doc["sensor_count"] = getTemperatureSensorCount();
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_RESPONSE, buffer);
}

// ================================================================
// 센서 데이터 조회
// ================================================================
void RemoteManager::handleRemoteSensorData() {
    StaticJsonDocument<512> doc;
    
    // 실시간 값
    doc["temperature"] = readTemperature();
    doc["pressure"] = readPressure();
    doc["current"] = readCurrent();
    
    // 통계
    SensorStats stats;
    calculateSensorStats(stats);
    
    JsonObject statsObj = doc.createNestedObject("stats");
    statsObj["avg_temp"] = stats.avgTemperature;
    statsObj["avg_pressure"] = stats.avgPressure;
    statsObj["avg_current"] = stats.avgCurrent;
    statsObj["samples"] = stats.sampleCount;
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_RESPONSE, buffer);
}

// ================================================================
// 캘리브레이션
// ================================================================
void RemoteManager::handleRemoteCalibrate(const char* sensorType) {
    if (strcmp(sensorType, "pressure") == 0) {
        calibratePressure();
        sendResponse("압력 센서 캘리브레이션 완료");
    }
    else if (strcmp(sensorType, "current") == 0) {
        calibrateCurrent();
        sendResponse("전류 센서 캘리브레이션 완료");
    }
    else if (strcmp(sensorType, "temperature") == 0) {
        calibrateTemperature();
        sendResponse("온도 센서 캘리브레이션 완료");
    }
    else {
        sendResponse("알 수 없는 센서 타입", false);
    }
}

// ================================================================
// 설정 변경
// ================================================================
void RemoteManager::handleRemoteSettings(const char* key, const char* value) {
    // 예시: target_pressure, pid_kp 등
    char msg[64];
    
    if (strcmp(key, "target_pressure") == 0) {
        float newValue = atof(value);
        // config.targetPressure = newValue;
        // saveConfig();
        snprintf(msg, sizeof(msg), "목표 압력 변경: %.1f kPa", newValue);
        sendResponse(msg);
    }
    else {
        snprintf(msg, sizeof(msg), "알 수 없는 설정: %s", key);
        sendResponse(msg, false);
    }
}

// ================================================================
// 주기적 업데이트
// ================================================================
void RemoteManager::update() {
    if (!remoteSessionActive) {
        return;
    }
    
    // 세션 타임아웃 체크
    uint32_t elapsed = millis() - remoteSessionStart;
    if (elapsed >= remoteSessionTimeout) {
        Serial.println("[RemoteManager] 원격 세션 타임아웃");
        terminateRemoteSession();
        sendResponse("세션 타임아웃 (자동 로그아웃)");
    }
}

// ================================================================
// 상태 발행
// ================================================================
void RemoteManager::publishStatus() {
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    
    doc["remote_session"] = remoteSessionActive;
    doc["mode"] = systemController.getModeString();
    doc["client_id"] = remoteClientId;
    
    if (remoteSessionActive) {
        uint32_t remaining = remoteSessionTimeout - (millis() - remoteSessionStart);
        doc["timeout_remaining"] = remaining / 1000;
    }
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_STATUS, buffer);
}

// ================================================================
// 센서 데이터 발행 (주기적)
// ================================================================
void RemoteManager::publishSensorData() {
    if (!mqttClient || !mqttClient->connected() || !remoteSessionActive) {
        return;
    }
    
    handleRemoteSensorData();  // 재사용
}

// ================================================================
// 비밀번호 검증
// ================================================================
bool RemoteManager::verifyRemotePassword(const char* password) {
    return strcmp(password, MANAGER_PASSWORD) == 0;
}

// ================================================================
// 응답 전송
// ================================================================
void RemoteManager::sendResponse(const char* message, bool success) {
    if (!mqttClient || !mqttClient->connected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["success"] = success;
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    mqttClient->publish(MQTT_TOPIC_REMOTE_RESPONSE, buffer);
}

// ================================================================
// 보안
// ================================================================
bool RemoteManager::isRemoteLocked() const {
    if (remoteLoginAttempts < 3) {
        return false;
    }
    return millis() < remoteLockoutEnd;
}

void RemoteManager::resetRemoteAttempts() {
    remoteLoginAttempts = 0;
    remoteLockoutEnd = 0;
}