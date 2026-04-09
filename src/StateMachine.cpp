// ================================================================
// StateMachine.cpp      v3.9 (  )
// StateMachine.cpp  changeState()  Mutex 
// :
// - [K3] Mutex / 
// -    (Mutex )
// -   100% 
// ================================================================
#include "Config.h"
#include "SensorManager.h"
extern SensorManager sensorManager;

#include "StateMachine.h"
#include "Control.h"
#include "PID_Control.h"
#include "ErrorHandler.h"
#include "SD_Logger.h"
#include "Trend_Graph.h"
#include "Lang.h"

// v3.9:  
#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern VoiceAlert voiceAlert;
#endif

//   
static uint8_t holdExtensionCount = 0;

// [K3]   Mutex (/   )
static SemaphoreHandle_t g_stateMutex = nullptr;

//     
void updateStateMachine() {
  uint32_t elapsedTime = millis() - stateStartTime;

  //   (NC : LOW = )
  if (!sensorManager.getEmergencyStop()) {
    changeState(STATE_EMERGENCY_STOP);
    return;
  }

  //  
  if (sensorManager.getCurrent() > CURRENT_THRESHOLD_CRITICAL) {
    setError(ERROR_OVERCURRENT, SEVERITY_CRITICAL, " ");
    changeState(STATE_ERROR);
    return;
  }

  //    (v3.5) 
  if (config.tempSensorEnabled) {
    //    (70C)
    if (sensorManager.getTemperature() >= config.tempShutdown) {
      setError(ERROR_OVERHEAT, SEVERITY_CRITICAL, " -  ");
      changeState(STATE_EMERGENCY_STOP);
      return;
    }
    
    //   (60C) -   
    if (sensorManager.getTemperature() >= config.tempCritical) {
      setError(ERROR_OVERHEAT, SEVERITY_RECOVERABLE, " -  ");
      changeState(STATE_ERROR);
      return;
    }
    
    //   (50C) -  +  
    if (sensorManager.getTemperature() >= config.tempWarning) {
      static uint32_t lastBeep = 0;
      if (millis() - lastBeep >= 10000) {  // 10
        digitalWrite(PIN_BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(PIN_BUZZER, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(PIN_BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(PIN_BUZZER, LOW);
        
        Serial.printf("[]  : %.1fC\n", sensorManager.getTemperature());
        
        // v3.9:  
        #ifdef ENABLE_VOICE_ALERTS
        if (voiceAlert.isOnline()) {
          voiceAlert.enqueue(1, 5);  //  
        }
        #endif
        
        lastBeep = millis();
      }
    }
  }

  //  
  switch (currentState) {
    //  IDLE 
    case STATE_IDLE:
      if (sensorManager.getLimitSwitch()) changeState(STATE_VACUUM_ON);
      break;

    //  VACUUM_ON 
    case STATE_VACUUM_ON:
      if (currentMode == MODE_AUTO) {
        if (elapsedTime >= config.vacuumOnTime) changeState(STATE_VACUUM_HOLD);
      } else if (currentMode == MODE_PID) {
        if (sensorManager.getPressure() <= config.targetPressure + config.pressureHysteresis) {
          changeState(STATE_VACUUM_HOLD);
        }
      }
      break;

    //  VACUUM_HOLD 
    case STATE_VACUUM_HOLD:
      if (elapsedTime >= config.vacuumHoldTime) {
        changeState(STATE_VACUUM_BREAK);
      }
      break;

    //  VACUUM_BREAK 
    case STATE_VACUUM_BREAK:
      if (elapsedTime >= config.vacuumBreakTime) {
        changeState(STATE_WAIT_REMOVAL);
      }
      break;

    //  WAIT_REMOVAL (  ) 
    case STATE_WAIT_REMOVAL:
      //   (  )
      if (!sensorManager.getPhotoSensor()) {
        Serial.println("[WAIT_REMOVAL]     COMPLETE");
        holdExtensionCount = 0;
        changeState(STATE_COMPLETE);
        break;
      }
      
      //  
      if (elapsedTime >= config.waitRemovalTime) {
        //    &&    
        if (config.holdExtensionEnabled && 
            holdExtensionCount < config.maxHoldExtensions) {
          
          holdExtensionCount++;
          stateStartTime = millis();
          
          Serial.printf("[WAIT_REMOVAL]   %d/%d (+ %lu ms)\n",
                        holdExtensionCount, 
                        config.maxHoldExtensions,
                        config.vacuumHoldExtension);
          
          //  
          digitalWrite(PIN_BUZZER, HIGH);
          vTaskDelay(pdMS_TO_TICKS(100));
          digitalWrite(PIN_BUZZER, LOW);
          
          // v3.9:   (  )
          #ifdef ENABLE_VOICE_ALERTS
          if (voiceAlert.isOnline()) {
            voiceAlert.enqueue(1, 6);  //   
          }
          #endif
        } 
        else {
          Serial.printf("[WAIT_REMOVAL]  ( %d )  ERROR\n", 
                        holdExtensionCount);
          holdExtensionCount = 0;
          setError(ERROR_PHOTO_TIMEOUT, SEVERITY_INFO, "  ");
          changeState(STATE_ERROR);
        }
      }
      break;

    //  COMPLETE 
    case STATE_COMPLETE:
      if (elapsedTime >= 1000) changeState(STATE_IDLE);
      break;

    //  ERROR 
    case STATE_ERROR:
      //       
      if (config.tempSensorEnabled && 
          currentError.code == ERROR_OVERHEAT &&
          sensorManager.getTemperature() < config.tempCritical - 5.0) {
        Serial.println("[ERROR]     ");
        clearError();
        changeState(STATE_IDLE);
      }
      break;

    //  EMERGENCY_STOP 
    case STATE_EMERGENCY_STOP:
      if (sensorManager.getEmergencyStop()) {
        //       
        if (currentError.code == ERROR_OVERHEAT) {
          if (sensorManager.getTemperature() < config.tempShutdown - 10.0) {
            Serial.println("[EMERGENCY]     ");
            clearError();
            changeState(STATE_IDLE);
          }
        } else {
          changeState(STATE_IDLE);
        }
      }
      break;
  }
}

// StateMachine   Mutex  void 
void initStateMachine() {
  if (g_stateMutex == nullptr) {
    g_stateMutex = xSemaphoreCreateMutex();
    if (g_stateMutex == nullptr) {
      Serial.println("[StateMachine]   Mutex  !");
    } else {
      Serial.println("[StateMachine] Mutex  ");
    }
  }
}

//  changeState()  Mutex 
void changeState(SystemState newState) {
  // [K3] Mutex  (100ms )
  if (g_stateMutex == nullptr || 
      xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("[StateMachine]   Mutex      ");
    return;
  }

  //   
  if (currentState == newState) {
    xSemaphoreGive(g_stateMutex);
    return;
  }

  previousState = currentState;
  currentState = newState;
  stateStartTime = millis();
  screenNeedsRedraw = true;
  
  Serial.printf("[ ] %s  %s\n", 
                getStateName(previousState), 
                getStateName(currentState));
  
  // v3.9:   -     
  #ifdef ENABLE_VOICE_ALERTS
  if (voiceAlert.isOnline() && voiceAlert.isOnline()) {
    voiceAlert.enqueue(1, (uint8_t)newState + 1);  //  
  }
  #endif
    
  //    
  if (newState == STATE_WAIT_REMOVAL) {
    holdExtensionCount = 0;
  }

  //   
  switch (newState) {
    case STATE_IDLE:
      controlPump(0);
      controlValve(false);
      resetPID();
      break;

    case STATE_VACUUM_ON:
      controlValve(false);
      stats.totalCycles++;
      // initGraphData();  // 
      break;

    case STATE_VACUUM_HOLD:
      if (currentMode == MODE_AUTO) controlPump(200  /* manualPWM  */);
      break;

    case STATE_VACUUM_BREAK:
      controlPump(0);
      controlValve(true);
      break;

    case STATE_WAIT_REMOVAL:
      controlPump(0);
      controlValve(false);
      
      // v3.9:    
      #ifdef ENABLE_VOICE_ALERTS
      if (voiceAlert.isOnline()) {
        voiceAlert.enqueue(1, 6);  //   
      }
      #endif
      break;

    case STATE_COMPLETE:
      stats.successfulCycles++;
      logCycle();
      digitalWrite(PIN_BUZZER, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
      digitalWrite(PIN_BUZZER, LOW);
      break;

    case STATE_ERROR:
      controlPump(0);
      controlValve(true);
      stats.failedCycles++;
      stats.totalErrors++;
      logCycle();
      digitalWrite(PIN_BUZZER, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(PIN_BUZZER, LOW);
      break;

    case STATE_EMERGENCY_STOP:
      emergencyShutdown();
      digitalWrite(PIN_BUZZER, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(PIN_BUZZER, LOW);
      break;
  }

  // [K3] Mutex 
  xSemaphoreGive(g_stateMutex);
}

//     
const char* getStateName(SystemState state) {
  switch (state) {
    case STATE_IDLE:            return L(SN_IDLE);
    case STATE_VACUUM_ON:       return L(SN_VAC_ON);
    case STATE_VACUUM_HOLD:     return L(SN_VAC_HOLD);
    case STATE_VACUUM_BREAK:    return L(SN_VAC_BREAK);
    case STATE_WAIT_REMOVAL:    return L(SN_WAIT_REM);
    case STATE_COMPLETE:        return L(SN_COMPLETE);
    case STATE_ERROR:           return L(SN_ERROR);
    case STATE_EMERGENCY_STOP:  return L(SN_EMERGENCY);
    default:                    return L(SN_UNKNOWN);
  }
}
