// ================================================================
// UI_Screen_VoiceSettings.cpp -   
// ================================================================
#include "UIComponents.h"
#include "Config.h"
#include "UI_AccessControl.h"

// Config.h   
#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"
#endif

using namespace UIComponents;
using namespace UITheme;

void drawVoiceSettingsScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    
    //   
    drawHeader(" ");
    
    //  
    if (!canAccessScreen(SCREEN_VOICE_SETTINGS)) {
        showAccessDenied(" ");
        NavButton navButtons[] = {{"", BTN_OUTLINE, true}};
        drawNavBar(navButtons, 1);
        return;
    }
    
    #ifdef ENABLE_VOICE_ALERTS
    
    //    
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    CardConfig statusCard = {
        .x = SPACING_SM,
        .y = startY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 60,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(statusCard);
    
    bool voiceOnline = voiceAlert.isOnline();
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING);
    tft.print("DFPlayer Mini ");
    
    drawBadge(statusCard.x + statusCard.w - 70, statusCard.y + CARD_PADDING,
              voiceOnline ? "" : "",
              voiceOnline ? BADGE_SUCCESS : BADGE_DANGER);
    
    if (voiceOnline) {
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 20);
        tft.printf(" : %d", voiceAlert.getFileCount());
    } else {
        tft.setTextSize(1);
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(statusCard.x + CARD_PADDING, statusCard.y + CARD_PADDING + 20);
        tft.print("  ");
    }
    
    //    
    int16_t langY = statusCard.y + statusCard.h + SPACING_SM;
    
    CardConfig langCard = {
        .x = SPACING_SM,
        .y = langY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 55,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(langCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(langCard.x + CARD_PADDING, langCard.y + CARD_PADDING);
    tft.print(" ");
    
    Language currentVoiceLang = voiceAlert.getLanguage();
    const char* langText = (currentVoiceLang == LANG_KO) ? "" : "English";
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(langCard.x + CARD_PADDING, langCard.y + CARD_PADDING + 20);
    tft.print(langText);
    
    //   
    ButtonConfig langBtn = {
        .x = (int16_t)(langCard.x + langCard.w - 80),
        .y = (int16_t)(langCard.y + CARD_PADDING + 10),
        .w = 70,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = voiceOnline
    };
    drawButton(langBtn);
    
    //    
    int16_t volY = langCard.y + langCard.h + SPACING_SM;
    
    CardConfig volCard = {
        .x = SPACING_SM,
        .y = volY,
        .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
        .h = 75,
        .bgColor = COLOR_BG_CARD
    };
    drawCard(volCard);
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(volCard.x + CARD_PADDING, volCard.y + CARD_PADDING);
    tft.print("");
    
    uint8_t currentVolume = voiceAlert.getVolume();
    
    //  
    int16_t sliderX = volCard.x + CARD_PADDING;
    int16_t sliderY = volCard.y + CARD_PADDING + 25;
    int16_t sliderW = volCard.w - CARD_PADDING * 2;
    
    drawProgressBar(sliderX, sliderY, sliderW, 20, 
                    (currentVolume / 30.0f) * 100, COLOR_PRIMARY);
    
    //   
    ButtonConfig volDownBtn = {
        .x = (int16_t)(volCard.x + CARD_PADDING),
        .y = (int16_t)(volCard.y + volCard.h - 32),
        .w = 60,
        .h = 24,
        .label = "-",
        .style = BTN_SECONDARY,
        .enabled = voiceOnline && currentVolume > 0
    };
    drawButton(volDownBtn);
    
    ButtonConfig volUpBtn = {
        .x = (int16_t)(volCard.x + volCard.w - 70),
        .y = (int16_t)(volCard.y + volCard.h - 32),
        .w = 60,
        .h = 24,
        .label = "+",
        .style = BTN_SECONDARY,
        .enabled = voiceOnline && currentVolume < 30
    };
    drawButton(volUpBtn);
    
    //    
    int16_t testY = volCard.y + volCard.h + SPACING_SM;
    
    struct TestVoice {
        const char* label;
        uint8_t voiceId;
    };
    
    TestVoice tests[] = {
        {" ", VOICE_READY},
        {"", VOICE_START},
        {"", VOICE_STOP},
        {"", VOICE_WARNING}
    };
    
    int16_t testBtnW = (SCREEN_WIDTH - SPACING_SM * 5) / 4;
    int16_t testBtnH = 32;
    
    for (int i = 0; i < 4; i++) {
        int16_t btnX = SPACING_SM + i * (testBtnW + SPACING_SM);
        
        ButtonConfig testBtn = {
            .x = btnX,
            .y = testY,
            .w = testBtnW,
            .h = testBtnH,
            .label = tests[i].label,
            .style = BTN_OUTLINE,
            .enabled = voiceOnline
        };
        drawButton(testBtn);
    }
    
    #else
    
    //   
    int16_t msgY = SCREEN_HEIGHT / 2 - 30;
    
    drawIconWarning(SCREEN_WIDTH / 2 - 8, msgY, COLOR_WARNING);
    
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(80, msgY + 30);
    tft.print(" ");
    
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setCursor(60, msgY + 55);
    tft.print("Config.h ");
    
    #endif
    
    //    
    NavButton navButtons[] = {
        {"", BTN_OUTLINE, true}
    };
    drawNavBar(navButtons, 1);
}

void handleVoiceSettingsTouch(uint16_t x, uint16_t y) {
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    #ifdef ENABLE_VOICE_ALERTS
    
    bool voiceOnline = voiceAlert.isOnline();
    int16_t startY = HEADER_HEIGHT + SPACING_SM;
    
    //   
    int16_t langY = startY + 60 + SPACING_SM;
    
    ButtonConfig langBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 80),
        .y = (int16_t)(langY + CARD_PADDING + 10),
        .w = 70,
        .h = 28,
        .label = "",
        .style = BTN_PRIMARY,
        .enabled = voiceOnline
    };
    
    if (isButtonPressed(langBtn, x, y) && voiceOnline) {
        Language newLang = (voiceAlert.getLanguage() == LANG_KO) 
                          ? LANG_EN 
                          : LANG_KO;
        voiceAlert.setLanguage(newLang);
        voiceAlert.playSystem(VOICE_READY);
        screenNeedsRedraw = true;
        return;
    }
    
    //   
    int16_t volY = langY + 55 + SPACING_SM;
    uint8_t currentVolume = voiceAlert.getVolume();
    
    ButtonConfig volDownBtn = {
        .x = (int16_t)(SPACING_SM + CARD_PADDING),
        .y = (int16_t)(volY + 75 - 32),
        .w = 60,
        .h = 24,
        .label = "-",
        .style = BTN_SECONDARY,
        .enabled = voiceOnline && currentVolume > 0
    };
    
    if (isButtonPressed(volDownBtn, x, y) && voiceOnline) {
        if (currentVolume > 0) {
            voiceAlert.setVolume(currentVolume - 1);
            voiceAlert.playSystem(VOICE_READY); //  
            screenNeedsRedraw = true;
        }
        return;
    }
    
    ButtonConfig volUpBtn = {
        .x = (int16_t)(SCREEN_WIDTH - SPACING_SM - 70),
        .y = (int16_t)(volY + 75 - 32),
        .w = 60,
        .h = 24,
        .label = "+",
        .style = BTN_SECONDARY,
        .enabled = voiceOnline && currentVolume < 30
    };
    
    if (isButtonPressed(volUpBtn, x, y) && voiceOnline) {
        if (currentVolume < 30) {
            voiceAlert.setVolume(currentVolume + 1);
            voiceAlert.playSystem(VOICE_READY); //  
            screenNeedsRedraw = true;
        }
        return;
    }
    
    //   
    int16_t testY = volY + 75 + SPACING_SM;
    int16_t testBtnW = (SCREEN_WIDTH - SPACING_SM * 5) / 4;
    
    uint8_t testVoices[] = {VOICE_READY, VOICE_START, VOICE_STOP, VOICE_WARNING};
    
    for (int i = 0; i < 4; i++) {
        int16_t btnX = SPACING_SM + i * (testBtnW + SPACING_SM);
        
        ButtonConfig testBtn = {
            .x = btnX,
            .y = testY,
            .w = testBtnW,
            .h = 32,
            .label = "",
            .style = BTN_OUTLINE,
            .enabled = voiceOnline
        };
        
        if (isButtonPressed(testBtn, x, y) && voiceOnline) {
            voiceAlert.playSystem((VoiceSystemId)testVoices[i]);
            return;
        }
    }
    
    #endif
    
    // 
    if (y >= navY) {
        ButtonConfig backBtn = {
            .x = SPACING_SM,
            .y = (int16_t)(navY + 2),
            .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2),
            .h = (int16_t)(FOOTER_HEIGHT - 4),
            .label = "",
            .style = BTN_OUTLINE,
            .enabled = true
        };
        
        if (isButtonPressed(backBtn, x, y)) {
            currentScreen = SCREEN_SETTINGS;
            screenNeedsRedraw = true;
        }
    }
}