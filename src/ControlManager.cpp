// ================================================================
// ControlManager.cpp -   
// ESP32-S3 v3.9.2 Phase 3-1 - Step 4
// ================================================================
#include "ControlManager.h"
#include "Config.h"

//  
ControlManager controlManager;

//  
extern SystemConfig config;

// ================================================================
// 
// ================================================================
void ControlManager::begin() {
    Serial.println("[ControlMgr]  ...");
    
    pumpActive = false;
    valveActive = false;
    pumpPWM = 0;
    
    // PID 
    pidError = 0.0f;
    pidIntegral = 0.0f;
    pidDerivative = 0.0f;
    pidLastError = 0.0f;
    pidOutput = 0.0f;
    
    // PID  
    pidKp = config.pidKp;
    pidKi = config.pidKi;
    pidKd = config.pidKd;
    
    lastPIDUpdate = 0;
    
    //   
    setPumpState(false);
    setValveState(false);
    
    Serial.println("[ControlMgr]   ");
}

// ================================================================
//  
// ================================================================
void ControlManager::setPumpState(bool on) {
    pumpActive = on;
    
    if (on) {
        writePumpPWM(pumpPWM);
    } else {
        writePumpPWM(0);
    }
    
    Serial.printf("[ControlMgr] : %s\n", on ? "ON" : "OFF");
}

void ControlManager::setPumpPWM(uint8_t pwm) {
    pumpPWM = constrain(pwm, 0, 255);
    
    if (pumpActive) {
        writePumpPWM(pumpPWM);
    }
}

void ControlManager::writePumpPWM(uint8_t pwm) {
    //   
    ledcWrite(0, pwm);  // PWM  0
}

// ================================================================
//  
// ================================================================
void ControlManager::setValveState(bool on) {
    valveActive = on;
    writeValveState(on);
    
    Serial.printf("[ControlMgr] : %s\n", on ? "ON" : "OFF");
}

void ControlManager::writeValveState(bool on) {
    //   
    digitalWrite(PIN_VALVE, on ? HIGH : LOW);
}

// ================================================================
//  
// ================================================================
void ControlManager::emergencyStop() {
    Serial.println("[ControlMgr]    !");
    
    setPumpState(false);
    setValveState(false);
    resetPID();
}

bool ControlManager::isSafeToOperate() {
    //   
    extern SensorData sensorData;
    
    //   
    if (sensorData.pressure < -120.0f || sensorData.pressure > 20.0f) {
        return false;
    }
    
    //   
    if (sensorData.temperature < -10.0f || sensorData.temperature > 80.0f) {
        return false;
    }
    
    //   
    if (sensorData.current < 0.0f || sensorData.current > 8.0f) {
        return false;
    }
    
    return true;
}

// ================================================================
// PID 
// ================================================================
void ControlManager::updatePID(float targetPressure, float currentPressure) {
    uint32_t now = millis();
    
    //    (50ms)
    if (now - lastPIDUpdate < 50) {
        return;
    }
    
    float dt = (now - lastPIDUpdate) / 1000.0f;  //  
    lastPIDUpdate = now;
    
    //  
    pidError = targetPressure - currentPressure;
    
    // P: 
    float P = pidKp * pidError;
    
    // I: 
    pidIntegral += pidError * dt;
    pidIntegral = constrain(pidIntegral, -100.0f, 100.0f);  //   
    float I = pidKi * pidIntegral;
    
    // D: 
    pidDerivative = (pidError - pidLastError) / dt;
    float D = pidKd * pidDerivative;
    
    // PID  
    pidOutput = P + I + D;
    pidOutput = constrain(pidOutput, 0.0f, 100.0f);  // 0-100%
    
    // PWM  (0-255)
    uint8_t pwm = map(pidOutput, 0, 100, 0, 255);
    setPumpPWM(pwm);
    
    //    
    pidLastError = pidError;
}

void ControlManager::resetPID() {
    pidError = 0.0f;
    pidIntegral = 0.0f;
    pidDerivative = 0.0f;
    pidLastError = 0.0f;
    pidOutput = 0.0f;
    lastPIDUpdate = millis();
    
    Serial.println("[ControlMgr] PID ");
}

void ControlManager::setPIDGains(float kp, float ki, float kd) {
    pidKp = kp;
    pidKi = ki;
    pidKd = kd;
    
    Serial.printf("[ControlMgr] PID : Kp=%.2f, Ki=%.2f, Kd=%.2f\n", kp, ki, kd);
}

// ================================================================
//  
// ================================================================
void ControlManager::printStatus() {
    Serial.println("\n");
    Serial.println("                               ");
    Serial.println("");
    Serial.printf(" : %s (PWM: %d)                    \n", 
                  pumpActive ? " ON" : " OFF", pumpPWM);
    Serial.printf(" : %s                              \n",
                  valveActive ? " ON" : " OFF");
    Serial.println("");
    Serial.printf(" PID : %.1f%%                      \n", pidOutput);
    Serial.printf(" PID : %.2f kPa                   \n", pidError);
    Serial.printf(" PID : %.2f                        \n", pidIntegral);
    Serial.println("");
    Serial.printf("  : %s                         \n",
                  isSafeToOperate() ? " " : "  ");
    Serial.println("\n");
}
