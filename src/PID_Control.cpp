// ================================================================
// PID_Control.cpp    PID  
// ================================================================
#include "Config.h"
#include "PID_Control.h"
#include "Control.h"        // controlPump()
#include "SensorManager.h"

//  PID  
static float    pidError      = 0.0f;
static float    pidIntegral   = 0.0f;
static float    pidDerivative = 0.0f;
static float    pidLastError  = 0.0f;
static float    pidOutput     = 0.0f;
static uint32_t lastPIDUpdate = 0;

//  
extern SensorManager sensorManager;


//  PID  
void updatePID() {
  uint32_t currentTime = millis();

  //  
  if (currentTime - lastPIDUpdate < PID_UPDATE_INTERVAL) return;

  float dt = (currentTime - lastPIDUpdate) / 1000.0f;
  lastPIDUpdate = currentTime;

  //  
  pidError = config.targetPressure - sensorManager.getPressure();

  //   (Anti-windup)
  pidIntegral += pidError * dt;
  pidIntegral  = constrain(pidIntegral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

  // 
  pidDerivative = (pidError - pidLastError) / dt;
  pidLastError  = pidError;

  // PID 
  pidOutput = (config.pidKp * pidError)
            + (config.pidKi * pidIntegral)
            + (config.pidKd * pidDerivative);
  pidOutput = constrain(pidOutput, PID_OUTPUT_MIN, PID_OUTPUT_MAX);

  // PWM 
  uint8_t pwm = map((long)pidOutput,
                    (long)PID_OUTPUT_MIN, (long)PID_OUTPUT_MAX,
                    (long)PWM_MIN,        (long)PWM_MAX);

  //   (  )
  if (currentState == STATE_VACUUM_ON || currentState == STATE_VACUUM_HOLD) {
    controlPump(true, pwm);
  }

  //   (5 )
  static uint32_t lastDebugPrint = 0;
  if (currentTime - lastDebugPrint >= 5000) {
    Serial.printf("[PID] Error: %.2f, I: %.2f, D: %.2f, Output: %.1f%%, PWM: %d\n",
                  pidError, pidIntegral, pidDerivative, pidOutput, pwm);
    lastDebugPrint = currentTime;
  }
}

//  PID  
void resetPID() {
  pidError      = 0.0f;
  pidIntegral   = 0.0f;
  pidDerivative = 0.0f;
  pidLastError  = 0.0f;
  pidOutput     = 0.0f;
  Serial.println("[PID]  ");
}
