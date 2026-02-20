// ================================================================
// PID_Control.cpp  —  PID 제어 루프
// ================================================================
#include "Config.h"
#include "PID_Control.h"
#include "Control.h"        // controlPump()

// ─────────────────── PID 업데이트 ───────────────────────────
void updatePID() {
  uint32_t currentTime = millis();

  // 주기 체크
  if (currentTime - lastPIDUpdate < PID_UPDATE_INTERVAL) return;

  float dt = (currentTime - lastPIDUpdate) / 1000.0f;
  lastPIDUpdate = currentTime;

  // 에러 계산
  pidError = config.targetPressure - sensorData.pressure;

  // 적분  (Anti-windup)
  pidIntegral += pidError * dt;
  pidIntegral  = constrain(pidIntegral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

  // 미분
  pidDerivative = (pidError - pidLastError) / dt;
  pidLastError  = pidError;

  // PID 출력
  pidOutput = (config.pidKp * pidError)
            + (config.pidKi * pidIntegral)
            + (config.pidKd * pidDerivative);
  pidOutput = constrain(pidOutput, PID_OUTPUT_MIN, PID_OUTPUT_MAX);

  // PWM 매핑
  uint8_t pwm = map((long)pidOutput,
                    (long)PID_OUTPUT_MIN, (long)PID_OUTPUT_MAX,
                    (long)PWM_MIN,        (long)PWM_MAX);

  // 펌프 제어 (진공 작동 중에만)
  if (currentState == STATE_VACUUM_ON || currentState == STATE_VACUUM_HOLD) {
    controlPump(true, pwm);
  }

  // 디버그 출력 (5초 간격)
  static uint32_t lastDebugPrint = 0;
  if (currentTime - lastDebugPrint >= 5000) {
    Serial.printf("[PID] Error: %.2f, I: %.2f, D: %.2f, Output: %.1f%%, PWM: %d\n",
                  pidError, pidIntegral, pidDerivative, pidOutput, pwm);
    lastDebugPrint = currentTime;
  }
}

// ─────────────────── PID 리셋 ───────────────────────────────
void resetPID() {
  pidError      = 0.0f;
  pidIntegral   = 0.0f;
  pidDerivative = 0.0f;
  pidLastError  = 0.0f;
  pidOutput     = 0.0f;
  Serial.println("[PID] 리셋 완료");
}
