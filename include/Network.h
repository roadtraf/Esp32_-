#pragma once
// ================================================================
// Network.h  —  WiFi, MQTT, 시리얼 명령, 절전, 유틸리티
// v4.0 - 완벽한 MQTT 양방향 제어 기능 추가
// ================================================================

// ── WiFi ──
void initWiFi();
void connectWiFi();

// ── MQTT ──
void initMQTT();
void connectMQTT();
void publishMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttLoop();   // loop()에서 매 루프 호출 필요

// ── MQTT 양방향 제어 (신규) ──
void subscribeToTopics();              // 모든 제어 토픽 구독
void handleMQTTCommand(const char* cmd, JsonDocument& doc);  // 명령 처리
void publishSystemStatus();            // 시스템 전체 상태 발행
void publishSensorData();              // 센서 데이터만 발행
void publishAlarmState();              // 알람 상태 발행
void publishConfigUpdate();            // 설정 변경 알림

// ── NTP ──
void initNTP();   // wifiConnected 체크 후 syncTime() 호출

// ── 설정 저장 ──
void saveConfig();
void loadConfig();

// ── Watchdog ──
void initWatchdog();
void feedWatchdog();

// ── 시리얼 명령 ──
void handleSerialCommand();

// ── 절전 ──
void enterSleepMode();
void exitSleepMode();

// ── 유틸리티 ──
void printMemoryInfo();
void printStatistics();
