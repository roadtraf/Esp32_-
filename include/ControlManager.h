// ================================================================
// ControlManager.h -   
// ESP32-S3 v3.9.2 Phase 3-1 - Step 4
// ================================================================
#pragma once

#include <Arduino.h>

// ================================================================
//   
// ================================================================
class ControlManager {
public:
    // 
    void begin();
    
    //  
    void setPumpState(bool on);
    void setPumpPWM(uint8_t pwm);
    bool isPumpOn() const { return pumpActive; }
    uint8_t getPumpPWM() const { return pumpPWM; }
    
    //  
    void setValveState(bool on);
    bool isValveOn() const { return valveActive; }
    
    //  
    void emergencyStop();
    bool isSafeToOperate();
    
    // PID 
    void updatePID(float targetPressure, float currentPressure);
    void resetPID();
    void setPIDGains(float kp, float ki, float kd);
    
    //  
    float getPIDOutput() const { return pidOutput; }
    void printStatus();
    
private:
    //  
    bool pumpActive;
    uint8_t pumpPWM;
    
    //  
    bool valveActive;
    
    // PID 
    float pidError;
    float pidIntegral;
    float pidDerivative;
    float pidLastError;
    float pidOutput;
    
    // PID 
    float pidKp;
    float pidKi;
    float pidKd;
    
    uint32_t lastPIDUpdate;
    
    //  
    void writePumpPWM(uint8_t pwm);
    void writeValveState(bool on);
};

extern ControlManager controlManager;
