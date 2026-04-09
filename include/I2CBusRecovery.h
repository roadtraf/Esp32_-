// ================================================================
// I2CBusRecovery.h - I2C   &   
// v3.9.4 Hardened Edition
// ================================================================
// [   #5]
//
//   :
//   A)      I2C ACK   Wire  
//   B)     SCL     
//   C)  SDA LOW    (read  reset )
//       Wire.begin()  SDA LOW    
//
// :
//   - I2C Bus Recovery (SMBus 2.0 4.3.2 )
//     : SCL 9    state machine 
//     : STOP condition  
//   - Wire.begin() 
//   -   endTransmission
//   -   :    
// ================================================================
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "HardenedConfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ================================================================
// I2C  
// ================================================================
enum I2CError {
    I2C_OK              = 0,
    I2C_ERR_TIMEOUT     = 1,   // 
    I2C_ERR_NACK_ADDR   = 2,   //  NACK
    I2C_ERR_NACK_DATA   = 3,   //  NACK
    I2C_ERR_BUS_BUSY    = 4,   //   
    I2C_ERR_RECOVERY    = 5,   //   
    I2C_ERR_FATAL       = 6,   //  
};

// I2C endTransmission 
// 0=success, 1=data too long, 2=NACK addr, 3=NACK data, 4=other
static const char* i2cErrorStr(uint8_t err) {
    switch (err) {
        case 0: return "OK";
        case 1: return "DATA_TOO_LONG";
        case 2: return "NACK_ADDR";
        case 3: return "NACK_DATA";
        case 4: return "OTHER";
        default: return "UNKNOWN";
    }
}

// ================================================================
// I2C Bus Recovery 
// ================================================================
class I2CBusRecovery {
public:
    static I2CBusRecovery& getInstance() {
        static I2CBusRecovery instance;
        return instance;
    }

    //   
    void begin(uint8_t sdaPin = I2C_SDA_PIN,
               uint8_t sclPin = I2C_SCL_PIN,
               uint32_t freq  = I2C_FREQ_HZ)
    {
        _sdaPin = sdaPin;
        _sclPin = sclPin;
        _freq   = freq;

        _mutex = xSemaphoreCreateMutex();
        if (!_mutex) {
            Serial.println("[I2C]  Mutex  !");
        }

        // Wire 
        Wire.begin(_sdaPin, _sclPin, _freq);
        Wire.setTimeOut(I2C_TIMEOUT_MS);

        _recoveryCount = 0;
        _initialized   = true;

        //    
        if (!isBusHealthy()) {
            Serial.println("[I2C]         ");
            recover();
        } else {
            Serial.println("[I2C]   ");
        }
    }

    //     
    // SDA/SCL  HIGH  (idle )
    bool isBusHealthy() {
        //  GPIO  
        pinMode(_sdaPin, INPUT_PULLUP);
        pinMode(_sclPin, INPUT_PULLUP);
        delayMicroseconds(10);

        bool sdaHigh = digitalRead(_sdaPin);
        bool sclHigh = digitalRead(_sclPin);

        // Wire 
        Wire.begin(_sdaPin, _sclPin, _freq);

        if (!sdaHigh) {
            Serial.printf("[I2C]   SDA LOW  (SCL=%d)\n", sclHigh);
            return false;
        }
        if (!sclHigh) {
            Serial.println("[I2C]   SCL LOW  (   )");
            return false;
        }
        return true;
    }

    //    (SMBus 2.0 4.3.2) 
    //  SDA LOW     
    bool recover() {
        Serial.println("[I2C] ===    ===");
        _recoveryCount++;

        // Step 1: Wire 
        Wire.end();
        delayMicroseconds(100);

        // Step 2: GPIO   
        pinMode(_sdaPin, OUTPUT);
        pinMode(_sclPin, OUTPUT);
        digitalWrite(_sdaPin, HIGH);
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(I2C_RECOVER_DELAY_US * 10);

        // Step 3: SCL 9 
        //  state machine    
        Serial.println("[I2C] SCL 9  ...");
        for (int i = 0; i < I2C_RECOVER_CLOCK_COUNT; i++) {
            // SDA   ( HIGH    )
            pinMode(_sdaPin, INPUT_PULLUP);
            delayMicroseconds(I2C_RECOVER_DELAY_US);

            bool sdaReleased = digitalRead(_sdaPin);

            pinMode(_sclPin, OUTPUT);
            digitalWrite(_sclPin, LOW);
            delayMicroseconds(I2C_RECOVER_DELAY_US);
            digitalWrite(_sclPin, HIGH);
            delayMicroseconds(I2C_RECOVER_DELAY_US);

            if (sdaReleased) {
                Serial.printf("[I2C] SDA   (%d )\n", i + 1);
                break;
            }
        }

        // Step 4: STOP condition  
        // SDA LOW  SCL HIGH  SDA HIGH
        pinMode(_sdaPin, OUTPUT);
        digitalWrite(_sdaPin, LOW);
        delayMicroseconds(I2C_RECOVER_DELAY_US);
        digitalWrite(_sclPin, HIGH);
        delayMicroseconds(I2C_RECOVER_DELAY_US);
        digitalWrite(_sdaPin, HIGH);
        delayMicroseconds(I2C_RECOVER_DELAY_US);

        // Step 5: Wire 
        Wire.begin(_sdaPin, _sclPin, _freq);
        Wire.setTimeOut(I2C_TIMEOUT_MS);
        delay(I2C_SENSOR_WARMUP_MS);  //   

        // Step 6:   
        bool healthy = isBusHealthy();

        if (healthy) {
            Serial.println("[I2C]   ");
        } else {
            Serial.println("[I2C]    -   ");
        }

        return healthy;
    }

    //    ( ) 
    I2CError safeWrite(uint8_t addr, const uint8_t* data, size_t len,
                       bool sendStop = true)
    {
        if (!_initialized) return I2C_ERR_FATAL;

        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
            return I2C_ERR_BUS_BUSY;
        }

        I2CError result = I2C_ERR_FATAL;

        for (int attempt = 0; attempt < I2C_MAX_RETRY; attempt++) {
            Wire.beginTransmission(addr);
            Wire.write(data, len);
            uint8_t err = Wire.endTransmission(sendStop);

            if (err == 0) {
                result = I2C_OK;
                break;
            }

            Serial.printf("[I2C]   addr=0x%02X err=%s ( %d/%d)\n",
                          addr, i2cErrorStr(err), attempt + 1, I2C_MAX_RETRY);

            // NACK     
            if (err == 2 || err == 3 || err == 4) {
                xSemaphoreGive(_mutex);
                delay(I2C_SENSOR_WARMUP_MS);
                recover();
                if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
                    return I2C_ERR_BUS_BUSY;
                }
            } else {
                delay(10);
            }

            result = (err == 2 || err == 3) ? I2C_ERR_NACK_DATA : I2C_ERR_TIMEOUT;
        }

        xSemaphoreGive(_mutex);
        return result;
    }

    //    
    I2CError safeRead(uint8_t addr, uint8_t* buffer, size_t len)
    {
        if (!_initialized) return I2C_ERR_FATAL;

        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
            return I2C_ERR_BUS_BUSY;
        }

        I2CError result = I2C_ERR_FATAL;

        for (int attempt = 0; attempt < I2C_MAX_RETRY; attempt++) {
            size_t received = Wire.requestFrom(addr, len);

            if (received == len) {
                for (size_t i = 0; i < len; i++) {
                    buffer[i] = Wire.read();
                }
                result = I2C_OK;
                break;
            }

            Serial.printf("[I2C]   addr=0x%02X recv=%d ( %d/%d)\n",
                          addr, received, attempt + 1, I2C_MAX_RETRY);

            xSemaphoreGive(_mutex);
            delay(10);
            if (received == 0) recover();
            if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
                return I2C_ERR_BUS_BUSY;
            }

            result = I2C_ERR_TIMEOUT;
        }

        xSemaphoreGive(_mutex);
        return result;
    }

    //    
    void scan() {
        Serial.println("[I2C] ===   ===");
        int found = 0;
        for (uint8_t addr = 8; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("[I2C] : 0x%02X\n", addr);
                found++;
            }
        }
        Serial.printf("[I2C]  %d \n", found);
    }

    //   
    uint32_t getRecoveryCount() const { return _recoveryCount; }

private:
    I2CBusRecovery() : _sdaPin(I2C_SDA_PIN), _sclPin(I2C_SCL_PIN),
                       _freq(I2C_FREQ_HZ), _mutex(nullptr),
                       _initialized(false), _recoveryCount(0) {}

    uint8_t           _sdaPin;
    uint8_t           _sclPin;
    uint32_t          _freq;
    SemaphoreHandle_t _mutex;
    bool              _initialized;
    uint32_t          _recoveryCount;
};

// ================================================================
//  
// ================================================================
#define I2C_BUS     I2CBusRecovery::getInstance()
#define I2C_RECOVER() I2CBusRecovery::getInstance().recover()
#define I2C_HEALTHY() I2CBusRecovery::getInstance().isBusHealthy()
