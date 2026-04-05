pragma once
// ================================================================
// Sensor.h  —  센서 읽기, 캘리브레이션, 건강 체크
// ================================================================

#include <OneWire.h>
#include <DallasTemperature.h>

// ── 온도 센서 객체 (전역) ──
extern OneWire oneWire;
extern DallasTemperature tempSensor;

// ── 센서 초기화 ──
void initSensors();

// ── 센서 읽기 ──
void  readSensors();
float readTemperature();    // DS18B20 온도 센서
float readPressure();
float readCurrent();
bool  readLimitSwitch();
bool  readPhotoSensor();
bool  readEmergencyStop();

// ── 캘리브레이션 ──
void calibratePressure();
void calibrateCurrent();
void calibrateTemperature();

// ── 안전 체크 ──
void checkSensorHealth();
bool validateParameters();

// ── 온도 센서 상태 ──
bool isTemperatureSensorConnected();
int  getTemperatureSensorCount();