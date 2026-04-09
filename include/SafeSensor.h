// ================================================================
// SafeSensor.h - DS18B20   + WDT   
// v3.9.4 Hardened Edition
// ================================================================
// [   #9]
//
//  :
//   - requestTemperatures()  12  750ms  OneWire 
//          UI/  
//   -   setWaitForConversion(false) ,
//     requestTemperatures()    ms 
//   - SensorReadTask 100ms    WDT checkin  
//   -     readSensors()  
//
// :
//   -  DS18B20Task ( ) 
//   -  : REQUEST  WAIT_CONVERSION  READ  IDLE
//   -  atomic   (SensorReadTask  )
//   -     fallback 
//   -      
// ================================================================
#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "HardenedConfig.h"
#include "EnhancedWatchdog.h"

// ================================================================
// DS18B20  
// ================================================================
enum DS18B20State {
    DS18B20_IDLE,               // 
    DS18B20_REQUESTING,         //   
    DS18B20_WAIT_CONVERSION,    //    (750ms)
    DS18B20_READING,            //   
    DS18B20_ERROR,              //  
};

// ================================================================
//    
// ================================================================
class SafeDS18B20 {
public:
    SafeDS18B20(uint8_t pin)
        : _ow(pin), _sensor(&_ow),
          _state(DS18B20_IDLE),
          _lastTemp(DS18B20_FALLBACK_TEMP),
          _tempValid(false),
          _sensorPresent(false),
          _requestTime(0),
          _errorCount(0),
          _readCount(0)
    {}

    //   
    bool begin() {
        Serial.println("[DS18B20] ...");
        _sensor.begin();

        int count = _sensor.getDeviceCount();
        Serial.printf("[DS18B20]  : %d\n", count);

        if (count == 0) {
            Serial.println("[DS18B20]     - fallback ");
            _sensorPresent = false;
            return false;
        }

        //   (12bit = 750ms, 9bit = 93.75ms)
        // WDT   9bit  :
        // 9bit: 0.5C   93.75ms  (WDT )
        // 12bit: 0.0625C  750ms (  )
        _sensor.setResolution(12);
        _sensor.setWaitForConversion(false);  //  !

        _sensorPresent = true;
        _state = DS18B20_IDLE;

        Serial.println("[DS18B20]   ");
        return true;
    }

    //    (  ) 
    // FreeRTOS Task : xTaskCreate(ds18b20TaskFunc, ...)
    void runTask() {
        for (;;) {
            step();
            //    -   CPU 
            vTaskDelay(pdMS_TO_TICKS(100));

            WDT_CHECKIN("DS18B20");
        }
    }

    //     
    void step() {
        if (!_sensorPresent) {
            //  :   (30)
            static uint32_t lastSearchTime = 0;
            if (millis() - lastSearchTime > 30000) {
                lastSearchTime = millis();
                if (_sensor.getDeviceCount() > 0) {
                    Serial.println("[DS18B20]  !");
                    begin();
                }
            }
            return;
        }

        uint32_t now = millis();

        switch (_state) {
            case DS18B20_IDLE:
                //    (1)
                static uint32_t lastRequest = 0;
                if (now - lastRequest >= 1000) {
                    lastRequest = now;
                    _requestConversion();
                }
                break;

            case DS18B20_WAIT_CONVERSION:
                //   
                if (now - _requestTime >= DS18B20_CONVERSION_TIME_MS) {
                    _readTemperature();
                }
                break;

            case DS18B20_ERROR:
                // : 2  
                if (now - _requestTime >= 2000) {
                    Serial.printf("[DS18B20]    ( %lu)\n", _errorCount);
                    _sensor.begin();  // 
                    _state = DS18B20_IDLE;
                }
                break;

            default:
                _state = DS18B20_IDLE;
                break;
        }
    }

    //     (Non-blocking,   ) 
    // SensorReadTask      
    float getTemperature() const {
        return _lastTemp;
    }

    bool isValid()   const { return _tempValid && _sensorPresent; }
    bool isPresent() const { return _sensorPresent; }

    //   
    uint32_t getErrorCount() const { return _errorCount; }
    uint32_t getReadCount()  const { return _readCount; }
    DS18B20State getState()  const { return _state; }

    void printStatus() const {
        Serial.printf("[DS18B20] : %.2fC | : %s | : %lu\n",
                      _lastTemp, _tempValid ? "" : "", _errorCount);
    }

private:
    void _requestConversion() {
        _state = DS18B20_REQUESTING;
        _sensor.requestTemperatures();
        _requestTime = millis();
        _state = DS18B20_WAIT_CONVERSION;
    }

    void _readTemperature() {
        _state = DS18B20_READING;

        // isConversionComplete()  (  )
        float temp = _sensor.getTempCByIndex(0);

        if (temp == DEVICE_DISCONNECTED_C ||
            temp == DEVICE_DISCONNECTED_RAW ||
            temp < -55.0f || temp > 125.0f)
        {
            Serial.printf("[DS18B20]   : %.2fC\n", temp);
            _errorCount++;

            // 3    
            if (_errorCount % 3 == 0) {
                _sensorPresent = (_sensor.getDeviceCount() > 0);
                if (!_sensorPresent) {
                    Serial.println("[DS18B20]    ");
                }
            }
            _state = DS18B20_ERROR;
            return;
        }

        //    ( )
        if (_tempValid && abs(temp - _lastTemp) > 10.0f) {
            Serial.printf("[DS18B20]     : %.2f%.2f\n",
                          _lastTemp, temp);
            _state = DS18B20_IDLE;
            return;
        }

        _lastTemp  = temp;
        _tempValid = true;
        _readCount++;
        _state = DS18B20_IDLE;
    }

    OneWire         _ow;
    DallasTemperature _sensor;
    volatile DS18B20State _state;
    volatile float  _lastTemp;
    volatile bool   _tempValid;
    volatile bool   _sensorPresent;
    uint32_t        _requestTime;
    uint32_t        _errorCount;
    uint32_t        _readCount;
};

// ================================================================
//    ( SafeSensor.cpp)
// ================================================================
extern SafeDS18B20 safeDS18B20;

// ================================================================
// DS18B20  FreeRTOS  
// ================================================================
void ds18b20Task(void* param);
