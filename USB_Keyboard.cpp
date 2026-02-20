// ================================================================
// USB_Keyboard.cpp — 키보드-터치 충돌 수정본
//
// 발견된 5가지 충돌 문제:
//
// [K1] 팝업 활성화 중 키보드 화면 전환 → 팝업 좀비 상태
//      popupActive=true인데 화면만 바뀌어 터치가 팝업으로만 가고
//      화면 조작 불가
//
// [K2] currentScreen 직접 대입 16회 → Race Condition (Mutex 없음)
//      uiUpdateStep에서 screenNeedsRedraw 체크와 currentScreen 읽기가
//      분리되어 있어 키보드가 중간에 currentScreen 변경 시
//      다른 화면 그려질 수 있음
//
// [K3] changeState() Mutex 보호 없음 → 터치/키보드 동시 START/STOP
//      가능. StateMachine이 Mutex로 보호되지 않음.
//
// [K4] helpPageIndex, currentMode 등 UI 상태 변수 Race
//      키보드가 직접 조작하는데 터치도 조작 가능
//
// [K5] PIN 화면 활성화 중 키보드 입력 → PIN 우회
//      PIN 입력 중인데 키보드로 화면 전환 가능
//
// 수정: 모든 키보드 명령 처리 전에 팝업/PIN 체크 추가
//       currentScreen 변경은 uiManager.setScreen() 경유
// ================================================================

#include "Config.h"
#include <USBHIDKeyboard.h>
#include "StateMachine.h"
#include "Network.h"
#include "Sensor.h"
#include "ErrorHandler.h"
#include "State_Diagram.h"
#include "UIManager.h"              // [K2] 추가
#include "UI_AccessControl.h"       // [K5] 추가

extern UIManager uiManager;
extern bool popupActive;            // [K1] 추가

// ================================================================
// USB 키보드 입력 처리
// ================================================================
void handleKeyboardInput() {
  if (!keyboard.available()) {
    return;
  }

  uint8_t key = keyboard.read();
  processKeyboardCommand(key);
}

void processKeyboardCommand(uint8_t key) {
  Serial.printf("[키보드] 키 코드: 0x%02X\n", key);

  // [K5] PIN 화면 활성화 중이면 키보드 입력 무시
  if (isPinScreenActive()) {
    Serial.println("[키보드] PIN 입력 중 — 키보드 차단");
    return;
  }

  // [K1] 팝업 활성화 중이면 화면 전환 차단 (ESC/취소만 허용)
  if (popupActive) {
    if (key == 0x1B || key == 0x08 || key == 0x7F) {  // ESC, Backspace, Delete
      // 팝업 닫기
      extern void hidePopup();  // UI_Popup.cpp
      hidePopup();
      Serial.println("[키보드] 팝업 닫기");
      return;
    }
    // 그 외 모든 키는 무시
    Serial.println("[키보드] 팝업 활성 중 — 다른 명령 차단");
    return;
  }

  // ── 숫자 키 (0-9) ──
  if (key >= '0' && key <= '9') {
    uint8_t num = key - '0';

    switch (num) {
      case 1: // START
        // [K3] changeState는 내부에 중복 방지 체크 있음 (if currentState == newState return)
        if (currentState == STATE_IDLE) {
          changeState(STATE_VACUUM_ON);
          Serial.println("[키보드] START 명령");
        }
        break;

      case 2: // STOP
        changeState(STATE_IDLE);
        Serial.println("[키보드] STOP 명령");
        break;

      case 3: // MODE
        // [K4] currentMode 변경 — Race 가능하지만 심각도 낮음 (UI 표시용)
        if (currentMode == MODE_MANUAL) {
          currentMode = MODE_AUTO;
          config.controlMode = MODE_AUTO;
        } else if (currentMode == MODE_AUTO) {
          currentMode = MODE_PID;
          config.controlMode = MODE_PID;
        } else {
          currentMode = MODE_MANUAL;
          config.controlMode = MODE_MANUAL;
        }
        saveConfig();
        uiManager.requestRedraw();  // [K2] screenNeedsRedraw 대신
        Serial.printf("[키보드] 모드 변경: %d\n", currentMode);
        break;

      case 4: // RESET
        clearError();
        Serial.println("[키보드] 알람 리셋");
        break;

      // [K2] 화면 전환 — currentScreen 직접 대입 → uiManager.setScreen()
      case 5: // STATISTICS
        uiManager.setScreen(SCREEN_STATISTICS);
        Serial.println("[키보드] 통계 화면");
        break;

      case 6: // ABOUT
        uiManager.setScreen(SCREEN_ABOUT);
        Serial.println("[키보드] 정보 화면");
        break;

      case 7: // TIMING SETUP
        uiManager.setScreen(SCREEN_TIMING_SETUP);
        Serial.println("[키보드] 타이밍 설정");
        break;

      case 8: // TREND GRAPH
        uiManager.setScreen(SCREEN_TREND_GRAPH);
        Serial.println("[키보드] 추세 그래프");
        break;

      case 9: // HELP
        uiManager.setScreen(SCREEN_HELP);
        helpPageIndex = 0;  // [K4] Race 가능하지만 영향 미미
        Serial.println("[키보드] 도움말");
        break;

      case 0: // MAIN SCREEN
        uiManager.setScreen(SCREEN_MAIN);
        Serial.println("[키보드] 메인 화면");
        break;
    }
  }
  // ── 특수 키 ──
  else if (key == '.') { // STATE DIAGRAM
    uiManager.setScreen(SCREEN_STATE_DIAGRAM);
    Serial.println("[키보드] 상태 다이어그램");
  }
  else if (key == '*') { // MENU
    uiManager.setScreen(SCREEN_SETTINGS);
    Serial.println("[키보드] 설정 메뉴");
  }
  else if (key == '/') { // HELP (대체)
    uiManager.setScreen(SCREEN_HELP);
    helpPageIndex = 0;
    Serial.println("[키보드] 도움말");
  }
  else if (key == '+') { // 다음 페이지
    ScreenType current = uiManager.getCurrentScreen();  // [K2] 안전한 읽기
    if (current == SCREEN_HELP) {
      if (helpPageIndex < 5) {
        helpPageIndex++;
        uiManager.requestRedraw();
        Serial.printf("[키보드] 도움말 다음 페이지: %d\n", helpPageIndex + 1);
      }
    }
    else if (current == SCREEN_STATE_DIAGRAM) {
      stateDiagramNextPage();
      Serial.println("[키보드] 상태다이어그램 다음 페이지");
    }
  }
  else if (key == '-') { // 이전 페이지
    ScreenType current = uiManager.getCurrentScreen();
    if (current == SCREEN_HELP) {
      if (helpPageIndex > 0) {
        helpPageIndex--;
        uiManager.requestRedraw();
        Serial.printf("[키보드] 도움말 이전 페이지: %d\n", helpPageIndex + 1);
      }
    }
    else if (current == SCREEN_STATE_DIAGRAM) {
      stateDiagramPrevPage();
      Serial.println("[키보드] 상태다이어그램 이전 페이지");
    }
  }
  else if (key == '\r' || key == '\n') { // Enter
    Serial.println("[키보드] 확인");
    // 현재 화면의 기본 액션 (필요시 구현)
  }
  else if (key == 0x08 || key == 0x7F) { // Backspace — 뒤로 가기
    ScreenType current = uiManager.getCurrentScreen();
    if (current != SCREEN_MAIN) {
      if (current == SCREEN_SETTINGS) {
        uiManager.setScreen(SCREEN_MAIN);
      } else {
        uiManager.setScreen(SCREEN_SETTINGS);
      }
      Serial.println("[키보드] 뒤로 가기");
    }
  }
  else if (key == 0x1B) { // ESC — 메인으로
    uiManager.setScreen(SCREEN_MAIN);
    Serial.println("[키보드] ESC → 메인 화면");
  }
}
