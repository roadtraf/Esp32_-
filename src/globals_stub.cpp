#include <Arduino.h>
#include "Config.h"
#include "SharedState.h"
#include <Preferences.h>
#include "ErrorHandler.h"
#include "HealthMonitor.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

Preferences preferences;
SystemConfig config;
ControlMode currentMode = MODE_MANUAL;
bool errorActive = false;
ErrorInfo currentError;
ErrorInfo errorHistory[10];
uint8_t errorHistCnt = 0;
uint8_t errorHistIdx = 0;
bool keyboardConnected = false;
ScreenType currentScreen = SCREEN_MAIN;
SystemState currentState = STATE_IDLE;
uint32_t g_estopStartMs = 0;
bool mqttConnected = false;
uint8_t helpPageIndex = 0;
WiFiUDP g_ntpUDP;
NTPClient ntpClient(g_ntpUDP);
bool ntpSynced = false;
bool pumpRunning = false;
float pumpDutyCycle = 0.0f;
bool valveState[3] = {false, false, false};
void startVacuumCycle() {}
void stopVacuumCycle() {}
float pidOutput = 0.0f;
float pidError  = 0.0f;
HealthMonitor healthMonitor;
void HealthMonitor::performMaintenance() {}Statistics stats;
