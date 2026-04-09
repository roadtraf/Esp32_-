// ================================================================
// ErrorHandler.cpp      v3.9 (  )
// ================================================================
#include "Config.h"
#include "ErrorHandler.h"
#include "StateMachine.h"  // changeState, previousState
#include "SD_Logger.h"     // logError(ErrorInfo&)

// v3.9:  
#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"

// FreeRTOS (delay )
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern VoiceAlert voiceAlert;
#endif

//    
void setError(ErrorCode code, ErrorSeverity severity, const char* message) {
  currentError.code       = code;
  currentError.severity   = severity;
  currentError.timestamp  = millis();
  currentError.retryCount = 0;
  strncpy(currentError.message, message, sizeof(currentError.message) - 1);

  errorActive = true;
  stats.totalErrors++;

  Serial.printf("[] %s (: %d)\n", message, severity);

  // v3.9:   -     
  #ifdef ENABLE_VOICE_ALERTS
  if (voiceAlert.isOnline()) {
    // voiceAlert.playErrorMessage(code);  //  
    
    //    
    if (severity == SEVERITY_CRITICAL) {
      voiceAlert.setVolume(25);  //    
    }
  }
  #endif

  // SD  
  logError(currentError);

  //  ring buffer 
  errorHistory[errorHistIdx] = currentError;
  errorHistIdx = (errorHistIdx + 1) % ERROR_HIST_MAX;
  if (errorHistCnt < ERROR_HIST_MAX) errorHistCnt++;
}

//    
void clearError() {
  currentError.code       = ERROR_NONE;
  currentError.retryCount = 0;
  errorActive = false;

  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_BUZZER,  LOW);

  Serial.println("[] ");
  
  // v3.9:   -   
  #ifdef ENABLE_VOICE_ALERTS
  if (voiceAlert.isOnline()) {
    voiceAlert.enqueue(1, 1);  //  
  }
  #endif
}

//     
void handleError() {
  if (!errorActive) return;

  if (attemptErrorRecovery()) {
    clearError();
    changeState(previousState != STATE_ERROR ? previousState : STATE_IDLE);
  }
}

//    
bool attemptErrorRecovery() {
  switch (currentError.severity) {
    case SEVERITY_INFO:  // SEVERITY_TEMPORARY  SEVERITY_INFO
      // 3 , 30 
      if (currentError.retryCount < 3) {
        static uint32_t lastRetryTime = 0;
        if (millis() - lastRetryTime >= 30000) {
          currentError.retryCount++;
          lastRetryTime = millis();
          Serial.printf("[]  %d/3\n", currentError.retryCount);
          
          // v3.9:  
          #ifdef ENABLE_VOICE_ALERTS
          if (voiceAlert.isOnline()) {
            voiceAlert.enqueue(1, 2);  //  
          }
          #endif
          
          return true;
        }
      }
      break;

    case SEVERITY_RECOVERABLE:
      // 2 
      if (currentError.retryCount < 2) {
        currentError.retryCount++;
        Serial.printf("[]  %d/2\n", currentError.retryCount);
        
        // v3.9:  
        #ifdef ENABLE_VOICE_ALERTS
        if (voiceAlert.isOnline()) {
          voiceAlert.enqueue(1, 3);  //  
        }
        #endif
        
        vTaskDelay(pdMS_TO_TICKS(1000));
        return true;
      }
      break;

    case SEVERITY_CRITICAL:
      //      
      Serial.println("[]  -   ");
      
      // v3.9:   -    
      #ifdef ENABLE_VOICE_ALERTS
      static uint32_t lastCriticalAlert = 0;
      if (voiceAlert.isOnline() && millis() - lastCriticalAlert >= 60000) {
        // 1 
        voiceAlert.setVolume(VOICE_VOLUME_EMERGENCY);
        // voiceAlert.playErrorMessage(currentError.code);
        // voiceAlert.enableRepeat(true);  // 
        // voiceAlert.setRepeatCount(3);  // 
        lastCriticalAlert = millis();
      }
      #endif
      
      return false;
  }

  return false;
}

//       
const char* getErrorCodeString(ErrorCode code) {
  switch (code) {
    case ERROR_NONE:              return "NONE";
    case ERROR_OVERCURRENT:       return "OVERCURRENT";
    case ERROR_SENSOR_FAULT:      return "SENSOR_FAULT";
    case ERROR_MOTOR_FAILURE:     return "MOTOR_FAILURE";
    case ERROR_PHOTO_TIMEOUT:     return "PHOTO_TIMEOUT";
    case ERROR_EMERGENCY_STOP:    return "EMERGENCY_STOP";
    case ERROR_WATCHDOG:          return "WATCHDOG";
    case ERROR_MEMORY:            return "MEMORY";
    case ERROR_OVERHEAT:          return "OVERHEAT";
    case ERROR_TEMP_SENSOR_FAULT: return "TEMP_SENSOR_FAULT";
    default:                      return "UNKNOWN";
  }
}

//       
const char* getErrorSeverityString(ErrorSeverity severity) {
  switch (severity) {
    case SEVERITY_INFO:  // SEVERITY_TEMPORARY  SEVERITY_INFO   return "TEMPORARY";
    case SEVERITY_RECOVERABLE: return "RECOVERABLE";
    case SEVERITY_CRITICAL:    return "CRITICAL";
    default:                   return "UNKNOWN";
  }
}
