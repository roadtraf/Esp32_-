// ================================================================
// Network.cpp    WiFi, MQTT, , Watchdog, , , 
// v4.0 -  MQTT    
// ================================================================
#include "Config.h"
#include "SensorManager.h"
#include "Control.h"
#include "VacuumNetwork.h"
#include "StateMachine.h"  // changeState, getStateName
#include "Sensor.h"        // calibratePressure, calibrateCurrent
#include "SD_Logger.h"     // getCurrentTimeISO8601, generateDailyReport
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
// #include "LovyanGFX_Config.hpp"  // tft (   )
#include "Lang.h"                // setLanguage()
#include "RemoteManager.h"
#include "../include/ConfigManager.h"

// ── forward declarations ──────────────────────────────────────
void connectWiFi();
void connectMQTT();
void subscribeToTopics();
void publishSystemStatus();
void saveConfig();
void printMemoryInfo();
void printStatistics();
void mqttCallback(char* topic, byte* payload, unsigned int length);
// ─

extern SensorManager sensorManager;
extern bool pumpActive;
extern bool valveActive;
extern uint8_t pumpPWM;

//  MQTT  (   ) 
static WiFiClient    wifiClientObj;
static PubSubClient  mqttClientObj(wifiClientObj);

//  MQTT   (v4.0 ) 
#define MQTT_TOPIC_STATUS        "vacuum/status"
#define MQTT_TOPIC_SENSOR        "vacuum/sensor"
#define MQTT_TOPIC_ALARM         "vacuum/alarm"
#define MQTT_TOPIC_COMMAND       "vacuum/command"
#define MQTT_TOPIC_CONFIG        "vacuum/config"
#define MQTT_TOPIC_RESPONSE      "vacuum/response"

//  MQTT   
#define MQTT_RECONNECT_INTERVAL  5000  // 5  
static uint32_t lastMQTTReconnect = 0;

//  WiFi 
void initWiFi() {
  if (strlen(config.wifiSSID) == 0) {
    Serial.println("[WiFi] SSID , ");
    return;
  }
  connectWiFi();
}

void connectWiFi() {
    // WiFiResilience 
    if (wifiResilience.connect()) {
        Serial.println("[WiFi]  ");
    } else {
        Serial.println("[WiFi]  ");
    }
}

//  MQTT 
void initMQTT() {
  if (strlen(config.mqttBroker) == 0) {
    Serial.println("[MQTT]  , ");
    return;
  }

  mqttClientObj.setServer(config.mqttBroker, config.mqttPort);
  mqttClientObj.setCallback(mqttCallback);
  mqttClientObj.setBufferSize(1024);  //   

  connectMQTT();
}

void connectMQTT() {
  if (!wifiConnected) return;
  
  //    
  if (mqttClientObj.connected()) {
    if (!mqttConnected) {
      mqttConnected = true;
      subscribeToTopics();  //    
    }
    return;
  }

  //   
  uint32_t now = millis();
  if (now - lastMQTTReconnect < MQTT_RECONNECT_INTERVAL) return;
  lastMQTTReconnect = now;

  Serial.printf("[MQTT]  : %s:%d\n", config.mqttBroker, config.mqttPort);

//   ID 
char clientId[32];
snprintf(clientId, sizeof(clientId), 
    "VacuumControl-%08X", 
    (uint32_t)ESP.getEfuseMac()
);

// MQTT  (  )
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
    Serial.println("[MQTT]  ");
    
    //    
    subscribeToTopics();
    
    //    
    publishSystemStatus();
    
  } else {
    mqttConnected = false;
    Serial.printf("[MQTT]   (code: %d)\n", mqttClientObj.state());
  }
}

//  v4.0 :     
void subscribeToTopics() {
  if (!mqttClientObj.connected()) return;
  
  //    (QoS 1 -  1  )
  mqttClientObj.subscribe(MQTT_TOPIC_COMMAND, 1);
  Serial.println("[MQTT] : " MQTT_TOPIC_COMMAND);
  
  //    
  mqttClientObj.subscribe(MQTT_TOPIC_CONFIG, 1);
  Serial.println("[MQTT] : " MQTT_TOPIC_CONFIG);
}

//  v4.0 :     
void publishSystemStatus() {
  if (!mqttConnected) return;

  StaticJsonDocument<512> doc;
  
  //  
  char deviceId[24];
  snprintf(deviceId, sizeof(deviceId), "%08x", (uint32_t)ESP.getEfuseMac());
  doc["device_id"] = deviceId;
  doc["timestamp"] = millis();
  
  //  
  doc["state"] = getStateName(currentState);
  doc["mode"] = currentMode == MODE_MANUAL ? "MANUAL" :
                currentMode == MODE_AUTO   ? "AUTO"   : "PID";
  
  //  
  doc["pressure"] = sensorManager.getPressure();
  doc["temperature"] = sensorManager.getTemperature();
  doc["current"] = sensorManager.getCurrent();
  
  // 
  doc["target_pressure"] = config.targetPressure;
  
  //  
  doc["pump_active"] = pumpActive;
  doc["valve_active"] = valveActive;
  doc["pump_pwm"] = pumpPWM;
  
  // 
  doc["total_cycles"] = stats.totalCycles;
  doc["successful_cycles"] = stats.successfulCycles;
  doc["total_errors"] = stats.totalErrors;
  doc["uptime"] = stats.uptime;
  
  // WiFi  
  doc["wifi_rssi"] = WiFi.RSSI();

  char buffer[512];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_STATUS, buffer, true);  // Retained 
}

//  v4.0 :    ( ) 
void publishSensorData() {
  if (!mqttConnected) return;

  StaticJsonDocument<256> doc;
  doc["pressure"] = sensorManager.getPressure();
  doc["temperature"] = sensorManager.getTemperature();
  doc["current"] = sensorManager.getCurrent();
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_SENSOR, buffer);
}

//  v4.0 :    
void publishAlarmState() {
  if (!mqttConnected) return;

  StaticJsonDocument<256> doc;
  doc["error_active"] = errorActive;
  
  if (errorActive) {
    doc["error_code"] = currentError.code;
    doc["error_message"] = currentError.message;
    doc["error_level"] = currentError.severity;
  }

  char buffer[256];
  serializeJson(doc, buffer);
  mqttClientObj.publish(MQTT_TOPIC_ALARM, buffer, true);  // Retained
}

//  v4.0 :    
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

//   publishMQTT() -   
void publishMQTT() {
  publishSystemStatus();  // v4.0   
}

// loop() MQTT       loop() 
void mqttLoop() {
  if (!wifiConnected) return;
  
  //    
  if (!mqttClientObj.connected()) {
    mqttConnected = false;
    connectMQTT();
    return;
  }
  
  mqttClientObj.loop();
}

//  v4.0 : MQTT    
void handleMQTTCommand(const char* cmd, JsonDocument& doc) {
  Serial.printf("[MQTT]  : %s\n", cmd);
  
  //   
  StaticJsonDocument<256> response;
  response["command"] = cmd;
  response["timestamp"] = millis();
  
  bool success = false;
  const char* message = "Unknown command";

  //     
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
    //  
    changeState(STATE_ERROR);
    success = true;
    message = "Emergency stop activated";
  }
  
  //     
  else if (strcmp(cmd, "SET_PRESSURE") == 0) {
    if (doc.containsKey("value")) {
      float value = doc["value"];
      if (value >= -100.0 && value <= 0.0) {  //   
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
  
  //  PID   
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
  
  //    
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
  
  //     
  else if (strcmp(cmd, "PUMP_CONTROL") == 0 && currentMode == MODE_MANUAL) {
    if (doc.containsKey("active")) {
      bool active = doc["active"];
      uint8_t pwm = 255;  // 
      
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
  
  //     
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
  
  //   
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
  
  //    
  else if (strcmp(cmd, "BUZZER") == 0) {
    if (doc.containsKey("enabled")) {
      config.buzzerEnabled = doc["enabled"];
      saveConfig();
      success = true;
      message = config.buzzerEnabled ? "Buzzer enabled" : "Buzzer disabled";
    }
  }

  //  
  response["success"] = success;
  response["message"] = message;
  
  char buffer[256];
  serializeJson(response, buffer);
  mqttClientObj.publish(MQTT_TOPIC_RESPONSE, buffer);
  
  //      
  if (success) {
    publishSystemStatus();
  }
}

//  v4.0 : MQTT   
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  //   
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.printf("[MQTT] : %s -> %s\n", topic, message);

 // Phase 2:    
 if (strncmp(topic, "vacuum/remote/", 14) == 0) {
    remoteManager.handleMQTTMessage(topic, message);
    return;
  }
  
  // JSON 
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.printf("[MQTT] JSON  : %s\n", error.c_str());
    return;
  }
  
  //  
  if (strcmp(topic, MQTT_TOPIC_COMMAND) == 0) {
    //  
    if (doc.containsKey("cmd")) {
      const char* cmd = doc["cmd"];
      handleMQTTCommand(cmd, doc);
    } else {
      Serial.println("[MQTT] 'cmd'  ");
    }
  }
  else if (strcmp(topic, MQTT_TOPIC_CONFIG) == 0) {
    //    (  )
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
      Serial.println("[MQTT]  ");
    }
  }
}

//  NTP 
void initNTP() {
  if (!wifiConnected) {
    Serial.println("[NTP] WiFi , ");
    return;
  }
  syncTime();  // SD_Logger.h
}

//   / 
void saveConfig() {
  if (configManager.saveConfig(&config, sizeof(config), true)) {
    Serial.println("[]   ");
  } else {
    Serial.println("[]   ");
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
  preferences.getString("mqttUser",   config.mqttUser,    sizeof(config.mqttUser));     // v4.0 
  preferences.getString("mqttPass",   config.mqttPassword,sizeof(config.mqttPassword)); // v4.0 

  currentMode = config.controlMode;
  config.language = preferences.getUChar("language", LANG_EN);
  setLanguage((Language)config.language);

  Serial.println("[]  ");
  Serial.printf("   : %.1f kPa\n", config.targetPressure);
  Serial.printf("  PID: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", config.pidKp, config.pidKi, config.pidKd);
  Serial.printf("  : %d\n", currentMode);
} 

//  Watchdog 
void initWatchdog() {
  const esp_task_wdt_config_t wdt_cfg = {
    .timeout_ms     = WDT_TIMEOUT * 1000U,
    .idle_core_mask = (1U << portNUM_PROCESSORS) - 1U,
    .trigger_panic  = true,
};
  esp_task_wdt_init(&wdt_cfg);
  esp_task_wdt_add(NULL);
  Serial.printf("[Watchdog]  (%d)\n", WDT_TIMEOUT);
}

void feedWatchdog() {
  esp_task_wdt_reset();
}

//    
void handleSerialCommand() {
  char cmd[128];
  int cmdLen = Serial.readBytesUntil('\n', cmd, sizeof(cmd) - 1);
  if (cmdLen <= 0) return;
  
  cmd[cmdLen] = '\0';
  
  //     (trim)
  while (cmdLen > 0 && (cmd[cmdLen-1] == ' ' || cmd[cmdLen-1] == '\r' || cmd[cmdLen-1] == '\n')) {
    cmd[--cmdLen] = '\0';
  }
  
  Serial.printf("[] %s\n", cmd);

  //   - strcmp 
  if (strcmp(cmd, "START") == 0) {
    if (currentState == STATE_IDLE) changeState(STATE_VACUUM_ON);
  }
  else if (strcmp(cmd, "STOP") == 0) {
    changeState(STATE_IDLE);
  }
  else if (strcmp(cmd, "STATUS") == 0) {
    Serial.printf(": %s\n", getStateName(currentState));
    Serial.printf(": %.2f kPa\n", sensorManager.getPressure());
    Serial.printf(": %.2f A\n",   sensorManager.getCurrent());
  }
  else if (strncmp(cmd, "SET_PRESSURE ", 13) == 0) {
    float value = atof(cmd + 13);
    config.targetPressure = value;
    saveConfig();
    Serial.printf("  : %.1f kPa\n", value);
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
    
    Serial.printf(" : %s\n", mode);
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
    Serial.printf("[]    : %s\n", cmd);
  }
}

//    
bool sleepMode = false;
static uint8_t savedBacklight = 100;
uint32_t lastIdleTime = 0;

void enterSleepMode() {
  sleepMode        = true;
  savedBacklight   = config.backlightLevel;
  tft.setBrightness(0);
  Serial.println("[]   ");
}

void exitSleepMode() {
  sleepMode = false;
  tft.setBrightness(savedBacklight);
  lastIdleTime = millis();
  Serial.println("[]   ");
}

//   
void printMemoryInfo() {
  Serial.println("\n==========   ==========");
  Serial.printf("  Free Heap:   %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  Total Heap:  %d bytes\n", ESP.getHeapSize());
  Serial.printf("  Free PSRAM:  %d bytes\n", ESP.getFreePsram());
  Serial.printf("  Total PSRAM: %d bytes\n", ESP.getPsramSize());
  Serial.printf("  Flash Size:  %d bytes\n", ESP.getFlashChipSize());
  Serial.println("=================================\n");
}

void printStatistics() {
  Serial.println("\n==========  ==========");
  Serial.printf("   : %lu\n", stats.totalCycles);
  Serial.printf("  : %lu\n",      stats.successfulCycles);
  Serial.printf("  : %lu\n",      stats.failedCycles);
  Serial.printf("   : %lu\n",   stats.totalErrors);
  Serial.printf("   : %lu\n", stats.uptime);
  Serial.printf("   : %.2f ~ %.2f kPa\n", stats.minPressure, stats.maxPressure);
  Serial.println("===========================\n");
}
