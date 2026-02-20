// ================================================================
// SensorManager.h - 센서 관리 모듈 (v3.9.2 Phase 3-1 - 개선)
// sensorData 캡슐화 버전
// ================================================================
#pragma once

#include <Arduino.h>
#include <vector>

// ================================================================
// 센서 데이터 구조체 (Config.h와 동일)
// ================================================================
struct SensorData {
    float    pressure;
    float    current;
    float    temperature;
    bool     limitSwitch;
    bool     photoSensor;
    bool     emergencyStop;
    uint32_t timestamp;
};

// ================================================================
// 센서 관리자 클래스
// ================================================================
class SensorManager {
public:
    // 초기화
    void begin();
    
    // 센서 읽기
    void readAllSensors();
    
    // 버퍼 관리
    void updateBuffers();
    void clearBuffers();
    
    // 캘리브레이션
    void calibratePressure();
    void calibrateCurrent();
    
    // 센서 건강 체크
    bool checkSensorHealth();
    
    // 통계
    float getPressureAverage(uint8_t samples = 10);
    float getTemperatureAverage(uint8_t samples = 10);
    float getCurrentAverage(uint8_t samples = 10);
    
    // 버퍼 접근
    const std::vector<float>& getPressureBuffer() const { return pressureBuffer; }
    const std::vector<float>& getTemperatureBuffer() const { return temperatureBuffer; }
    const std::vector<float>& getCurrentBuffer() const { return currentBuffer; }
    
    // ================================================================
    // 센서 데이터 접근자 (캡슐화)
    // ================================================================
    
    // 전체 데이터 접근
    const SensorData& getData() const { return sensorData; }
    SensorData& getDataRef() { return sensorData; }
    
    // 개별 값 접근
    float getPressure() const { return sensorData.pressure; }
    float getCurrent() const { return sensorData.current; }
    float getTemperature() const { return sensorData.temperature; }
    bool getLimitSwitch() const { return sensorData.limitSwitch; }
    bool getPhotoSensor() const { return sensorData.photoSensor; }
    bool getEmergencyStop() const { return sensorData.emergencyStop; }
    uint32_t getTimestamp() const { return sensorData.timestamp; }
    
    // 개별 값 설정 (필요시)
    void setPressure(float value) { sensorData.pressure = value; }
    void setCurrent(float value) { sensorData.current = value; }
    void setTemperature(float value) { sensorData.temperature = value; }
    
    // 상태 출력
    void printStatus() const;
    
private:
    // 센서 데이터 (캡슐화됨!)
    SensorData sensorData;
    
    // 버퍼
    std::vector<float> pressureBuffer;
    std::vector<float> temperatureBuffer;
    std::vector<float> currentBuffer;
    
    // 캘리브레이션 오프셋
    float pressureOffset = 0.0f;
    float currentOffset = 0.0f;
    
    // 내부 헬퍼 함수
    float calculateAverage(const std::vector<float>& buffer, uint8_t samples);
    void addToBuffer(std::vector<float>& buffer, float value, size_t maxSize = 100);
    
    // 개별 센서 읽기 (내부용)
    float readPressureSensor();
    float readCurrentSensor();
    float readTemperatureSensor();
    bool readLimitSwitchSensor();
    bool readPhotoSensorInput();
    bool readEmergencyStopInput();
};

// 전역 인스턴스
extern SensorManager sensorManager;
