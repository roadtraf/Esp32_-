// ================================================================
// UIComponents.h - 재사용 가능한 UI 컴포넌트
// ================================================================
#pragma once

#include <Arduino.h>
#include "LovyanGFX_Config.hpp"
#include "UITheme.h"

extern LGFX tft;

namespace UIComponents {
    
    // ================================================================
    // 헤더 컴포넌트
    // ================================================================
    void drawHeader(const char* title, bool showManagerBadge = true);
    
    // ================================================================
    // 카드 컴포넌트
    // ================================================================
    struct CardConfig {
        int16_t x, y, w, h;
        uint16_t bgColor = UITheme::COLOR_BG_CARD;
        uint16_t borderColor = UITheme::COLOR_BORDER;
        bool elevated = false;
    };
    
    void drawCard(const CardConfig& config);
    
    // ================================================================
    // 버튼 컴포넌트
    // ================================================================
    enum ButtonStyle {
        BTN_PRIMARY,
        BTN_SECONDARY,
        BTN_SUCCESS,
        BTN_DANGER,
        BTN_OUTLINE
    };
    
    struct ButtonConfig {
        int16_t x, y, w, h;
        const char* label;
        ButtonStyle style = BTN_PRIMARY;
        bool enabled = true;
    };
    
    void drawButton(const ButtonConfig& config);
    bool isButtonPressed(const ButtonConfig& config, uint16_t touchX, uint16_t touchY);
    
    // ================================================================
    // 값 표시 컴포넌트 (라벨 + 값)
    // ================================================================
    struct ValueDisplayConfig {
        int16_t x, y;
        const char* label;
        const char* value;
        const char* unit;
        uint16_t valueColor = UITheme::COLOR_TEXT_PRIMARY;
    };
    
    void drawValueDisplay(const ValueDisplayConfig& config);
    
    // ================================================================
    // 상태 배지
    // ================================================================
    enum BadgeType {
        BADGE_SUCCESS,
        BADGE_WARNING,
        BADGE_DANGER,
        BADGE_INFO
    };
    
    void drawBadge(int16_t x, int16_t y, const char* text, BadgeType type);
    
    // ================================================================
    // 프로그레스 바
    // ================================================================
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, float percentage, uint16_t color = UITheme::COLOR_PRIMARY);
    
    // ================================================================
    // 아이콘 (간단한 기하학 아이콘)
    // ================================================================
    void drawIconHome(int16_t x, int16_t y, uint16_t color);
    void drawIconSettings(int16_t x, int16_t y, uint16_t color);
    void drawIconBack(int16_t x, int16_t y, uint16_t color);
    void drawIconWarning(int16_t x, int16_t y, uint16_t color);
    void drawIconCheck(int16_t x, int16_t y, uint16_t color);
    
    // ================================================================
    // 구분선
    // ================================================================
    void drawDivider(int16_t x, int16_t y, int16_t w);
    
    // ================================================================
    // 네비게이션 바 (하단)
    // ================================================================
    struct NavButton {
        const char* label;
        ButtonStyle style;
        bool enabled;
    };
    
    void drawNavBar(NavButton buttons[], uint8_t count);
}
