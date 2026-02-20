// ================================================================
// UI_AccessControl.h - PIN 입력 화면 및 접근 제어 통합 헤더
// [U2] canAccessScreen() 중복 정의 제거 → 단일 구현
// [U4] PIN 입력 UI 신규 구현
// ================================================================
#pragma once

#include <Arduino.h>
#include "../include/Config.h"
#include "../include/UITheme.h"

// ================================================================
// [U2] canAccessScreen() 단일 선언 (ManagerUI.h, UI_Screens.h 중복 제거)
// ================================================================
bool canAccessScreen(ScreenType screen);

// ================================================================
// [U4] PIN 입력 화면
// ================================================================

// PIN 입력 결과 콜백 타입
using PinResultCallback = void(*)(bool success, SystemMode targetMode);

// PIN 입력 화면 표시
// targetMode: MANAGER or DEVELOPER
// onResult  : 성공/실패 콜백
void showPinInputScreen(SystemMode targetMode, PinResultCallback onResult);

// PIN 입력 화면 그리기 (UIManager 내부에서 호출)
void drawPinInputScreen();

// PIN 입력 화면 터치 처리
void handlePinInputTouch(uint16_t x, uint16_t y);

// PIN 화면 활성 여부
bool isPinScreenActive();

// 키보드 입력 처리 (PIN 화면 전용)
void handleKeyboardOnPinScreen(uint8_t key);

// ================================================================
// [U3] 비동기 알림 다이얼로그 (vTaskDelay 제거)
// ================================================================

// 비동기 접근 거부 알림 (블로킹 없음)
// 내부적으로 UIManager.showMessage() 활용
void showAccessDeniedAsync(const char* screenName);

// 하위 호환: 기존 showAccessDenied() → 비동기 버전으로 리디렉트
inline void showAccessDenied(const char* screenName) {
    showAccessDeniedAsync(screenName);
}
