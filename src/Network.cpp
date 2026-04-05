// ================================================================
// Network.cpp  —  WiFi, MQTT, 설정, Watchdog, 시리얼, 절전, 유틸리티
// v4.0 - 완벽한 MQTT 양방향 제어 기능 추가
// ================================================================
#include "Config.h"
#include "Network.h"
#include "StateMachine.h"  // changeState, getStateName
#include "Sensor.h"        // calibratePressure, calibrateCurrent
#include "SD_Logger.h"     // getCurrentTimeISO8601, generateDailyReport
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "LovyanGFX_Config.hpp"  // tft (절전 모드 밝기 제어)
#include "Lang.h"                // setLanguage()
#include "RemoteManager.h"

// ── MQTT 클라이언트 (파일 내부 단일 인스턴스) ──
static WiFiClient    wifiClientObj;
static PubSubClient  mqttClientObj(wifiClientObj);

// ── MQTT 토픽 정의 (v4.0 신규) ──
#define MQTT_TOPIC_STATUS        "vacuum/status"
#define MQTT_TOPIC_SENSOR        "vacuum/sensor"
#define MQTT_TOPIC_ALARM         "vacuum/alarm"
#define MQTT_TOPIC_COMMAND       "vacuum/command"
#define MQTT_TOPIC_CONFIG        "vacuum/config"
#define MQTT_TOPIC_RESPONSE      "vacuum/response"

// ── MQTT 재연결 설정 ──
#define MQTT_RECONNECT_INTERVAL  5000  // 5초마다 재연결 시도
static uint32_t lastMQTTReconnect = 0;

// ─────────────────── WiFi ───────────────────────────────────
void initWiFi() {
  if (strlen(config.wifiSSID) == 0) {
    Serial.println("[WiFi] SSID 없음, 건너뜀");
    return;
  }
  connectWiFi();
}

void connectWiFi() {
    // WiFiResilience 사용
    if (wifiResilience.connect()) {
        Serial.println("[WiFi] 연결 성공");
    } else {
        Serial.println("[WiFi] 연결 실패");
    }
}

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n[WiFi] 연결 성공");
    char ipBuf[16];
    WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
    Serial.printf("  IP: %s\n", ipBuf);
  } else {
    wifiConnected = false;
    Serial.println("\n[WiFi] 연결 실패");
  }
}

// ─────────────────── MQTT ───────────────────────────────────
void initMQTT() {
  if (strlen(config.mqttBroker) == 0) {
    Serial.println("[MQTT] 브로커 없음, 건너뜀");
    return;
  }

  mqttClientObj.setServer(config.mqttBroker, config.mqttPort);
  mqttClientObj.setCallback(mqttCallback);
  mqttClientObj.setBufferSize(1024);  // 버퍼 크기 증가

  connectMQTT();
}

void connectMQTT() {
  if (!wifiConnected) return;
  
  // 이미 연결되어 있으면 리턴
  if (mqttClientObj.connected()) {
    if (!mqttConnected) {
      mqttConnected = true;
      subscribeToTopics();  // 재연결 시 구독 복원
    }
    return;
  }

  // 재연결 간격 체크
  uint32_t now = millis();
  if (now - lastMQTTReconnect < MQTT_RECONNECT_INTERVAL) return;
  lastMQTTReconnect = now;

  Serial.printf("[MQTT] 연결 시도: %s:%d\n", config.mqttBroker, config.mqttPort);

// 고유한 클라이언트 ID 생성
char clientId[32];
snprintf(clientId, sizeof(clientId), 
    "VacuumControl-%08X", 
    (uint32_t)ESP.getEfuseMac()
);

// MQTT 연결 (인증정보 있으면 사용)
bool connected = false;
if (strlen(config.mqttUser) > 0 && strlen(config.mqttPassword) > 0) {
  connected = mqttClientObj.connect(clientId, 
                                    config.mqttUser, 
                                    config.mqttPassword);
} else {
  connected = mqttClientObj.connect(clientId);
}

  if (connected) {
    mqttConnected = true;
    Serial.println("[MQTT] 연결 성공");
    
    // 모든 제어 토픽 구독
    subscribeToTopics();
    
    // 연결 성공 메시지 발행
    publishSystemStatus();
    
  } else {
    mqttConnected = false;
    Serial.printf("[MQTT] 연결 실패 (code: %d)\n", mqttClientObj.state());
  }
}

// ── v4.0 신규: 모든 제어 토픽 구독 ──
void subscribeToTopics() {
  if (!mqttClientObj.connected()) return;
  
  // 명령 토픽 구독 (QoS 1 - 최소 1회 전달 보장)
  mqttClientObj.subscribe(MQTT_TOPIC_COMMAND, 1);
  Serial.println("[MQTT] 구독: " MQTT_TOPIC_COMMAND);
  
  // 설정 변경 토픽 구독
  mqttClientObj.subscribe(MQTT_TOPIC_CONFIG, 1);
  Serial.println("[MQTT] 구독: " MQTT_TOPIC_CONFIG);
}

// ── v4.0 개선: 시스템 전체 상태 발행 ──
void publishSystemStatus() {
  if (!mqttConnected) return;

  StaticJsonDocument<512> doc;
  
  // 시스템 정보
  char deviceId[24];
  snprintf(deviceId, sizeof(deviceId), "%08x", (uint32_t)ESP.getEfuseMac());
  doc["device_id"] = deviceId;
  doc["timestamp"] = millis();
  
  // 상태 정보
  doc["state"] = getStateName(currentState);
  doc["mode"] = currentMode == MODE_MANUAL ? "MANUAL" :
                currentMode == MODE_AUTO   ? "AUTO"   : "PID";
  
  // 센서 데이터
  doc["pressure"] = sensorData.pressure;
  doc["temperature"] = sensorData.temperature;
  doc["current"] = sensorData.current;
  
  // 설정값
  doc["target_pressure"] = config.targetPressure;
  
  // 제어 상태
  doc["pump_active"] = pumpActive;
  doc["valve_active"] = valveActive;
  doc["pump_pwm"] = pumpPWM;
  
  // 통계
  doc["total_cycles"] = stats.totalCycles;
  doc["successful_cycles"] = stats.successfulCycles;
  doc["total_errors"] = stats.totalErrors;
  doc["uptime"] = stats.uptime;
  
  // WiFi 신호 강도
  doc["wifi_rssi"] = WiFi.RSSI();

  char buffer[512];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_STATUS, buffer, true);  // Retained 메시지
}

// ── v4.0 신규: 센서 데이터만 발행 (빠른 업데이트용) ──
void publishSensorData() {
  if (!mqttConnected) return;

  StaticJsonDocument<256> doc;
  doc["pressure"] = sensorData.pressure;
  doc["temperature"] = sensorData.temperature;
  doc["current"] = sensorData.current;
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_SENSOR, buffer);
}

// ── v4.0 신규: 알람 상태 발행 ──
void publishAlarmState() {
  if (!mqttConnected) return;

  StaticJsonDocument<256> doc;
  doc["error_active"] = errorActive;
  
  if (errorActive) {
    doc["error_code"] = currentError.code;
    doc["error_message"] = currentError.message;
    doc["error_level"] = currentError.level;
  }

  char buffer[256];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_ALARM, buffer, true);  // Retained
}

// ── v4.0 신규: 설정 변경 알림 ──
void publishConfigUpdate() {
  if (!mqttConnected) return;

  StaticJsonDocument<512> doc;
  doc["target_pressure"] = config.targetPressure;
  doc["pressure_hysteresis"] = config.pressureHysteresis;
  doc["pid_kp"] = config.pidKp;
  doc["pid_ki"] = config.pidKi;
  doc["pid_kd"] = config.pidKd;
  doc["vacuum_on_time"] = config.vacuumOnTime;
  doc["vacuum_hold_time"] = config.vacuumHoldTime;
  doc["vacuum_break_time"] = config.vacuumBreakTime;
  doc["buzzer_enabled"] = config.buzzerEnabled;

  char buffer[512];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_CONFIG, buffer, true);
}

// ── 기존 publishMQTT() - 호환성 유지 ──
void publishMQTT() {
  publishSystemStatus();  // v4.0에서는 전체 상태 발행
}

// loop()에서 MQTT 루프 처리 필요 — 이 함수를 loop()에 호출
void mqttLoop() {
  if (!wifiConnected) return;
  
  // 연결이 끊어졌으면 재연결 시도
  if (!mqttClientObj.connected()) {
    mqttConnected = false;
    connectMQTT();
    return;
  }
  
  mqttClientObj.loop();
}

// ── v4.0 신규: MQTT 명령 처리 함수 ──
void handleMQTTCommand(const char* cmd, JsonDocument& doc) {
  Serial.printf("[MQTT] 명령 처리: %s\n", cmd);
  
  // 응답 메시지 준비
  StaticJsonDocument<256> response;
  response["command"] = cmd;
  response["timestamp"] = millis();
  
  bool success = false;
  const char* message = "Unknown command";

  // ── 시스템 제어 명령 ──
  if (strcmp(cmd, "START") == 0) {
    if (currentState == STATE_IDLE) {
      changeState(STATE_VACUUM_ON);
      success = true;
      message = "Vacuum started";
    } else {
      message = "Cannot start - system not idle";
    }
  }
  else if (strcmp(cmd, "STOP") == 0) {
    changeState(STATE_IDLE);
    success = true;
    message = "System stopped";
  }
  else if (strcmp(cmd, "EMERGENCY_STOP") == 0) {
    // 비상 정지
    changeState(STATE_ERROR);
    success = true;
    message = "Emergency stop activated";
  }
  
  // ── 설정 변경 명령 ──
  else if (strcmp(cmd, "SET_PRESSURE") == 0) {
    if (doc.containsKey("value")) {
      float value = doc["value"];
      if (value >= -100.0 && value <= 0.0) {  // 압력 범위 체크
        config.targetPressure = value;
        saveConfig();
        publishConfigUpdate();
        success = true;
        message = "Target pressure updated";
      } else {
        message = "Invalid pressure value";
      }
    } else {
      message = "Missing 'value' parameter";
    }
  }
  else if (strcmp(cmd, "SET_MODE") == 0) {
    if (doc.containsKey("mode")) {
      const char* mode = doc["mode"];
      if (strcmp(mode, "MANUAL") == 0) {
        currentMode = MODE_MANUAL;
        success = true;
      } else if (strcmp(mode, "AUTO") == 0) {
        currentMode = MODE_AUTO;
        success = true;
      } else if (strcmp(mode, "PID") == 0) {
        currentMode = MODE_PID;
        success = true;
      }
      
      if (success) {
        config.controlMode = currentMode;
        saveConfig();
        message = "Mode changed";
      } else {
        message = "Invalid mode";
      }
    }
  }
  
  // ── PID 파라미터 설정 ──
  else if (strcmp(cmd, "SET_PID") == 0) {
    bool changed = false;
    if (doc.containsKey("kp")) {
      config.pidKp = doc["kp"];
      changed = true;
    }
    if (doc.containsKey("ki")) {
      config.pidKi = doc["ki"];
      changed = true;
    }
    if (doc.containsKey("kd")) {
      config.pidKd = doc["kd"];
      changed = true;
    }
    
    if (changed) {
      saveConfig();
      publishConfigUpdate();
      success = true;
      message = "PID parameters updated";
    } else {
      message = "No PID parameters provided";
    }
  }
  
  // ── 타이밍 설정 ──
  else if (strcmp(cmd, "SET_TIMING") == 0) {
    bool changed = false;
    if (doc.containsKey("vacuum_on_time")) {
      config.vacuumOnTime = doc["vacuum_on_time"];
      changed = true;
    }
    if (doc.containsKey("vacuum_hold_time")) {
      config.vacuumHoldTime = doc["vacuum_hold_time"];
      changed = true;
    }
    if (doc.containsKey("vacuum_break_time")) {
      config.vacuumBreakTime = doc["vacuum_break_time"];
      changed = true;
    }
    
    if (changed) {
      saveConfig();
      publishConfigUpdate();
      success = true;
      message = "Timing parameters updated";
    }
  }
  
  // ── 수동 제어 명령 ──
  else if (strcmp(cmd, "PUMP_CONTROL") == 0 && currentMode == MODE_MANUAL) {
    if (doc.containsKey("active")) {
      bool active = doc["active"];
      uint8_t pwm = 255;  // 기본값
      
      if (doc.containsKey("pwm")) {
        pwm = constrain((int)doc["pwm"], 0, 255);
      }
      
      pumpActive = active;
      pumpPWM = active ? pwm : 0;
      success = true;
      message = active ? "Pump ON" : "Pump OFF";
    }
  }
  else if (strcmp(cmd, "VALVE_CONTROL") == 0 && currentMode == MODE_MANUAL) {
    if (doc.containsKey("active")) {
      valveActive = doc["active"];
      success = true;
      message = valveActive ? "Valve opened" : "Valve closed";
    }
  }
  
  // ── 정보 조회 명령 ──
  else if (strcmp(cmd, "GET_STATUS") == 0) {
    publishSystemStatus();
    success = true;
    message = "Status published";
  }
  else if (strcmp(cmd, "GET_CONFIG") == 0) {
    publishConfigUpdate();
    success = true;
    message = "Config published";
  }
  
  // ── 캘리브레이션 ──
  else if (strcmp(cmd, "CALIBRATE_PRESSURE") == 0) {
    calibratePressure();
    success = true;
    message = "Pressure calibration started";
  }
  else if (strcmp(cmd, "CALIBRATE_CURRENT") == 0) {
    calibrateCurrent();
    success = true;
    message = "Current calibration started";
  }
  
  // ── 부저 제어 ──
  else if (strcmp(cmd, "BUZZER") == 0) {
    if (doc.containsKey("enabled")) {
      config.buzzerEnabled = doc["enabled"];
      saveConfig();
      success = true;
      message = config.buzzerEnabled ? "Buzzer enabled" : "Buzzer disabled";
    }
  }

  // 응답 발행
  response["success"] = success;
  response["message"] = message;
  
  char buffer[256];
  serializeJson(response, buffer);
  mqttClientObj.publish(MQTT_TOPIC_RESPONSE, buffer);
  
  // 상태 변경 시 즉시 상태 발행
  if (success) {
    publishSystemStatus();
  }
}

// ── v4.0 개선: MQTT 콜백 함수 ──
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // 페이로드를 문자열로 변환
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.printf("[MQTT] 수신: %s -> %s\n", topic, message);

 // Phase 2: 원격 관리 명령 처리
 if (strncmp(topic, "vacuum/remote/", 14) == 0) {
    remoteManager.handleMQTTMessage(topic, message);
    return;
  }
  
  // JSON 파싱
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.printf("[MQTT] JSON 파싱 실패: %s\n", error.c_str());
    return;
  }
  
  // 토픽별 처리
  if (strcmp(topic, MQTT_TOPIC_COMMAND) == 0) {
    // 명령 토픽
    if (doc.containsKey("cmd")) {
      const char* cmd = doc["cmd"];
      handleMQTTCommand(cmd, doc);
    } else {
      Serial.println("[MQTT] 'cmd' 필드 없음");
    }
  }
  else if (strcmp(topic, MQTT_TOPIC_CONFIG) == 0) {
    // 설정 변경 토픽 (일괄 설정 변경)
    bool changed = false;
    
    if (doc.containsKey("target_pressure")) {
      config.targetPressure = doc["target_pressure"];
      changed = true;
    }
    if (doc.containsKey("pid_kp")) {
      config.pidKp = doc["pid_kp"];
      changed = true;
    }
    if (doc.containsKey("pid_ki")) {
      config.pidKi = doc["pid_ki"];
      changed = true;
    }
    if (doc.containsKey("pid_kd")) {
      config.pidKd = doc["pid_kd"];
      changed = true;
    }
    
    if (changed) {
      saveConfig();
      publishConfigUpdate();
      Serial.println("[MQTT] 설정이 업데이트되었습니다");
    }
  }
}

// ─────────────────── NTP ────────────────────────────────────
void initNTP() {
  if (!wifiConnected) {
    Serial.println("[NTP] WiFi 미연결, 건너뜀");
    return;
  }
  syncTime();  // SD_Logger.h
}

// ─────────────────── 설정 저장/로드 ─────────────────────────
void saveConfig() {
  if (configManager.saveConfig(&config, sizeof(config), true)) {
    Serial.println("[설정] ✅ 저장 완료");
  } else {
    Serial.println("[설정] ❌ 저장 실패");
  }
}

  void loadConfig() {
  config.targetPressure     = preferences.getFloat("targetPressure", -60.0);
  config.pressureHysteresis = preferences.getFloat("hysteresis",     5.0);
  config.pidKp  = preferences.getFloat("pidKp",  PID_KP);
  config.pidKi  = preferences.getFloat("pidKi",  PID_KI);
  config.pidKd  = preferences.getFloat("pidKd",  PID_KD);
  config.vacuumOnTime        = preferences.getUInt("vacOnTime",    200);
  config.vacuumHoldTime      = preferences.getUInt("vacHoldTime",  5000);
  config.vacuumHoldExtension = preferences.getUInt("vacHoldExt",   2000);
  config.maxHoldExtensions   = preferences.getUChar("maxHoldExt",  3);
  config.vacuumBreakTime     = preferences.getUInt("vacBreakTime", 700);
  config.waitRemovalTime     = preferences.getUInt("waitRemTime",  30000);
  config.tempWarning         = preferences.getFloat("tempWarn",    50.0);
  config.tempCritical        = preferences.getFloat("tempCrit",    60.0);
  config.tempShutdown        = preferences.getFloat("tempShut",    70.0);
  config.controlMode         = (ControlMode)preferences.getUChar("mode", MODE_PID);
  config.buzzerEnabled       = preferences.getBool("buzzer", true);
  config.holdExtensionEnabled= preferences.getBool("holdExtEn", true);
  config.tempSensorEnabled   = preferences.getBool("tempEnable", true);
  config.backlightLevel      = preferences.getUChar("backlight", 255);
  preferences.getString("wifiSSID",   config.wifiSSID,    sizeof(config.wifiSSID));
  preferences.getString("wifiPass",   config.wifiPassword,sizeof(config.wifiPassword));
  preferences.getString("mqttBroker", config.mqttBroker,  sizeof(config.mqttBroker));
  config.mqttPort = preferences.getUShort("mqttPort", 1883);
  preferences.getString("mqttUser",   config.mqttUser,    sizeof(config.mqttUser));     // v4.0 신규
  preferences.getString("mqttPass",   config.mqttPassword,sizeof(config.mqttPassword)); // v4.0 신규

  currentMode = config.controlMode;
  config.language = preferences.getUChar("language", LANG_EN);
  setLanguage((Language)config.language);

  Serial.println("[설정] 로드 완료");
  Serial.printf("  목표 압력: %.1f kPa\n", config.targetPressure);
  Serial.printf("  PID: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", config.pidKp, config.pidKi, config.pidKd);
  Serial.printf("  모드: %d\n", currentMode);
} 

// ─────────────────── Watchdog ───────────────────────────────
void initWatchdog() {
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.printf("[Watchdog] 활성화 (%d초)\n", WDT_TIMEOUT);
}

void feedWatchdog() {
  esp_task_wdt_reset();
}

// ─────────────────── 시리얼 명령 ────────────────────────────
void handleSerialCommand() {
  char cmd[128];
  int cmdLen = Serial.readBytesUntil('\n', cmd, sizeof(cmd) - 1);
  if (cmdLen <= 0) return;
  
  cmd[cmdLen] = '\0';
  
  // 공백 및 개행 제거 (trim)
  while (cmdLen > 0 && (cmd[cmdLen-1] == ' ' || cmd[cmdLen-1] == '\r' || cmd[cmdLen-1] == '\n')) {
    cmd[--cmdLen] = '\0';
  }
  
  Serial.printf("[명령] %s\n", cmd);

  // 명령어 비교 - strcmp 사용
  if (strcmp(cmd, "START") == 0) {
    if (currentState == STATE_IDLE) changeState(STATE_VACUUM_ON);
  }
  else if (strcmp(cmd, "STOP") == 0) {
    changeState(STATE_IDLE);
  }
  else if (strcmp(cmd, "STATUS") == 0) {
    Serial.printf("상태: %s\n", getStateName(currentState));
    Serial.printf("압력: %.2f kPa\n", sensorData.pressure);
    Serial.printf("전류: %.2f A\n",   sensorData.current);
  }
  else if (strncmp(cmd, "SET_PRESSURE ", 13) == 0) {
    float value = atof(cmd + 13);
    config.targetPressure = value;
    saveConfig();
    Serial.printf("목표 압력 변경: %.1f kPa\n", value);
  }
  else if (strncmp(cmd, "SET_MODE ", 9) == 0) {
    const char* mode = cmd + 9;
    
    if (strcmp(mode, "MANUAL") == 0) {
      currentMode = MODE_MANUAL;
    }
    else if (strcmp(mode, "AUTO") == 0) {
      currentMode = MODE_AUTO;
    }
    else if (strcmp(mode, "PID") == 0) {
      currentMode = MODE_PID;
    }
    
    Serial.printf("모드 변경: %s\n", mode);
  }
  else if (strcmp(cmd, "CALIBRATE_PRESSURE") == 0) { 
    calibratePressure(); 
  }
  else if (strcmp(cmd, "CALIBRATE_CURRENT") == 0) { 
    calibrateCurrent(); 
  }
  else if (strcmp(cmd, "PRINT_MEMORY") == 0) { 
    printMemoryInfo(); 
  }
  else if (strcmp(cmd, "PRINT_STATS") == 0) { 
    printStatistics(); 
  }
  else if (strcmp(cmd, "RESET") == 0) { 
    ESP.restart(); 
  }
  else {
    Serial.printf("[에러] 알 수 없는 명령: %s\n", cmd);
  }
}

// ─────────────────── 절전 모드 ──────────────────────────────
void enterSleepMode() {
  sleepMode        = true;
  savedBacklight   = config.backlightLevel;
  tft.setBrightness(0);
  Serial.println("[절전] 슬립 모드 진입");
}

void exitSleepMode() {
  sleepMode = false;
  tft.setBrightness(savedBacklight);
  lastIdleTime = millis();
  Serial.println("[절전] 슬립 모드 해제");
}

// ─────────────────── 유틸리티 ───────────────────────────────
void printMemoryInfo() {
  Serial.println("\n========== 메모리 정보 ==========");
  Serial.printf("  Free Heap:   %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  Total Heap:  %d bytes\n", ESP.getHeapSize());
  Serial.printf("  Free PSRAM:  %d bytes\n", ESP.getFreePsram());
  Serial.printf("  Total PSRAM: %d bytes\n", ESP.getPsramSize());
  Serial.printf("  Flash Size:  %d bytes\n", ESP.getFlashChipSize());
  Serial.println("=================================\n");
}

void printStatistics() {
  Serial.println("\n========== 통계 ==========");
  Serial.printf("  총 사이클: %lu\n", stats.totalCycles);
  Serial.printf("  성공: %lu\n",      stats.successfulCycles);
  Serial.printf("  실패: %lu\n",      stats.failedCycles);
  Serial.printf("  총 에러: %lu\n",   stats.totalErrors);
  Serial.printf("  가동 시간: %lu초\n", stats.uptime);
  Serial.printf("  압력 범위: %.2f ~ %.2f kPa\n", stats.minPressure, stats.maxPressure);
  Serial.println("===========================\n");
}
