// ================================================================
// StateMachine.cpp  —  상태 머신 v3.9 (음성 알림 통합)
// StateMachine.cpp — changeState() 함수 Mutex 추가
// 변경사항:
// - [K3] Mutex 획득/해제 추가
// - 중복 진입 방지 (Mutex 보호)
// - 원본 로직 100% 유지
// ================================================================
#include "Config.h"
#include "StateMachine.h"
#include "Control.h"
#include "PID_Control.h"
#include "ErrorHandler.h"
#include "SD_Logger.h"
#include "Trend_Graph.h"
#include "Lang.h"

// v3.9: 음성 알림
#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern VoiceAlert voiceAlert;
#endif

// 자동 연장 카운터
static uint8_t holdExtensionCount = 0;

// [K3] 상태 전이 Mutex (터치/키보드 동시 입력 보호)
static SemaphoreHandle_t g_stateMutex = nullptr;

// ─────────────────── 매 루프 체크 ───────────────────────────
void updateStateMachine() {
  uint32_t elapsedTime = millis() - stateStartTime;

  // 비상정지 체크 (NC 타입: LOW = 트리거)
  if (!sensorData.emergencyStop) {
    changeState(STATE_EMERGENCY_STOP);
    return;
  }

  // 과전류 체크
  if (sensorData.current > CURRENT_THRESHOLD_CRITICAL) {
    setError(ERROR_OVERCURRENT, SEVERITY_CRITICAL, "과전류 감지");
    changeState(STATE_ERROR);
    return;
  }

  // ═══ 온도 체크 (v3.5) ═══
  if (config.tempSensorEnabled) {
    // 강제 정지 온도 (70°C)
    if (sensorData.temperature >= config.tempShutdown) {
      setError(ERROR_OVERHEAT, SEVERITY_CRITICAL, "과열 - 강제 정지");
      changeState(STATE_EMERGENCY_STOP);
      return;
    }
    
    // 위험 온도 (60°C) - 에러 상태로 전환
    if (sensorData.temperature >= config.tempCritical) {
      setError(ERROR_OVERHEAT, SEVERITY_RECOVERABLE, "과열 - 냉각 필요");
      changeState(STATE_ERROR);
      return;
    }
    
    // 경고 온도 (50°C) - 경고음 + 음성 알림
    if (sensorData.temperature >= config.tempWarning) {
      static uint32_t lastBeep = 0;
      if (millis() - lastBeep >= 10000) {  // 10초마다
        digitalWrite(PIN_BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(PIN_BUZZER, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(PIN_BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(PIN_BUZZER, LOW);
        
        Serial.printf("[경고] 온도 상승: %.1f°C\n", sensorData.temperature);
        
        // v3.9: 음성 경고
        #ifdef ENABLE_VOICE_ALERTS
        if (voiceAlert.isOnline()) {
          voiceAlert.playError(VOICE_ERROR_OVERHEAT);
        }
        #endif
        
        lastBeep = millis();
      }
    }
  }

  // 상태별 처리
  switch (currentState) {
    // ── IDLE ──────────────────────────────────────────────
    case STATE_IDLE:
      if (sensorData.limitSwitch) changeState(STATE_VACUUM_ON);
      break;

    // ── VACUUM_ON ─────────────────────────────────────────
    case STATE_VACUUM_ON:
      if (currentMode == MODE_AUTO) {
        if (elapsedTime >= config.vacuumOnTime) changeState(STATE_VACUUM_HOLD);
      } else if (currentMode == MODE_PID) {
        if (sensorData.pressure <= config.targetPressure + config.pressureHysteresis) {
          changeState(STATE_VACUUM_HOLD);
        }
      }
      break;

    // ── VACUUM_HOLD ───────────────────────────────────────
    case STATE_VACUUM_HOLD:
      if (elapsedTime >= config.vacuumHoldTime) {
        changeState(STATE_VACUUM_BREAK);
      }
      break;

    // ── VACUUM_BREAK ──────────────────────────────────────
    case STATE_VACUUM_BREAK:
      if (elapsedTime >= config.vacuumBreakTime) {
        changeState(STATE_WAIT_REMOVAL);
      }
      break;

    // ── WAIT_REMOVAL (자동 연장 기능) ──────────────────────
    case STATE_WAIT_REMOVAL:
      // 광센서 체크 (박스 제거 감지)
      if (!sensorData.photoSensor) {
        Serial.println("[WAIT_REMOVAL] 박스 제거 감지 → COMPLETE");
        holdExtensionCount = 0;
        changeState(STATE_COMPLETE);
        break;
      }
      
      // 타임아웃 체크
      if (elapsedTime >= config.waitRemovalTime) {
        // 자동 연장 활성화 && 연장 가능 횟수 남음
        if (config.holdExtensionEnabled && 
            holdExtensionCount < config.maxHoldExtensions) {
          
          holdExtensionCount++;
          stateStartTime = millis();
          
          Serial.printf("[WAIT_REMOVAL] 자동 연장 %d/%d (+ %lu ms)\n",
                        holdExtensionCount, 
                        config.maxHoldExtensions,
                        config.vacuumHoldExtension);
          
          // 부드러운 알림음
          digitalWrite(PIN_BUZZER, HIGH);
          vTaskDelay(pdMS_TO_TICKS(100));
          digitalWrite(PIN_BUZZER, LOW);
          
          // v3.9: 음성 안내 (박스 제거 요청)
          #ifdef ENABLE_VOICE_ALERTS
          if (voiceAlert.isOnline()) {
            voiceAlert.playGuide(VOICE_GUIDE_REMOVE_BOX);
          }
          #endif
        } 
        else {
          Serial.printf("[WAIT_REMOVAL] 타임아웃 (연장 %d회 후) → ERROR\n", 
                        holdExtensionCount);
          holdExtensionCount = 0;
          setError(ERROR_PHOTO_TIMEOUT, SEVERITY_TEMPORARY, "박스 제거 타임아웃");
          changeState(STATE_ERROR);
        }
      }
      break;

    // ── COMPLETE ──────────────────────────────────────────
    case STATE_COMPLETE:
      if (elapsedTime >= 1000) changeState(STATE_IDLE);
      break;

    // ── ERROR ─────────────────────────────────────────────
    case STATE_ERROR:
      // 온도 과열 에러인 경우 자동 복구 체크
      if (config.tempSensorEnabled && 
          currentError.code == ERROR_OVERHEAT &&
          sensorData.temperature < config.tempCritical - 5.0) {
        Serial.println("[ERROR] 온도 하강 → 자동 복구");
        clearError();
        changeState(STATE_IDLE);
      }
      break;

    // ── EMERGENCY_STOP ────────────────────────────────────
    case STATE_EMERGENCY_STOP:
      if (sensorData.emergencyStop) {
        // 온도 과열로 인한 비상정지는 온도가 내려가야 복구
        if (currentError.code == ERROR_OVERHEAT) {
          if (sensorData.temperature < config.tempShutdown - 10.0) {
            Serial.println("[EMERGENCY] 온도 정상화 → 복구 가능");
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

// ================================================================
// StateMachine 초기화 — Mutex 생성
// ================================================================
void initStateMachine() {
  if (g_stateMutex == nullptr) {
    g_stateMutex = xSemaphoreCreateMutex();
    if (g_stateMutex == nullptr) {
      Serial.println("[StateMachine] ⚠️  Mutex 생성 실패!");
    } else {
      Serial.println("[StateMachine] Mutex 생성 완료");
    }
  }
}

// ================================================================
//  changeState() 함수 Mutex 추가
// - [K3] Mutex 획득/해제 추가, 중복 진입 방지 (Mutex 보호)
// ================================================================

void changeState(SystemState newState) {
  // [K3] Mutex 획득 (100ms 타임아웃)
  if (g_stateMutex == nullptr || 
      xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("[StateMachine] ⚠️  Mutex 획득 실패 — 상태 전이 건너뜀");
    return;
  }

  // 중복 진입 방지
  if (currentState == newState) {
    xSemaphoreGive(g_stateMutex);
    return;
  }

  previousState = currentState;
  currentState = newState;
  stateStartTime = millis();
  screenNeedsRedraw = true;
  
  Serial.printf("[상태 전이] %s → %s\n", 
                getStateName(previousState), 
                getStateName(currentState));
  
  // v3.9: 음성 알림 - 상태 변경 시 자동 재생
  #ifdef ENABLE_VOICE_ALERTS
  if (voiceAlert.isOnline() && voiceAlert.isAutoVoiceEnabled()) {
    voiceAlert.playStateMessage(newState);
  }
  #endif
    
  // 상태 진입 시 초기화
  if (newState == STATE_WAIT_REMOVAL) {
    holdExtensionCount = 0;
  }

  // 상태별 진입 처리
  switch (newState) {
    case STATE_IDLE:
      controlPump(0);
      controlValve(false);
      resetPID();
      break;

    case STATE_VACUUM_ON:
      controlValve(false);
      stats.totalCycles++;
      initGraphData();
      break;

    case STATE_VACUUM_HOLD:
      if (currentMode == MODE_AUTO) controlPump(config.manualPWM);
      break;

    case STATE_VACUUM_BREAK:
      controlPump(0);
      controlValve(true);
      break;

    case STATE_WAIT_REMOVAL:
      controlPump(0);
      controlValve(false);
      
      // v3.9: 박스 제거 안내 음성
      #ifdef ENABLE_VOICE_ALERTS
      if (voiceAlert.isOnline()) {
        voiceAlert.playGuide(VOICE_GUIDE_REMOVE_BOX);
      }
      #endif
      break;

    case STATE_COMPLETE:
      stats.successfulCycles++;
      logCycle(true, sensorData.pressure, sensorData.current);
      digitalWrite(PIN_BUZZER, HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
      digitalWrite(PIN_BUZZER, LOW);
      break;

    case STATE_ERROR:
      controlPump(0);
      controlValve(true);
      stats.failedCycles++;
      stats.totalErrors++;
      logCycle(false, sensorData.pressure, sensorData.current);
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

  // [K3] Mutex 해제
  xSemaphoreGive(g_stateMutex);
}

// ─────────────────── 상태 이름 반환 ─────────────────────────
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
