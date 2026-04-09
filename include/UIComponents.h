// ================================================================
// UIComponents.h -   UI 
// ================================================================
#pragma once

#include <Arduino.h>
#include "GFX_Wrapper.hpp"
#include "UITheme.h"

extern TFT_GFX tft;

namespace UIComponents {
    
    // ================================================================
    //  
    // ================================================================
    void drawHeader(const char* title, bool showManagerBadge = true);
    
    // ================================================================
    //  
    // ================================================================
    struct CardConfig {
    int16_t  x           = 0;
    int16_t  y           = 0;
    int16_t  w           = 0;
    int16_t  h           = 0;
    uint16_t bgColor     = UITheme::COLOR_BG_CARD;
    uint16_t borderColor = UITheme::COLOR_BORDER;
    bool     elevated    = false;
};
    
    void drawCard(const CardConfig& config);
    
    // ================================================================
    //  
    // ================================================================
    enum ButtonStyle {
        BTN_PRIMARY,
        BTN_SECONDARY,
        BTN_SUCCESS,
        BTN_DANGER,
        BTN_OUTLINE
    };
    
    struct ButtonConfig {
    int16_t     x       = 0;
    int16_t     y       = 0;
    int16_t     w       = 0;
    int16_t     h       = 0;
    const char* label   = "";
    ButtonStyle style   = BTN_PRIMARY;
    bool        enabled = true;
};
    
    void drawButton(const ButtonConfig& config);
    bool isButtonPressed(const ButtonConfig& config, uint16_t touchX, uint16_t touchY);
    
    // ================================================================
    //    ( + )
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
    //  
    // ================================================================
    enum BadgeType {
        BADGE_SUCCESS,
        BADGE_WARNING,
        BADGE_DANGER,
        BADGE_INFO
    };
    
    void drawBadge(int16_t x, int16_t y, const char* text, BadgeType type);
    
    // ================================================================
    //  
    // ================================================================
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, float percentage, uint16_t color = UITheme::COLOR_PRIMARY);
    
    // ================================================================
    //  (  )
    // ================================================================
    void drawIconHome(int16_t x, int16_t y, uint16_t color);
    void drawIconSettings(int16_t x, int16_t y, uint16_t color);
    void drawIconBack(int16_t x, int16_t y, uint16_t color);
    void drawIconWarning(int16_t x, int16_t y, uint16_t color);
    void drawIconCheck(int16_t x, int16_t y, uint16_t color);
    
    // ================================================================
    // 
    // ================================================================
    void drawDivider(int16_t x, int16_t y, int16_t w);
    
    // ================================================================
    //   ()
    // ================================================================
    struct NavButton {
        const char* label;
        ButtonStyle style;
        bool enabled;
    };
    
    void drawNavBar(NavButton buttons[], uint8_t count);
}
