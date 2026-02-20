// ================================================================
// ControlManager.h - 제어 관리 모듈
// ESP32-S3 v3.9.2 Phase 3-1 - Step 4
// ================================================================
#pragma once

#include <Arduino.h>

// ================================================================
// 제어 관리자 클래스
// ================================================================
class ControlManager {
public:
    // 초기화
    void begin();
    
    // 펌프 제어
    void setPumpState(bool on);
    void setPumpPWM(uint8_t pwm);
    bool isPumpOn() const { return pumpActive; }
    uint8_t getPumpPWM() const { return pumpPWM; }
    
    // 밸브 제어
    void setValveState(bool on);
    bool isValveOn() const { return valveActive; }
    
    // 안전 제어
    void emergencyStop();
    bool isSafeToOperate();
    
    // PID 제어
    void updatePID(float targetPressure, float currentPressure);
    void resetPID();
    void setPIDGains(float kp, float ki, float kd);
    
    // 상태 조회
    float getPIDOutput() const { return pidOutput; }
    void printStatus();
    
private:
    // 펌프 상태
    bool pumpActive;
    uint8_t pumpPWM;
    
    // 밸브 상태
    bool valveActive;
    
    // PID 변수
    float pidError;
    float pidIntegral;
    float pidDerivative;
    float pidLastError;
    float pidOutput;
    
    // PID 게인
    float pidKp;
    float pidKi;
    float pidKd;
    
    uint32_t lastPIDUpdate;
    
    // 내부 메서드
    void writePumpPWM(uint8_t pwm);
    void writeValveState(bool on);
};

extern ControlManager controlManager;
