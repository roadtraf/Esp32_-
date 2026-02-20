// ================================================================
// NetworkManager.cpp - 네트워크 관리 구현
// ESP32-S3 v3.9.2 Phase 3-1 - Step 5
// ================================================================
#include "NetworkManager.h"
#include "../include/Config.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 전역 인스턴스
NetworkManager networkManager;

// 외부 참조
extern SystemConfig config;
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

// ================================================================
// 초기화
// ================================================================
void NetworkManager::begin() {
    Serial.println("[NetworkMgr] 초기화 시작...");
    
    wifiConnected = false;
    mqttConnected = false;
    autoReconnect = true;
    
    lastWiFiCheck = 0;
    lastMQTTCheck = 0;
    lastPublish = 0;
    lastCloudUpload = 0;
    
    // MQTT 콜백 설정
    mqttClient.setCallback(mqttCallback);
    
    Serial.println("[NetworkMgr] ✅ 초기화 완료");
}

void NetworkManager::update() {
    if (autoReconnect) {
        checkConnections();
    }
    
    if (mqttConnected) {
        mqttLoop();
    }
}

// ================================================================
// WiFi 관리
// ================================================================
bool NetworkManager::connectWiFi() {
    if (strlen(config.wifiSSID) == 0) {
        Serial.println("[NetworkMgr] WiFi SSID 없음");
        return false;
    }
    
    Serial.printf("[NetworkMgr] WiFi 연결 시도: %s\n", config.wifiSSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifiSSID, config.wifiPassword);
    
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        char ipBuf[16]; WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
        Serial.printf("[NetworkMgr] ✅ WiFi 연결됨: %s\n", ipBuf);
        Serial.printf("[NetworkMgr] RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        wifiConnected = false;
        Serial.println("[NetworkMgr] ❌ WiFi 연결 실패");
        return false;
    }
}

void NetworkManager::disconnectWiFi() {
    WiFi.disconnect();
    wifiConnected = false;
    Serial.println("[NetworkMgr] WiFi 연결 해제");
}

bool NetworkManager::isWiFiConnected() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    return wifiConnected;
}

int NetworkManager::getWiFiRSSI() {
    return WiFi.RSSI();
}

// ================================================================
// MQTT 관리
// ================================================================
bool NetworkManager::connectMQTT() {
    if (!isWiFiConnected()) {
        Serial.println("[NetworkMgr] WiFi 미연결, MQTT 연결 불가");
        return false;
    }
    
    if (strlen(config.mqttServer) == 0) {
        Serial.println("[NetworkMgr] MQTT 서버 설정 없음");
        return false;
    }
    
    mqttClient.setServer(config.mqttServer, config.mqttPort);
    
    Serial.printf("[NetworkMgr] MQTT 연결 시도: %s:%d\n", 
                  config.mqttServer, config.mqttPort);
    
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "ESP32-%08x", (uint32_t)ESP.getEfuseMac());
    
    if (mqttClient.connect(clientId, config.mqttUser, config.mqttPassword)) {
        mqttConnected = true;
        Serial.println("[NetworkMgr] ✅ MQTT 연결됨");
        
        // 토픽 구독
        mqttClient.subscribe("vacuum/control/#");
        
        return true;
    } else {
        mqttConnected = false;
        Serial.printf("[NetworkMgr] ❌ MQTT 연결 실패 (상태: %d)\n", mqttClient.state());
        return false;
    }
}

void NetworkManager::disconnectMQTT() {
    mqttClient.disconnect();
    mqttConnected = false;
    Serial.println("[NetworkMgr] MQTT 연결 해제");
}

bool NetworkManager::isMQTTConnected() {
    mqttConnected = mqttClient.connected();
    return mqttConnected;
}

void NetworkManager::mqttLoop() {
    if (mqttConnected) {
        mqttClient.loop();
    }
}

// ================================================================
// 데이터 발행
// ================================================================
void NetworkManager::publishSensorData() {
    if (!mqttConnected) return;
    
    extern SensorData sensorData;
    
    char payload[200];
    snprintf(payload, sizeof(payload),
             "{\"pressure\":%.1f,\"temperature\":%.1f,\"current\":%.2f}",
             sensorData.pressure, sensorData.temperature, sensorData.current);
    
    mqttClient.publish("vacuum/sensors", payload);
}

void NetworkManager::publishSystemStatus() {
    if (!mqttConnected) return;
    
    extern SystemState currentState;
    extern bool errorActive;
    
    char payload[100];
    snprintf(payload, sizeof(payload),
             "{\"state\":%d,\"error\":%s}",
             (int)currentState, errorActive ? "true" : "false");
    
    mqttClient.publish("vacuum/status", payload);
}

void NetworkManager::publishCustom(const char* topic, const char* payload) {
    if (!mqttConnected) return;
    mqttClient.publish(topic, payload);
}

// ================================================================
// 클라우드 업로드
// ================================================================
void NetworkManager::uploadToCloud() {
    if (!isWiFiConnected()) return;
    
    uint32_t now = millis();
    if (now - lastCloudUpload < 60000) return;  // 1분에 1회
    
    // CloudManager 사용
    #ifdef ENABLE_CLOUD
    extern CloudManager cloudManager;
    extern SensorData sensorData;
    extern HealthMonitor healthMonitor;
    extern SystemState currentState;
    extern bool errorActive;
    extern Statistics stats;
    extern ErrorInfo currentError;
    
    bool success = cloudManager.uploadData(
        sensorData.pressure,
        sensorData.temperature,
        sensorData.current,
        healthMonitor.getHealthScore(),
        (int)currentState,
        errorActive ? 1 : 0,
        stats.uptime / 3600.0f,
        currentError.code
    );
    
    if (success) {
        Serial.println("[NetworkMgr] ✅ 클라우드 업로드 성공");
    }
    #endif
    
    lastCloudUpload = now;
}

// ================================================================
// 재연결 관리
// ================================================================
void NetworkManager::enableAutoReconnect(bool enable) {
    autoReconnect = enable;
    Serial.printf("[NetworkMgr] 자동 재연결: %s\n", enable ? "활성화" : "비활성화");
}

void NetworkManager::checkConnections() {
    uint32_t now = millis();
    
    // WiFi 체크 (5초마다)
    if (now - lastWiFiCheck >= 5000) {
        lastWiFiCheck = now;
        
        if (!isWiFiConnected()) {
            Serial.println("[NetworkMgr] WiFi 끊김 감지, 재연결 시도...");
            attemptWiFiReconnect();
        }
    }
    
    // MQTT 체크 (10초마다)
    if (now - lastMQTTCheck >= 10000) {
        lastMQTTCheck = now;
        
        if (isWiFiConnected() && !isMQTTConnected()) {
            Serial.println("[NetworkMgr] MQTT 끊김 감지, 재연결 시도...");
            attemptMQTTReconnect();
        }
    }
}

bool NetworkManager::attemptWiFiReconnect() {
    return connectWiFi();
}

bool NetworkManager::attemptMQTTReconnect() {
    return connectMQTT();
}

// ================================================================
// MQTT 콜백
// ================================================================
void NetworkManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[NetworkMgr] MQTT 수신: %s\n", topic);
    
    // 메시지 파싱 및 처리
    // (실제 구현은 프로젝트 요구사항에 따라)
}

// ================================================================
// 상태 출력
// ================================================================
void NetworkManager::printStatus() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       네트워크 상태                   ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ WiFi: %s                              ║\n",
                  wifiConnected ? "✅ 연결됨" : "❌ 끊김");
    
    if (wifiConnected) {
        Serial.printf("║ SSID: %-31s ║\n", WiFi.SSID().c_str());
        char ipBuf2[16]; WiFi.localIP().toString().toCharArray(ipBuf2, sizeof(ipBuf2));
        Serial.printf("║ IP: %-33s ║\n", ipBuf2);
        Serial.printf("║ RSSI: %d dBm                          ║\n", WiFi.RSSI());
    }
    
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ MQTT: %s                              ║\n",
                  mqttConnected ? "✅ 연결됨" : "❌ 끊김");
    
    if (mqttConnected) {
        Serial.printf("║ 서버: %-31s ║\n", config.mqttServer);
    }
    
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 자동 재연결: %s                       ║\n",
                  autoReconnect ? "활성화" : "비활성화");
    Serial.println("╚═══════════════════════════════════════╝\n");
}
