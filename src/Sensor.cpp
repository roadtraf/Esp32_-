// ================================================================
// Sensor.cpp  —  센서 읽기 구현
// ================================================================

#include "../include/Sensor.h"
#include "../include/Config.h"
#include <Arduino.h>

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ================================================================
// DS18B20 온도 센서 초기화
// ================================================================
OneWire oneWire(PIN_TEMPERATURE_SENSOR);
DallasTemperature tempSensor(&oneWire);

// ================================================================
// 캘리브레이션 값들
// ================================================================
static float pressureOffset = 0.0f;
static float currentOffset = 0.0f;
static float temperatureOffset = 0.0f;

// ================================================================
// 센서 초기화
// ================================================================
void initSensors() {
    Serial.println("[Sensor] 센서 초기화 시작...");
    
    // 아날로그 센서 핀
    pinMode(PIN_PRESSURE_SENSOR, INPUT);
    pinMode(PIN_CURRENT_SENSOR,  INPUT);
    
    // 디지털 센서 핀
    pinMode(PIN_LIMIT_SWITCH,    INPUT_PULLUP);
    pinMode(PIN_PHOTO_SENSOR,    INPUT_PULLUP);
    pinMode(PIN_EMERGENCY_STOP,  INPUT_PULLUP);
    
    // DS18B20 온도 센서 초기화
    tempSensor.begin();
    
    // 온도 센서 감지
    int deviceCount = tempSensor.getDeviceCount();
    Serial.printf("[Sensor] DS18B20 온도 센서: %d개 감지\n", deviceCount);
    
    if (deviceCount > 0) {
        tempSensor.setResolution(12);  // 12비트 해상도 (0.0625°C)
        tempSensor.setWaitForConversion(false);  // 비동기 읽기
        Serial.println("[Sensor] DS18B20 초기화 완료");
    } else {
        Serial.println("[Sensor] 경고: DS18B20 온도 센서를 찾을 수 없습니다!");
        Serial.println("[Sensor] 온도 센서 연결 상태를 확인하세요:");
        Serial.println("[Sensor]   - GPIO 14번에 DATA 연결");
        Serial.println("[Sensor]   - 4.7kΩ 풀업 저항 (DATA-VCC)");
        Serial.println("[Sensor]   - VCC: 3.3V, GND 연결");
    }
    
    Serial.println("[Sensor] 모든 센서 초기화 완료");
}

// ================================================================
// 온도 읽기 (DS18B20)
// ================================================================
float readTemperature() {
    static float lastTemp = 25.0f;  // 기본값
    static uint32_t lastRequestTime = 0;
    static bool conversionRequested = false;
    
    uint32_t now = millis();
    
    // 750ms 변환 시간 고려 (12비트 해상도)
    if (!conversionRequested) {
        tempSensor.requestTemperatures();
        lastRequestTime = now;
        conversionRequested = true;
        return lastTemp;  // 이전 값 반환
    }
    
    if (now - lastRequestTime >= 750) {
        float temp = tempSensor.getTempCByIndex(0);
        
        // 유효성 검사
        if (temp != DEVICE_DISCONNECTED_C && temp > -55.0f && temp < 125.0f) {
            lastTemp = temp + temperatureOffset;
        } else {
            Serial.println("[Sensor] 경고: 온도 센서 읽기 실패 (센서 연결 확인 필요)");
        }
        
        conversionRequested = false;
    }
    
    return lastTemp;
}

// ================================================================
// 압력 읽기 (아날로그)
// ================================================================
float readPressure() {
    int rawValue = analogRead(PIN_PRESSURE_SENSOR);
    
    // ADC 값을 압력(kPa)으로 변환
    // ESP32-S3: 12비트 ADC (0-4095)
    // 예시: 0-3.3V → 0-200 kPa
    float voltage = (rawValue / 4095.0f) * 3.3f;
    float pressure = (voltage / 3.3f) * 200.0f;  // 센서 스펙에 맞게 조정
    
    return pressure + pressureOffset;
}

// ================================================================
// 전류 읽기 (아날로그)
// ================================================================
float readCurrent() {
    int rawValue = analogRead(PIN_CURRENT_SENSOR);
    
    // ADC 값을 전류(A)로 변환
    // 예: ACS712-30A 센서 (2.5V = 0A, 66mV/A)
    float voltage = (rawValue / 4095.0f) * 3.3f;
    float current = (voltage - 1.65f) / 0.066f;
    
    // 음수 값은 절댓값 처리
    if (current < 0) current = -current;
    
    return current + currentOffset;
}

// ================================================================
// 디지털 센서 읽기
// ================================================================
bool readLimitSwitch() {
    return digitalRead(PIN_LIMIT_SWITCH) == LOW;  // LOW = 눌림
}

bool readPhotoSensor() {
    return digitalRead(PIN_PHOTO_SENSOR) == LOW;  // LOW = 감지
}

bool readEmergencyStop() {
    return digitalRead(PIN_EMERGENCY_STOP) == LOW;  // LOW = 눌림
}

// ================================================================
// 모든 센서 읽기
// ================================================================
void readSensors() {
    // 모든 센서 값 읽기
    float temp = readTemperature();
    float press = readPressure();
    float curr = readCurrent();
    bool limit = readLimitSwitch();
    bool photo = readPhotoSensor();
    bool estop = readEmergencyStop();
    
    // 디버그 출력 (필요시 활성화)
    // Serial.printf("T:%.2f P:%.2f C:%.2f L:%d Ph:%d E:%d\n", 
    //               temp, press, curr, limit, photo, estop);
}

// ================================================================
// 캘리브레이션
// ================================================================
void calibratePressure() {
    Serial.println("[Sensor] 압력 센서 캘리브레이션 시작...");
    Serial.println("[Sensor] 진공을 OFF 상태로 유지하세요...");
    
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2초 대기
    
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        int raw = analogRead(PIN_PRESSURE_SENSOR);
        float voltage = (raw / 4095.0f) * 3.3f;
        float pressure = (voltage / 3.3f) * 200.0f;
        sum += pressure;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    pressureOffset = -sum / 10.0f;  // 평균값을 0으로 조정
    Serial.printf("[Sensor] 압력 오프셋: %.2f kPa\n", pressureOffset);
    Serial.println("[Sensor] 압력 캘리브레이션 완료");
}

void calibrateCurrent() {
    Serial.println("[Sensor] 전류 센서 캘리브레이션 시작...");
    Serial.println("[Sensor] 모든 부하를 OFF 상태로 유지하세요...");
    
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2초 대기
    
    float sum = 0;
    for (int i = 0; i < 10; i++) {
        int raw = analogRead(PIN_CURRENT_SENSOR);
        float voltage = (raw / 4095.0f) * 3.3f;
        float current = abs((voltage - 1.65f) / 0.066f);
        sum += current;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    currentOffset = -sum / 10.0f;  // 평균값을 0으로 조정
    Serial.printf("[Sensor] 전류 오프셋: %.2f A\n", currentOffset);
    Serial.println("[Sensor] 전류 캘리브레이션 완료");
}

void calibrateTemperature() {
    Serial.println("[Sensor] 온도 센서 캘리브레이션");
    Serial.println("[Sensor] DS18B20은 공장 캘리브레이션 사용");
    Serial.println("[Sensor] 추가 오프셋이 필요한 경우 수동 설정 가능");
    
    // 필요시 알려진 온도와 비교하여 오프셋 설정
    // temperatureOffset = 실제온도 - 측정온도;
}

// ================================================================
// 센서 건강 체크
// ================================================================
void checkSensorHealth() {
    bool healthy = true;
    
    Serial.println("\n[Sensor] === 센서 건강 체크 ===");
    
    // 온도 센서 체크
    if (!isTemperatureSensorConnected()) {
        Serial.println("[Sensor] ✗ 온도 센서 연결 끊김!");
        healthy = false;
    } else {
        float temp = readTemperature();
        Serial.printf("[Sensor] ✓ 온도 센서: %.2f°C\n", temp);
    }
    
    // 압력 센서 체크
    float pressure = readPressure();
    if (pressure < -50.0f || pressure > 300.0f) {
        Serial.printf("[Sensor] ✗ 압력 센서 이상값: %.2f kPa\n", pressure);
        healthy = false;
    } else {
        Serial.printf("[Sensor] ✓ 압력 센서: %.2f kPa\n", pressure);
    }
    
    // 전류 센서 체크
    float current = readCurrent();
    if (current < 0 || current > 50.0f) {
        Serial.printf("[Sensor] ✗ 전류 센서 이상값: %.2f A\n", current);
        healthy = false;
    } else {
        Serial.printf("[Sensor] ✓ 전류 센서: %.2f A\n", current);
    }
    
    // 디지털 센서들
    Serial.printf("[Sensor] ✓ 리미트 스위치: %s\n", readLimitSwitch() ? "눌림" : "해제");
    Serial.printf("[Sensor] ✓ 포토 센서: %s\n", readPhotoSensor() ? "감지" : "미감지");
    Serial.printf("[Sensor] ✓ 비상정지: %s\n", readEmergencyStop() ? "눌림" : "해제");
    
    if (healthy) {
        Serial.println("[Sensor] === 모든 센서 정상 ===\n");
    } else {
        Serial.println("[Sensor] === 일부 센서 이상 감지 ===\n");
    }
}

bool validateParameters() {
    float temp = readTemperature();
    float press = readPressure();
    float curr = readCurrent();
    
    // 안전 범위 체크
    bool tempOK = (temp > -10.0f && temp < 80.0f);
    bool pressOK = (press >= -10.0f && press < 250.0f);
    bool currOK = (curr >= 0 && curr < 40.0f);
    
    return tempOK && pressOK && currOK;
}

// ================================================================
// 온도 센서 상태
// ================================================================
bool isTemperatureSensorConnected() {
    return tempSensor.getDeviceCount() > 0;
}

int getTemperatureSensorCount() {
    return tempSensor.getDeviceCount();
}