// ================================================================
// ErrorHandler.cpp  —  에러 관리 v3.9 (음성 알림 통합)
// ================================================================
#include "Config.h"
#include "ErrorHandler.h"
#include "StateMachine.h"  // changeState, previousState
#include "SD_Logger.h"     // logError(ErrorInfo&)

// v3.9: 음성 알림
#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern VoiceAlert voiceAlert;
#endif

// ─────────────────── 에러 설정 ──────────────────────────────
void setError(ErrorCode code, ErrorSeverity severity, const char* message) {
  currentError.code       = code;
  currentError.severity   = severity;
  currentError.timestamp  = millis();
  currentError.retryCount = 0;
  strncpy(currentError.message, message, sizeof(currentError.message) - 1);

  errorActive = true;
  stats.totalErrors++;

  Serial.printf("[에러] %s (심각도: %d)\n", message, severity);

  // v3.9: 음성 알림 - 에러 발생 시 자동 재생
  #ifdef ENABLE_VOICE_ALERTS
  if (voiceAlert.isOnline()) {
    voiceAlert.playErrorMessage(code);
    
    // 치명적 에러는 볼륨 증가
    if (severity == SEVERITY_CRITICAL) {
      uint8_t savedVolume = voiceAlert.getVolume();
      voiceAlert.setVolume(VOICE_VOLUME_ERROR);
      vTaskDelay(pdMS_TO_TICKS(100));
      voiceAlert.setVolume(savedVolume);
    }
  }
  #endif

  // SD 로그 저장
  logError(currentError);

  // ★ ring buffer 저장
  errorHistory[errorHistIdx] = currentError;
  errorHistIdx = (errorHistIdx + 1) % ERROR_HIST_MAX;
  if (errorHistCnt < ERROR_HIST_MAX) errorHistCnt++;
}

// ─────────────────── 에러 해제 ──────────────────────────────
void clearError() {
  currentError.code       = ERROR_NONE;
  currentError.retryCount = 0;
  errorActive = false;

  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_BUZZER,  LOW);

  Serial.println("[에러] 해제됨");
  
  // v3.9: 음성 알림 - 시스템 복구 안내
  #ifdef ENABLE_VOICE_ALERTS
  if (voiceAlert.isOnline()) {
    voiceAlert.playSystem(VOICE_READY);  // "준비 완료"
  }
  #endif
}

// ─────────────────── 에러 처리 루프 ─────────────────────────
void handleError() {
  if (!errorActive) return;

  if (attemptErrorRecovery()) {
    clearError();
    changeState(previousState != STATE_ERROR ? previousState : STATE_IDLE);
  }
}

// ─────────────────── 복구 시도 ──────────────────────────────
bool attemptErrorRecovery() {
  switch (currentError.severity) {
    case SEVERITY_TEMPORARY:
      // 3회까지 재시도, 30초 간격
      if (currentError.retryCount < 3) {
        static uint32_t lastRetryTime = 0;
        if (millis() - lastRetryTime >= 30000) {
          currentError.retryCount++;
          lastRetryTime = millis();
          Serial.printf("[복구] 재시도 %d/3\n", currentError.retryCount);
          
          // v3.9: 음성 안내
          #ifdef ENABLE_VOICE_ALERTS
          if (voiceAlert.isOnline()) {
            voiceAlert.playGuide(VOICE_GUIDE_WAIT);  // "잠시 기다려주세요"
          }
          #endif
          
          return true;
        }
      }
      break;

    case SEVERITY_RECOVERABLE:
      // 2회까지 재시도
      if (currentError.retryCount < 2) {
        currentError.retryCount++;
        Serial.printf("[복구] 재시도 %d/2\n", currentError.retryCount);
        
        // v3.9: 음성 안내
        #ifdef ENABLE_VOICE_ALERTS
        if (voiceAlert.isOnline()) {
          voiceAlert.playGuide(VOICE_GUIDE_CHECK_SYSTEM);  // "시스템을 점검해주세요"
        }
        #endif
        
        vTaskDelay(pdMS_TO_TICKS(1000));
        return true;
      }
      break;

    case SEVERITY_CRITICAL:
      // 복구 불가 — 수동 개입 필요
      Serial.println("[복구] 불가 - 수동 개입 필요");
      
      // v3.9: 음성 알림 - 치명적 에러는 반복 재생
      #ifdef ENABLE_VOICE_ALERTS
      static uint32_t lastCriticalAlert = 0;
      if (voiceAlert.isOnline() && millis() - lastCriticalAlert >= 60000) {
        // 1분마다 반복
        voiceAlert.setVolume(VOICE_VOLUME_EMERGENCY);
        voiceAlert.playErrorMessage(currentError.code);
        voiceAlert.enableRepeat(true);
        voiceAlert.setRepeatCount(3);  // 3회 반복
        lastCriticalAlert = millis();
      }
      #endif
      
      return false;
  }

  return false;
}

// ─────────────────── 에러 코드 → 문자열 변환 ─────────────────
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

// ─────────────────── 에러 심각도 → 문자열 변환 ───────────────
const char* getErrorSeverityString(ErrorSeverity severity) {
  switch (severity) {
    case SEVERITY_TEMPORARY:   return "TEMPORARY";
    case SEVERITY_RECOVERABLE: return "RECOVERABLE";
    case SEVERITY_CRITICAL:    return "CRITICAL";
    default:                   return "UNKNOWN";
  }
}
