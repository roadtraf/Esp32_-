// ================================================================
// RemoteManager.h - MQTT 원격 관리 시스템
// Phase 2: MQTT를 통한 원격 관리자 모드
// ================================================================
#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include "../include/SystemController.h"

// ================================================================
// MQTT 토픽 정의
// ================================================================
#define MQTT_TOPIC_REMOTE_CMD       "vacuum/remote/command"
#define MQTT_TOPIC_REMOTE_RESPONSE  "vacuum/remote/response"
#define MQTT_TOPIC_REMOTE_STATUS    "vacuum/remote/status"
#define MQTT_TOPIC_REMOTE_LOGIN     "vacuum/remote/login"

// ================================================================
// 원격 명령 구조체
// ================================================================
struct RemoteCommand {
    char action[32];        // login, logout, calibrate, status 등
    char password[32];      // 필요시 비밀번호
    char parameter[64];     // 추가 파라미터
    uint32_t timestamp;     // 명령 시간
};

// ================================================================
// 원격 관리자 클래스
// ================================================================
class RemoteManager {
private:
    PubSubClient* mqttClient;
    
    // 원격 세션 관리
    bool remoteSessionActive;
    uint32_t remoteSessionStart;
    uint32_t remoteSessionTimeout;
    char remoteClientId[32];
    
    // 보안
    uint8_t remoteLoginAttempts;
    uint32_t remoteLockoutEnd;
    
    // 내부 메서드
    bool verifyRemotePassword(const char* password);
    void sendResponse(const char* message, bool success = true);
    void processRemoteCommand(const char* action, const char* parameter);
    
    // 명령 핸들러
    void handleRemoteLogin(const char* password, const char* clientId);
    void handleRemoteLogout();
    void handleRemoteStatus();
    void handleRemoteCalibrate(const char* sensorType);
    void handleRemoteSensorData();
    void handleRemoteSettings(const char* setting, const char* value);
    
public:
    RemoteManager();
    
    // 초기화
    void begin(PubSubClient* client);
    
    // MQTT 콜백 처리
    void handleMQTTMessage(const char* topic, const char* payload);
    
    // 주기적 업데이트
    void update();
    
    // 원격 세션 관리
    bool isRemoteSessionActive() const { return remoteSessionActive; }
    void terminateRemoteSession();
    
    // 상태 발행
    void publishStatus();
    void publishSensorData();
    
    // 보안
    bool isRemoteLocked() const;
    void resetRemoteAttempts();
};

extern RemoteManager remoteManager;