// ================================================================
// Sensor.cpp      
// ================================================================

#include "Sensor.h"
#include "Config.h"
#include <Arduino.h>

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ================================================================
// DS18B20   
// ================================================================
OneWire oneWire(PIN_TEMP_SENSOR);
DallasTemperature tempSensor(&oneWire);

// ================================================================
//  
// ================================================================
static float pressureOffset = 0.0f;
static float currentOffset = 0.0f;
static float temperatureOffset = 0.0f;

// ================================================================
//  
// ================================================================
void initSensors() {
    Serial.println("[Sensor]   ...");
    
    //   
    pinMode(PIN_PRESSURE_SENSOR, INPUT);
    pinMode(PIN_CURRENT_SENSOR,  INPUT);
    
    //   
    pinMode(PIN_LIMIT_SWITCH,    INPUT_PULLUP);
    pinMode(PIN_PHOTO_SENSOR,    INPUT_PULLUP);
    pinMode(PIN_EMERGENCY_STOP,  INPUT_PULLUP);
    
    // DS18B20   
    tempSensor.begin();
    
    //   
    int deviceCount = tempSensor.getDeviceCount();
    Serial.printf("[Sensor] DS18B20  : %d \n", deviceCount);
    
    if (deviceCount > 0) {
        tempSensor.setResolution(12);  // 12  (0.0625C)
        tempSensor.setWaitForConversion(false);  //  
        Serial.println("[Sensor] DS18B20  ");
    } else {
        Serial.println("[Sensor] : DS18B20     !");
        Serial.println("[Sensor]     :");
        Serial.println("[Sensor]   - GPIO 14 DATA ");
        Serial.println("[Sensor]   - 4.7k   (DATA-VCC)");
        Serial.println("[Sensor]   - VCC: 3.3V, GND ");
    }
    
    Serial.println("[Sensor]    ");
}

// ================================================================
//   (DS18B20)
// ================================================================
float readTemperature() {
    static float lastTemp = 25.0f;  // 
    static uint32_t lastRequestTime = 0;
    static bool conversionRequested = false;
    
    uint32_t now = millis();
    
    // 750ms    (12 )
    if (!conversionRequested) {
        tempSensor.requestTemperatures();
        lastRequestTime = now;
        conversionRequested = true;
        return lastTemp;  //   
    }
    
    if (now - lastRequestTime >= 750) {
        float temp = tempSensor.getTempCByIndex(0);
        
        //  
        if (temp != DEVICE_DISCONNECTED_C && temp > -55.0f && temp < 125.0f) {
            lastTemp = temp + temperatureOffset;
        } else {
            Serial.println("[Sensor] :     (   )");
        }
        
        conversionRequested = false;
    }
    
    return lastTemp;
}

// ================================================================
//   ()
// ================================================================
float readPressure() {
    int rawValue = analogRead(PIN_PRESSURE_SENSOR);
    
    // ADC  (kPa) 
    // ESP32-S3: 12 ADC (0-4095)
    // : 0-3.3V  0-200 kPa
    float voltage = (rawValue / 4095.0f) * 3.3f;
    float pressure = (voltage / 3.3f) * 200.0f;  //    
    
    return pressure + pressureOffset;
}

// ================================================================
//   ()
// ================================================================
float readCurrent() {
    int rawValue = analogRead(PIN_CURRENT_SENSOR);
    
    // ADC  (A) 
    // : ACS712-30A  (2.5V = 0A, 66mV/A)
    float voltage = (rawValue / 4095.0f) * 3.3f;
    float current = (voltage - 1.65f) / 0.066f;
    
    //    
    if (current < 0) current = -current;
    
    return current + currentOffset;
}

// ================================================================
//   
// ================================================================
bool readLimitSwitch() {
    return digitalRead(PIN_LIMIT_SWITCH) == LOW;  // LOW = 
}

bool readPhotoSensor() {
    return digitalRead(PIN_PHOTO_SENSOR) == LOW;  // LOW = 
}

bool readEmergencyStop() {
    return digitalRead(PIN_EMERGENCY_STOP) == LOW;  // LOW = 
}

// ================================================================
//   
// ================================================================
void readSensors() {
    //    
    float temp = readTemperature();
    float press = readPressure();
    float curr = readCurrent();
    bool limit = readLimitSwitch();
    bool photo = readPhotoSensor();
    bool estop = readEmergencyStop();
    
    //   ( )
    // Serial.printf("T:%.2f P:%.2f C:%.2f L:%d Ph:%d E:%d\n", 
    //               temp, press, curr, limit, photo, estop);
}

// ================================================================
// 
// ================================================================
void calibratePressure() {
    Serial.println("[Sensor]    ...");
    Serial.println("[Sensor]  OFF  ...");
    
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 
    
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        int raw = analogRead(PIN_PRESSURE_SENSOR);
        float voltage = (raw / 4095.0f) * 3.3f;
        float pressure = (voltage / 3.3f) * 200.0f;
        sum += pressure;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    pressureOffset = -sum / 10.0f;  //  0 
    Serial.printf("[Sensor]  : %.2f kPa\n", pressureOffset);
    Serial.println("[Sensor]   ");
}

void calibrateCurrent() {
    Serial.println("[Sensor]    ...");
    Serial.println("[Sensor]   OFF  ...");
    
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2 
    
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        int raw = analogRead(PIN_CURRENT_SENSOR);
        float voltage = (raw / 4095.0f) * 3.3f;
        float current = abs((voltage - 1.65f) / 0.066f);
        sum += current;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    currentOffset = -sum / 10.0f;  //  0 
    Serial.printf("[Sensor]  : %.2f A\n", currentOffset);
    Serial.println("[Sensor]   ");
}

void calibrateTemperature() {
    Serial.println("[Sensor]   ");
    Serial.println("[Sensor] DS18B20   ");
    Serial.println("[Sensor]       ");
    
    //      
    // temperatureOffset =  - ;
}

// ================================================================
//   
// ================================================================
void checkSensorHealth() {
    bool healthy = true;
    
    Serial.println("\n[Sensor] ===    ===");
    
    //   
    if (!isTemperatureSensorConnected()) {
        Serial.println("[Sensor]     !");
        healthy = false;
    } else {
        float temp = readTemperature();
        Serial.printf("[Sensor]   : %.2fC\n", temp);
    }
    
    //   
    float pressure = readPressure();
    if (pressure < -50.0f || pressure > 300.0f) {
        Serial.printf("[Sensor]    : %.2f kPa\n", pressure);
        healthy = false;
    } else {
        Serial.printf("[Sensor]   : %.2f kPa\n", pressure);
    }
    
    //   
    float current = readCurrent();
    if (current < 0 || current > 50.0f) {
        Serial.printf("[Sensor]    : %.2f A\n", current);
        healthy = false;
    } else {
        Serial.printf("[Sensor]   : %.2f A\n", current);
    }
    
    //  
    Serial.printf("[Sensor]   : %s\n", readLimitSwitch() ? "" : "");
    Serial.printf("[Sensor]   : %s\n", readPhotoSensor() ? "" : "");
    Serial.printf("[Sensor]  : %s\n", readEmergencyStop() ? "" : "");
    
    if (healthy) {
        Serial.println("[Sensor] ===    ===\n");
    } else {
        Serial.println("[Sensor] ===     ===\n");
    }
}

bool validateParameters() {
    float temp = readTemperature();
    float press = readPressure();
    float curr = readCurrent();
    
    //   
    bool tempOK = (temp > -10.0f && temp < 80.0f);
    bool pressOK = (press >= -10.0f && press < 250.0f);
    bool currOK = (curr >= 0 && curr < 40.0f);
    
    return tempOK && pressOK && currOK;
}

// ================================================================
//   
// ================================================================
bool isTemperatureSensorConnected() {
    return tempSensor.getDeviceCount() > 0;
}

int getTemperatureSensorCount() {
    return tempSensor.getDeviceCount();
}