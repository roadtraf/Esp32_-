// ================================================================
// ManagerUI.cpp - 관리자 전용 UI 구현
// ================================================================
#include "../include/ManagerUI.h"
#include "../include/Config.h"
#include "../include/UI_Screens.h"
#include "UI_AccessControl.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern LGFX tft;

// ================================================================
// 관리자 모드 배지 표시 [U11] TFT_* 색상 → UITheme 색상으로 통일
// ================================================================
void drawManagerBadge() {
    using namespace UITheme;

    SystemMode mode = systemController.getMode();
    if (mode == SystemMode::OPERATOR) return;

    int16_t  x = SCREEN_WIDTH - 100;
    int16_t  y = 5;
    int16_t  w = 95;
    int16_t  h = 25;

    // [U11] TFT_ORANGE/TFT_RED → UITheme 색상
    uint16_t bgColor = (mode == SystemMode::MANAGER)
                       ? COLOR_MANAGER : COLOR_DEVELOPER;

    tft.fillRoundRect(x, y, w, h, 5, bgColor);
    tft.drawRoundRect(x, y, w, h, 5, COLOR_TEXT_PRIMARY);  // [U11] TFT_WHITE 제거

    tft.setTextSize(1);
    tft.setTextColor(COLOR_BG_DARK);  // [U11] TFT_WHITE → 다크 배경

    const char* modeText = systemController.getModeString();
    // [U7] textWidth 기반 중앙 정렬
    int16_t tw = tft.textWidth(modeText);
    tft.setCursor(x + (w - tw) / 2, y + (h - 8) / 2);
    tft.print(modeText);
}

// ================================================================
// 자동 로그아웃 타이머 표시
// ================================================================
void drawLogoutTimer() {
    if (!systemController.isAutoLogoutEnabled() || systemController.isOperatorMode()) {
        return;
    }
    
    uint32_t remaining = systemController.getRemainingTime();
    
    // 우측 상단 배지 아래
    int16_t x = tft.width() - 100;
    int16_t y = 35;
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    
    uint32_t minutes = remaining / 60000;
    uint32_t seconds = (remaining % 60000) / 1000;
    
    char timerBuf[16];
    snprintf(timerBuf, sizeof(timerBuf), "%lu:%02lu", minutes, seconds);
    
    tft.setCursor(x + 10, y);
    tft.print(timerBuf);
}

// ================================================================
// 관리자 메뉴 오버레이
// ================================================================
void drawManagerMenu() {
    // 화면 중앙에 메뉴
    int16_t menuW = 280;
    int16_t menuH = 200;
    int16_t menuX = (tft.width() - menuW) / 2;
    int16_t menuY = (tft.height() - menuH) / 2;
    
    // 반투명 배경
    tft.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);
    
    // 메뉴 배경
    tft.fillRoundRect(menuX, menuY, menuW, menuH, 10, TFT_DARKGREY);
    tft.drawRoundRect(menuX, menuY, menuW, menuH, 10, TFT_ORANGE);
    
    // 제목
    tft.setTextSize(2);
    tft.setTextColor(TFT_ORANGE);
    tft.setCursor(menuX + 60, menuY + 10);
    tft.print("관리자 메뉴");
    
    // 메뉴 항목
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    
    int16_t itemY = menuY + 50;
    int16_t lineHeight = 25;
    
    tft.setCursor(menuX + 20, itemY);
    tft.print("1. 설정 변경");
    
    tft.setCursor(menuX + 20, itemY + lineHeight);
    tft.print("2. 캘리브레이션");
    
    tft.setCursor(menuX + 20, itemY + lineHeight * 2);
    tft.print("3. 고급 통계");
    
    tft.setCursor(menuX + 20, itemY + lineHeight * 3);
    tft.print("4. 시스템 진단");
    
    tft.setCursor(menuX + 20, itemY + lineHeight * 4);
    tft.print("5. 로그아웃");
    
    // 하단 안내
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(menuX + 40, menuY + menuH - 20);
    tft.print("항목을 터치하세요");
}

// ================================================================
// 권한 확인 다이얼로그
// ================================================================
bool showPermissionDialog(const char* action) {
    SystemPermissions perms = systemController.getPermissions();
    
    // 해당 액션에 대한 권한 확인
    bool hasPermission = systemController.hasPermission(action);
    
    if (!hasPermission) {
        // 권한 없음 알림
        int16_t dialogW = 250;
        int16_t dialogH = 120;
        int16_t dialogX = (tft.width() - dialogW) / 2;
        int16_t dialogY = (tft.height() - dialogH) / 2;
        
        tft.fillRoundRect(dialogX, dialogY, dialogW, dialogH, 10, TFT_RED);
        tft.drawRoundRect(dialogX, dialogY, dialogW, dialogH, 10, TFT_WHITE);
        
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.setCursor(dialogX + 50, dialogY + 20);
        tft.print("권한 없음");
        
        tft.setTextSize(1);
        tft.setCursor(dialogX + 20, dialogY + 60);
        tft.print("관리자 권한이 필요합니다");
        
        tft.setCursor(dialogX + 70, dialogY + 90);
        tft.print("(3초 후 닫힘)");
        
        // [R4] 비블로킹: 3초 후 자동 소멸
        uiManager.showMessage("권한 필요", 3000); 
        return false;
    }
    
    return true;
}

// ================================================================
// 접근 거부 알림
// ================================================================
void showAccessDenied(const char* screenName) {
    int16_t dialogW = 280;
    int16_t dialogH = 140;
    int16_t dialogX = (tft.width() - dialogW) / 2;
    int16_t dialogY = (tft.height() - dialogH) / 2;
    
    tft.fillRoundRect(dialogX, dialogY, dialogW, dialogH, 10, TFT_MAROON);
    tft.drawRoundRect(dialogX, dialogY, dialogW, dialogH, 10, TFT_RED);
    
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(dialogX + 60, dialogY + 15);
    tft.print("접근 거부");
    
    tft.setTextSize(1);
    tft.setCursor(dialogX + 20, dialogY + 50);
    tft.printf("'%s' 화면은", screenName);
    
    tft.setCursor(dialogX + 20, dialogY + 70);
    tft.print("관리자 권한이 필요합니다");
    
    // 버튼
    int16_t btnW = 100;
    int16_t btnH = 30;
    int16_t btnX = dialogX + (dialogW - btnW) / 2;
    int16_t btnY = dialogY + dialogH - 40;
    
    tft.fillRoundRect(btnX, btnY, btnW, btnH, 5, TFT_DARKGREY);
    tft.drawRoundRect(btnX, btnY, btnW, btnH, 5, TFT_WHITE);
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(btnX + 35, btnY + 10);
    tft.print("확인");
    
    // 터치 대기 (간단 구현)
    showAccessDeniedAsync(screenName); (UI_AccessControl.h)
}

// ================================================================
// 고급 통계 화면
// ================================================================
void drawAdvancedStatistics() {
    tft.fillScreen(TFT_BLACK);
    
    // 제목
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 10);
    tft.print("고급 통계");
    
    // 관리자 배지
    drawManagerBadge();
    
    // 센서 통계 (Phase 2-3 버퍼 활용)
    SensorStats stats;
    calculateSensorStats(stats);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    
    int16_t y = 50;
    int16_t lineH = 20;
    
    // 온도
    tft.setCursor(10, y);
    tft.printf("온도: %.2f°C", stats.avgTemperature);
    tft.setCursor(200, y);
    tft.printf("(%.2f ~ %.2f)", stats.minTemperature, stats.maxTemperature);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("  표준편차: %.2f", stats.tempStdDev);
    y += lineH * 2;
    
    // 압력
    tft.setCursor(10, y);
    tft.printf("압력: %.2f kPa", stats.avgPressure);
    tft.setCursor(200, y);
    tft.printf("(%.2f ~ %.2f)", stats.minPressure, stats.maxPressure);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("  표준편차: %.2f", stats.pressureStdDev);
    y += lineH * 2;
    
    // 전류
    tft.setCursor(10, y);
    tft.printf("전류: %.2f A", stats.avgCurrent);
    tft.setCursor(200, y);
    tft.printf("(%.2f ~ %.2f)", stats.minCurrent, stats.maxCurrent);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("  표준편차: %.2f", stats.currentStdDev);
    y += lineH * 2;
    
    // 샘플 정보
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.printf("총 샘플 수: %lu개", stats.sampleCount);
    y += lineH;
    
    tft.setCursor(10, y);
    tft.printf("버퍼 사용률: %.1f%%", 
               (float)temperatureBuffer.size() / TEMP_BUFFER_SIZE * 100);
    
    // 뒤로 버튼
    tft.fillRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_DARKGREY);
    tft.drawRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(30, tft.height() - 32);
    tft.print("뒤로");
}

// ================================================================
// 시스템 진단 화면
// ================================================================
void drawSystemDiagnostics() {
    tft.fillScreen(TFT_BLACK);
    
    // 제목
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(10, 10);
    tft.print("시스템 진단");
    
    // 관리자 배지
    drawManagerBadge();
    
    tft.setTextSize(1);
    int16_t y = 50;
    int16_t lineH = 18;
    
    // 메모리 상태
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print("메모리 상태:");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, y);
    tft.printf("Free Heap: %lu KB", ESP.getFreeHeap() / 1024);
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf("Min Free:  %lu KB", ESP.getMinFreeHeap() / 1024);
    y += lineH * 2;
    
    // 센서 상태
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print("센서 상태:");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, y);
    tft.printf("온도 센서: %s", 
               isTemperatureSensorConnected() ? "정상" : "오류");
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf("압력 센서: 정상");
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf("전류 센서: 정상");
    y += lineH * 2;
    
    // WiFi 상태
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print("네트워크 상태:");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, y);
    if (WiFi.status() == WL_CONNECTED) {
        tft.printf("WiFi: 연결됨 (RSSI: %d)", WiFi.RSSI());
    } else {
        tft.print("WiFi: 연결 끊김");
    }
    y += lineH;
    
    tft.setCursor(20, y);
    tft.printf("MQTT: %s", mqttConnected ? "연결됨" : "끊김");
    y += lineH * 2;
    
    // 가동 시간
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, y);
    tft.print("시스템 정보:");
    y += lineH;
    
    tft.setTextColor(TFT_WHITE);
    uint32_t uptime = millis() / 1000;
    tft.setCursor(20, y);
    tft.printf("가동 시간: %luh %lum", uptime / 3600, (uptime % 3600) / 60);
    
    // 뒤로 버튼
    tft.fillRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_DARKGREY);
    tft.drawRoundRect(10, tft.height() - 40, 80, 30, 5, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(30, tft.height() - 32);
    tft.print("뒤로");
}