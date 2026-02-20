// ================================================================
// ManagerUI.h - 관리자 전용 UI 컴포넌트
// Phase 2: 관리자 기능 구현
// ================================================================
#pragma once

#include <Arduino.h>
#include "../include/LovyanGFX_Config.hpp"
#include "../include/SystemController.h"
#include "UI_AccessControl.h"

// ================================================================
// 관리자 UI 표시
// ================================================================

// 화면 상단에 관리자 모드 표시
void drawManagerBadge();

// 자동 로그아웃 타이머 표시
void drawLogoutTimer();

// 관리자 메뉴 오버레이
void drawManagerMenu();

// 관리자 설정 화면
void drawManagerSettingsScreen();

// 권한 확인 팝업
bool showPermissionDialog(const char* action);

// 비밀번호 입력 다이얼로그 (TFT 키패드)
bool showPasswordDialog(SystemMode targetMode);

// ================================================================
// 관리자 전용 화면 접근 제어
// ================================================================

// 권한 없음 알림
void showAccessDenied(const char* screenName);

// ================================================================
// 관리자 통계 화면
// ================================================================

// 고급 통계 화면
void drawAdvancedStatistics();

// 센서 히스토리 그래프
void drawSensorHistory();

// 시스템 진단 화면
void drawSystemDiagnostics();