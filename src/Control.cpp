// ================================================================
// Control.cpp    12V ,  ,  
// v3.9.2 Final - SensorManager  + FreeRTOS 
// ================================================================

#include "Config.h"
#include "Control.h"
#include "PID_Control.h"
#include "SensorManager.h"  //  SensorManager 

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// SensorManager  
extern SensorManager sensorManager;

//     
bool    pumpActive  = false;
bool    valveActive = false;
uint8_t pumpPWM     = 0;

//    
void controlPump(bool enable, uint8_t pwm) {
  if (!checkSafetyInterlock(enable, valveActive)) {
    Serial.println("[] :   (  )");
    enable = false;
  }

  pumpActive = enable;
  pumpPWM    = enable ? constrain(pwm, PWM_MIN, PWM_MAX) : 0;

  if (enable) {
    control12VMain(true);
    ledcWrite(PWM_CHANNEL_PUMP, pumpPWM);
  } else {
    ledcWrite(PWM_CHANNEL_PUMP, 0);
    control12VMain(false);
  }

  Serial.printf("[] : %s (PWM: %d)\n", enable ? "ON" : "OFF", pumpPWM);
}

//    
void controlValve(bool enable) {
  if (!checkSafetyInterlock(pumpActive, enable)) {
    Serial.println("[] :   (  )");
    return;
  }

  //     
  if (enable && pumpActive) {
    controlPump(false);
    vTaskDelay(pdMS_TO_TICKS(100));  // FreeRTOS delay
  }

  valveActive = enable;
  digitalWrite(PIN_VALVE, enable ? HIGH : LOW);

  Serial.printf("[] : %s\n", enable ? "ON" : "OFF");
}

//  12V   
void control12VMain(bool enable) {
  digitalWrite(PIN_12V_MAIN, enable ? HIGH : LOW);
}

void control12VEmergency(bool enable) {
  digitalWrite(PIN_12V_EMERGENCY, enable ? HIGH : LOW);
}

//    
bool checkSafetyInterlock(bool requestPump, bool requestValve) {
  //     
  if (requestPump && requestValve) return false;
  return true;
}

//    
void emergencyShutdown() {
  controlPump(false);
  controlValve(false);
  control12VMain(false);
  control12VEmergency(true);   //   

  if (currentMode == MODE_PID) resetPID();

  Serial.println("[]   ");
}

// ================================================================
//     (SensorManager )
// ================================================================

void updateControl() {
  // SensorManager   
  float pressure = sensorManager.getPressure();
  float current = sensorManager.getCurrent();
  float temperature = sensorManager.getTemperature();
  bool emergencyStop = sensorManager.getEmergencyStop();
  bool limitSwitch = sensorManager.getLimitSwitch();
  
  //     
  if (emergencyStop) {
    emergencyShutdown();
    Serial.println("[]    !");
    return;
  }
  
  //    
  if (current > CURRENT_THRESHOLD_CRITICAL) {
    emergencyShutdown();
    Serial.printf("[]  ! (%.2f A)\n", current);
    return;
  }
  
  //    
  if (temperature > TEMP_THRESHOLD_SHUTDOWN) {
    emergencyShutdown();
    Serial.printf("[]  ! (%.2f C)\n", temperature);
    return;
  }
  
  //     
  if (currentMode == MODE_MANUAL) {
    //    
    return;
  }
  
  if (currentMode == MODE_AUTO) {
    //  :  
    if (pressure > TARGET_PRESSURE + PRESSURE_HYSTERESIS) {
      //    -  
      if (!pumpActive) {
        controlPump(true, 200);  // PWM 200 
      }
    } 
    else if (pressure < TARGET_PRESSURE - PRESSURE_HYSTERESIS) {
      //    -  
      if (pumpActive) {
        controlPump(false);
      }
    }
    //      
  }
  
  //    
  if (current > CURRENT_THRESHOLD_WARNING) {
    Serial.printf("[]  : %.2f A\n", current);
  }
  
  //    
  if (temperature > TEMP_THRESHOLD_WARNING) {
    Serial.printf("[]  : %.2f C\n", temperature);
  }
}

// ================================================================
//  
// ================================================================

void initControl() {
  //    main.cpp 
  //   
  
  pumpActive = false;
  valveActive = false;
  pumpPWM = 0;
  
  //   OFF
  ledcWrite(PWM_CHANNEL_PUMP, 0);
  digitalWrite(PIN_VALVE, LOW);
  digitalWrite(PIN_12V_MAIN, LOW);
  digitalWrite(PIN_12V_EMERGENCY, LOW);
  
  Serial.println("[Control]  ");
  Serial.println("[Control] -   ");
  Serial.println("[Control] - PWM  ");
  Serial.println("[Control] - SensorManager ");
}

// ================================================================
//   ()
// ================================================================

void printControlStatus() {
  Serial.println("\n===   ===");
  Serial.printf(": %s", pumpActive ? "ON" : "OFF");
  if (pumpActive) {
    Serial.printf(" (PWM: %d, %.1f%%)\n", pumpPWM, (pumpPWM / 255.0) * 100);
  } else {
    Serial.println();
  }
  Serial.printf(": %s\n", valveActive ? "ON" : "OFF");
  Serial.printf("12V : %s\n", digitalRead(PIN_12V_MAIN) ? "ON" : "OFF");
  Serial.printf("12V : %s\n", digitalRead(PIN_12V_EMERGENCY) ? "ON" : "OFF");
  
  //   (SensorManager )
  Serial.println("\n---   ---");
  Serial.printf(": %.2f kPa\n", sensorManager.getPressure());
  Serial.printf(": %.2f A\n", sensorManager.getCurrent());
  Serial.printf(": %.2f C\n", sensorManager.getTemperature());
  Serial.printf(": %s\n", sensorManager.getEmergencyStop() ? "" : "");
  Serial.printf("SW: %s\n", sensorManager.getLimitSwitch() ? "ON" : "OFF");
  Serial.println("==================\n");
}

// ================================================================
//   
// ================================================================

bool performSafetyCheck() {
  bool safe = true;
  
  //    
  if (sensorManager.getEmergencyStop()) {
    Serial.println("[]    !");
    safe = false;
  }
  
  //  
  if (sensorManager.getCurrent() > CURRENT_THRESHOLD_CRITICAL) {
    Serial.printf("[] : %.2f A\n", sensorManager.getCurrent());
    safe = false;
  }
  
  //  
  if (sensorManager.getTemperature() > TEMP_THRESHOLD_SHUTDOWN) {
    Serial.printf("[] : %.2f C\n", sensorManager.getTemperature());
    safe = false;
  }
  
  //  
  if (pumpActive && valveActive) {
    Serial.println("[]  : /  !");
    safe = false;
  }
  
  if (!safe) {
    emergencyShutdown();
  }
  
  return safe;
}

// ================================================================
// PWM   ( /)
// ================================================================

void setPumpPWM(uint8_t targetPWM, uint16_t rampTimeMs) {
  if (!pumpActive) {
    Serial.println("[]  .  .");
    return;
  }
  
  targetPWM = constrain(targetPWM, PWM_MIN, PWM_MAX);
  
  if (rampTimeMs == 0) {
    //  
    pumpPWM = targetPWM;
    ledcWrite(PWM_CHANNEL_PUMP, pumpPWM);
    Serial.printf("[] PWM  : %d\n", pumpPWM);
  } else {
    //  
    int step = (targetPWM > pumpPWM) ? 1 : -1;
    uint16_t delayMs = rampTimeMs / abs(targetPWM - pumpPWM);
    delayMs = max(delayMs, (uint16_t)10);  //  10ms
    
    Serial.printf("[] PWM : %d  %d (%dms)\n", pumpPWM, targetPWM, rampTimeMs);
    
    while (pumpPWM != targetPWM) {
      pumpPWM += step;
      ledcWrite(PWM_CHANNEL_PUMP, pumpPWM);
      vTaskDelay(pdMS_TO_TICKS(delayMs));
      
      //  
      if (sensorManager.getEmergencyStop()) {
        emergencyShutdown();
        return;
      }
    }
    
    Serial.printf("[] PWM  : %d\n", pumpPWM);
  }
}

// ================================================================
//    ()
// ================================================================

void setPump(bool on) {
  if (on) {
    controlPump(true, 200);  //  PWM 200
  } else {
    controlPump(false);
  }
}

void setValve(bool on) {
  controlValve(on);
}
