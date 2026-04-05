// ================================================================
// Control.cpp  —  펌프·밸브·12V 제어, 안전 인터락, 비상 셧다운
// v3.9.2 Final - SensorManager 통합 + FreeRTOS 최적화
// ================================================================

#include "Config.h"
#include "Control.h"
#include "PID_Control.h"
#include "SensorManager.h"  // ← SensorManager 추가

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// SensorManager 외부 참조
extern SensorManager sensorManager;

// ─────────────────── 펌프 제어 ──────────────────────────────
void controlPump(bool enable, uint8_t pwm) {
  if (!checkSafetyInterlock(enable, valveActive)) {
    Serial.println("[안전] 인터락: 펌프 차단 (밸브 활성 중)");
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

  Serial.printf("[제어] 펌프: %s (PWM: %d)\n", enable ? "ON" : "OFF", pumpPWM);
}

// ─────────────────── 밸브 제어 ──────────────────────────────
void controlValve(bool enable) {
  if (!checkSafetyInterlock(pumpActive, enable)) {
    Serial.println("[안전] 인터락: 밸브 차단 (펌프 활성 중)");
    return;
  }

  // 펌프가 켜져 있으면 먼저 끄기
  if (enable && pumpActive) {
    controlPump(false);
    vTaskDelay(pdMS_TO_TICKS(100));  // FreeRTOS delay
  }

  valveActive = enable;
  digitalWrite(PIN_VALVE, enable ? HIGH : LOW);

  Serial.printf("[제어] 밸브: %s\n", enable ? "ON" : "OFF");
}

// ─────────────────── 12V 전원 제어 ─────────────────────────
void control12VMain(bool enable) {
  digitalWrite(PIN_12V_MAIN, enable ? HIGH : LOW);
}

void control12VEmergency(bool enable) {
  digitalWrite(PIN_12V_EMERGENCY, enable ? HIGH : LOW);
}

// ─────────────────── 안전 인터락 ────────────────────────────
bool checkSafetyInterlock(bool requestPump, bool requestValve) {
  // 펌프와 밸브 동시 동작 금지
  if (requestPump && requestValve) return false;
  return true;
}

// ─────────────────── 비상 셧다운 ────────────────────────────
void emergencyShutdown() {
  controlPump(false);
  controlValve(false);
  control12VMain(false);
  control12VEmergency(true);   // 비상 전원 차단

  if (currentMode == MODE_PID) resetPID();

  Serial.println("[안전] 비상 셧다운 실행됨");
}

// ================================================================
// 센서 기반 자동 제어 (SensorManager 통합)
// ================================================================

void updateControl() {
  // SensorManager에서 센서 값 가져오기
  float pressure = sensorManager.getPressure();
  float current = sensorManager.getCurrent();
  float temperature = sensorManager.getTemperature();
  bool emergencyStop = sensorManager.getEmergencyStop();
  bool limitSwitch = sensorManager.getLimitSwitch();
  
  // ─────────────────── 비상 정지 체크 ──────────────────────
  if (emergencyStop) {
    emergencyShutdown();
    Serial.println("[안전] 비상 정지 버튼 감지!");
    return;
  }
  
  // ─────────────────── 과전류 보호 ─────────────────────────
  if (current > CURRENT_THRESHOLD_CRITICAL) {
    emergencyShutdown();
    Serial.printf("[안전] 과전류 감지! (%.2f A)\n", current);
    return;
  }
  
  // ─────────────────── 과열 보호 ───────────────────────────
  if (temperature > TEMP_THRESHOLD_SHUTDOWN) {
    emergencyShutdown();
    Serial.printf("[안전] 과열 감지! (%.2f °C)\n", temperature);
    return;
  }
  
  // ─────────────────── 압력 기반 제어 ──────────────────────
  if (currentMode == MODE_MANUAL) {
    // 수동 모드는 별도 처리
    return;
  }
  
  if (currentMode == MODE_AUTO) {
    // 자동 모드: 히스테리시스 제어
    if (pressure > TARGET_PRESSURE + PRESSURE_HYSTERESIS) {
      // 압력이 목표보다 높음 - 펌프 가동
      if (!pumpActive) {
        controlPump(true, 200);  // PWM 200으로 시작
      }
    } 
    else if (pressure < TARGET_PRESSURE - PRESSURE_HYSTERESIS) {
      // 압력이 목표보다 낮음 - 펌프 정지
      if (pumpActive) {
        controlPump(false);
      }
    }
    // 히스테리시스 범위 내에서는 현재 상태 유지
  }
  
  // ─────────────────── 전류 경고 ───────────────────────────
  if (current > CURRENT_THRESHOLD_WARNING) {
    Serial.printf("[경고] 전류 높음: %.2f A\n", current);
  }
  
  // ─────────────────── 온도 경고 ───────────────────────────
  if (temperature > TEMP_THRESHOLD_WARNING) {
    Serial.printf("[경고] 온도 높음: %.2f °C\n", temperature);
  }
}

// ================================================================
// 초기화 함수
// ================================================================

void initControl() {
  // 핀 모드 설정은 main.cpp에서 수행
  // 여기서는 상태 초기화만
  
  pumpActive = false;
  valveActive = false;
  pumpPWM = 0;
  
  // 모든 출력 OFF
  ledcWrite(PWM_CHANNEL_PUMP, 0);
  digitalWrite(PIN_VALVE, LOW);
  digitalWrite(PIN_12V_MAIN, LOW);
  digitalWrite(PIN_12V_EMERGENCY, LOW);
  
  Serial.println("[Control] 초기화 완료");
  Serial.println("[Control] - 안전 인터락 활성화");
  Serial.println("[Control] - PWM 제어 준비");
  Serial.println("[Control] - SensorManager 통합");
}

// ================================================================
// 상태 출력 (디버깅용)
// ================================================================

void printControlStatus() {
  Serial.println("\n=== 제어 상태 ===");
  Serial.printf("펌프: %s", pumpActive ? "ON" : "OFF");
  if (pumpActive) {
    Serial.printf(" (PWM: %d, %.1f%%)\n", pumpPWM, (pumpPWM / 255.0) * 100);
  } else {
    Serial.println();
  }
  Serial.printf("밸브: %s\n", valveActive ? "ON" : "OFF");
  Serial.printf("12V 메인: %s\n", digitalRead(PIN_12V_MAIN) ? "ON" : "OFF");
  Serial.printf("12V 비상: %s\n", digitalRead(PIN_12V_EMERGENCY) ? "ON" : "OFF");
  
  // 센서 상태 (SensorManager 사용)
  Serial.println("\n--- 센서 상태 ---");
  Serial.printf("압력: %.2f kPa\n", sensorManager.getPressure());
  Serial.printf("전류: %.2f A\n", sensorManager.getCurrent());
  Serial.printf("온도: %.2f °C\n", sensorManager.getTemperature());
  Serial.printf("비상정지: %s\n", sensorManager.getEmergencyStop() ? "눌림" : "정상");
  Serial.printf("리밋SW: %s\n", sensorManager.getLimitSwitch() ? "ON" : "OFF");
  Serial.println("==================\n");
}

// ================================================================
// 안전 검사 함수
// ================================================================

bool performSafetyCheck() {
  bool safe = true;
  
  // 비상 정지 버튼 체크
  if (sensorManager.getEmergencyStop()) {
    Serial.println("[안전] 비상 정지 버튼 활성화!");
    safe = false;
  }
  
  // 과전류 체크
  if (sensorManager.getCurrent() > CURRENT_THRESHOLD_CRITICAL) {
    Serial.printf("[안전] 과전류: %.2f A\n", sensorManager.getCurrent());
    safe = false;
  }
  
  // 과열 체크
  if (sensorManager.getTemperature() > TEMP_THRESHOLD_SHUTDOWN) {
    Serial.printf("[안전] 과열: %.2f °C\n", sensorManager.getTemperature());
    safe = false;
  }
  
  // 인터락 체크
  if (pumpActive && valveActive) {
    Serial.println("[안전] 인터락 위반: 펌프/밸브 동시 동작!");
    safe = false;
  }
  
  if (!safe) {
    emergencyShutdown();
  }
  
  return safe;
}

// ================================================================
// PWM 조절 함수 (부드러운 가속/감속)
// ================================================================

void setPumpPWM(uint8_t targetPWM, uint16_t rampTimeMs) {
  if (!pumpActive) {
    Serial.println("[제어] 펌프가 꺼져있습니다. 먼저 켜주세요.");
    return;
  }
  
  targetPWM = constrain(targetPWM, PWM_MIN, PWM_MAX);
  
  if (rampTimeMs == 0) {
    // 즉시 변경
    pumpPWM = targetPWM;
    ledcWrite(PWM_CHANNEL_PUMP, pumpPWM);
    Serial.printf("[제어] PWM 즉시 변경: %d\n", pumpPWM);
  } else {
    // 부드러운 변경
    int step = (targetPWM > pumpPWM) ? 1 : -1;
    uint16_t delayMs = rampTimeMs / abs(targetPWM - pumpPWM);
    delayMs = max(delayMs, (uint16_t)10);  // 최소 10ms
    
    Serial.printf("[제어] PWM 변경: %d → %d (%dms)\n", pumpPWM, targetPWM, rampTimeMs);
    
    while (pumpPWM != targetPWM) {
      pumpPWM += step;
      ledcWrite(PWM_CHANNEL_PUMP, pumpPWM);
      vTaskDelay(pdMS_TO_TICKS(delayMs));
      
      // 안전 체크
      if (sensorManager.getEmergencyStop()) {
        emergencyShutdown();
        return;
      }
    }
    
    Serial.printf("[제어] PWM 변경 완료: %d\n", pumpPWM);
  }
}

// ================================================================
// 간편 제어 함수 (호환성)
// ================================================================

void setPump(bool on) {
  if (on) {
    controlPump(true, 200);  // 기본 PWM 200
  } else {
    controlPump(false);
  }
}

void setValve(bool on) {
  controlValve(on);
}
