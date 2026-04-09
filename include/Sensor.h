#pragma once
// ================================================================
// Sensor.h     , ,  
// ================================================================

#include <OneWire.h>
#include <DallasTemperature.h>

//     () 
extern OneWire oneWire;
extern DallasTemperature tempSensor;

//    
void initSensors();

//    
void  readSensors();
float readTemperature();    // DS18B20  
float readPressure();
float readCurrent();
bool  readLimitSwitch();
bool  readPhotoSensor();
bool  readEmergencyStop();

//   
void calibratePressure();
void calibrateCurrent();
void calibrateTemperature();

//    
void checkSensorHealth();
bool validateParameters();

//     
bool isTemperatureSensorConnected();
int  getTemperatureSensorCount();