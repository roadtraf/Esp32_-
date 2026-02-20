// ================================================================
// NetworkManager.h - 네트워크 관리 모듈
// ESP32-S3 v3.9.2 Phase 3-1 - Step 5
// ================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ================================================================
// 네트워크 관리자 클래스
// ================================================================
class NetworkManager {
public:
    // 초기화
    void begin();
    void update();
    
    // WiFi 관리
    bool connectWiFi();
    void disconnectWiFi();
    bool isWiFiConnected();
    int getWiFiRSSI();
    
    // MQTT 관리
    bool connectMQTT();
    void disconnectMQTT();
    bool isMQTTConnected();
    void mqttLoop();
    
    // 데이터 발행
    void publishSensorData();
    void publishSystemStatus();
    void publishCustom(const char* topic, const char* payload);
    
    // 클라우드 업로드
    void uploadToCloud();
    
    // 재연결 관리
    void enableAutoReconnect(bool enable);
    void checkConnections();
    
    // 상태 조회
    void printStatus();
    
private:
    bool wifiConnected;
    bool mqttConnected;
    bool autoReconnect;
    
    uint32_t lastWiFiCheck;
    uint32_t lastMQTTCheck;
    uint32_t lastPublish;
    uint32_t lastCloudUpload;
    
    // WiFi 재연결
    bool attemptWiFiReconnect();
    
    // MQTT 재연결
    bool attemptMQTTReconnect();
    
    // MQTT 콜백
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

extern NetworkManager networkManager;
