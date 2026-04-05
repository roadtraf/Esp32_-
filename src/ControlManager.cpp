// ================================================================
// ControlManager.cpp - 제어 관리 구현
// ESP32-S3 v3.9.2 Phase 3-1 - Step 4
// ================================================================
#include "ControlManager.h"
#include "../include/Config.h"

// 전역 인스턴스
ControlManager controlManager;

// 외부 참조
extern SystemConfig config;

// ================================================================
// 초기화
// ================================================================
void ControlManager::begin() {
    Serial.println("[ControlMgr] 초기화 시작...");
    
    pumpActive = false;
    valveActive = false;
    pumpPWM = 0;
    
    // PID 초기화
    pidError = 0.0f;
    pidIntegral = 0.0f;
    pidDerivative = 0.0f;
    pidLastError = 0.0f;
    pidOutput = 0.0f;
    
    // PID 게인 로드
    pidKp = config.pidKp;
    pidKi = config.pidKi;
    pidKd = config.pidKd;
    
    lastPIDUpdate = 0;
    
    // 초기 상태 설정
    setPumpState(false);
    setValveState(false);
    
    Serial.println("[ControlMgr] ✅ 초기화 완료");
}

// ================================================================
// 펌프 제어
// ================================================================
void ControlManager::setPumpState(bool on) {
    pumpActive = on;
    
    if (on) {
        writePumpPWM(pumpPWM);
    } else {
        writePumpPWM(0);
    }
    
    Serial.printf("[ControlMgr] 펌프: %s\n", on ? "ON" : "OFF");
}

void ControlManager::setPumpPWM(uint8_t pwm) {
    pumpPWM = constrain(pwm, 0, 255);
    
    if (pumpActive) {
        writePumpPWM(pumpPWM);
    }
}

void ControlManager::writePumpPWM(uint8_t pwm) {
    // 실제 하드웨어 제어
    ledcWrite(0, pwm);  // PWM 채널 0
}

// ================================================================
// 밸브 제어
// ================================================================
void ControlManager::setValveState(bool on) {
    valveActive = on;
    writeValveState(on);
    
    Serial.printf("[ControlMgr] 밸브: %s\n", on ? "ON" : "OFF");
}

void ControlManager::writeValveState(bool on) {
    // 실제 하드웨어 제어
    digitalWrite(VALVE_PIN, on ? HIGH : LOW);
}

// ================================================================
// 안전 제어
// ================================================================
void ControlManager::emergencyStop() {
    Serial.println("[ControlMgr] ⚠️  긴급 정지!");
    
    setPumpState(false);
    setValveState(false);
    resetPID();
}

bool ControlManager::isSafeToOperate() {
    // 센서 건강 체크
    extern SensorData sensorData;
    
    // 압력 범위 체크
    if (sensorData.pressure < -120.0f || sensorData.pressure > 20.0f) {
        return false;
    }
    
    // 온도 범위 체크
    if (sensorData.temperature < -10.0f || sensorData.temperature > 80.0f) {
        return false;
    }
    
    // 전류 범위 체크
    if (sensorData.current < 0.0f || sensorData.current > 8.0f) {
        return false;
    }
    
    return true;
}

// ================================================================
// PID 제어
// ================================================================
void ControlManager::updatePID(float targetPressure, float currentPressure) {
    uint32_t now = millis();
    
    // 최소 업데이트 간격 (50ms)
    if (now - lastPIDUpdate < 50) {
        return;
    }
    
    float dt = (now - lastPIDUpdate) / 1000.0f;  // 초 단위
    lastPIDUpdate = now;
    
    // 오차 계산
    pidError = targetPressure - currentPressure;
    
    // P: 비례항
    float P = pidKp * pidError;
    
    // I: 적분항
    pidIntegral += pidError * dt;
    pidIntegral = constrain(pidIntegral, -100.0f, 100.0f);  // 적분 누적 제한
    float I = pidKi * pidIntegral;
    
    // D: 미분항
    pidDerivative = (pidError - pidLastError) / dt;
    float D = pidKd * pidDerivative;
    
    // PID 출력 계산
    pidOutput = P + I + D;
    pidOutput = constrain(pidOutput, 0.0f, 100.0f);  // 0-100%
    
    // PWM 변환 (0-255)
    uint8_t pwm = map(pidOutput, 0, 100, 0, 255);
    setPumpPWM(pwm);
    
    // 다음 계산을 위해 저장
    pidLastError = pidError;
}

void ControlManager::resetPID() {
    pidError = 0.0f;
    pidIntegral = 0.0f;
    pidDerivative = 0.0f;
    pidLastError = 0.0f;
    pidOutput = 0.0f;
    lastPIDUpdate = millis();
    
    Serial.println("[ControlMgr] PID 리셋");
}

void ControlManager::setPIDGains(float kp, float ki, float kd) {
    pidKp = kp;
    pidKi = ki;
    pidKd = kd;
    
    Serial.printf("[ControlMgr] PID 게인: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", kp, ki, kd);
}

// ================================================================
// 상태 출력
// ================================================================
void ControlManager::printStatus() {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║       제어 상태                       ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 펌프: %s (PWM: %d)                    ║\n", 
                  pumpActive ? "✅ ON" : "❌ OFF", pumpPWM);
    Serial.printf("║ 밸브: %s                              ║\n",
                  valveActive ? "✅ ON" : "❌ OFF");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ PID 출력: %.1f%%                      ║\n", pidOutput);
    Serial.printf("║ PID 오차: %.2f kPa                   ║\n", pidError);
    Serial.printf("║ PID 적분: %.2f                        ║\n", pidIntegral);
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.printf("║ 안전 상태: %s                         ║\n",
                  isSafeToOperate() ? "✅ 정상" : "⚠️  경고");
    Serial.println("╚═══════════════════════════════════════╝\n");
}
