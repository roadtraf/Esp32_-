// ================================================================
// SensorManager.h -    (v3.9.2 Phase 3-1 - )
// sensorData  
// ================================================================
#pragma once

#include <Arduino.h>
#include <vector>

// ================================================================
//    (Config.h )
// ================================================================
#ifndef SENSOR_DATA_DEFINED
#define SENSOR_DATA_DEFINED
struct SensorData {
    float    pressure;
    float    current;
    float    temperature;
    bool     limitSwitch;
    bool     photoSensor;
    bool     emergencyStop;
    uint32_t timestamp;
};
#endif

// ================================================================
//   
// ================================================================
class SensorManager {
public:
    // 
    void begin();
    
    //  
    void readAllSensors();
    
    //  
    void updateBuffers();
    void clearBuffers();
    
    // 
    void calibratePressure();
    void calibrateCurrent();
    
    //   
    bool checkSensorHealth();
    
    // 
    float getPressureAverage(uint8_t samples = 10);
    float getTemperatureAverage(uint8_t samples = 10);
    float getCurrentAverage(uint8_t samples = 10);
    
    //  
    const std::vector<float>& getPressureBuffer() const { return pressureBuffer; }
    const std::vector<float>& getTemperatureBuffer() const { return temperatureBuffer; }
    const std::vector<float>& getCurrentBuffer() const { return currentBuffer; }
    
    // ================================================================
    //    ()
    // ================================================================
    
    //   
    const SensorData& getData() const { return sensorData; }
    SensorData& getDataRef() { return sensorData; }
    
    //   
    float getPressure() const { return sensorData.pressure; }
    float getCurrent() const { return sensorData.current; }
    float getTemperature() const { return sensorData.temperature; }
    bool getLimitSwitch() const { return sensorData.limitSwitch; }
    bool getPhotoSensor() const { return sensorData.photoSensor; }
    bool getEmergencyStop() const { return sensorData.emergencyStop; }
    uint32_t getTimestamp() const { return sensorData.timestamp; }
    
    //    ()
    void setPressure(float value) { sensorData.pressure = value; }
    void setCurrent(float value) { sensorData.current = value; }
    void setTemperature(float value) { sensorData.temperature = value; }
    
    //  
    void printStatus() const;
    
private:
    //   (!)
    SensorData sensorData;
    
    // 
    std::vector<float> pressureBuffer;
    std::vector<float> temperatureBuffer;
    std::vector<float> currentBuffer;
    
    //  
    float pressureOffset = 0.0f;
    float currentOffset = 0.0f;
    
    //   
    float calculateAverage(const std::vector<float>& buffer, uint8_t samples);
    void addToBuffer(std::vector<float>& buffer, float value, size_t maxSize = 100);
    
    //    ()
    float readPressureSensor();
    float readCurrentSensor();
    float readTemperatureSensor();
    bool readLimitSwitchSensor();
    bool readPhotoSensorInput();
    bool readEmergencyStopInput();
};

//  
extern SensorManager sensorManager;
