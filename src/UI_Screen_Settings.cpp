// ================================================================
// UI_Screen_Settings.cpp -   
// [U10]  /   (  )
// [U11] UITheme   (TFT_*   )
// [U3]  PIN   (   PIN )
// [U7]  textWidth()  
// ================================================================
#include "UIComponents.h"
#include "UITheme.h"
#include "UIManager.h"
#include "UI_AccessControl.h"
#include "Config.h"
#include "SystemController.h"
#include "Lang.h"

#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"
extern VoiceAlert voiceAlert;
#endif

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
#include "HealthMonitor.h"
extern HealthMonitor healthMonitor;
#endif

using namespace UIComponents;
using namespace UITheme;

extern TFT_GFX tft;
extern UIManager        uiManager;
extern SystemController systemController;
extern Language         currentLang;

// ================================================================
// [U10]     , /   
// ================================================================
struct MenuItem {
    const char*  title;
    const char*  subtitle;
    uint16_t     accentColor;
    ScreenType   screen;
    bool         requiresManager;   // OPERATOR  
    bool         enabled;           //     
};

//     ( subtitle )
static void buildMenuItems(MenuItem items[], uint8_t* count) {
    uint8_t n = 0;

    items[n++] = {"",   " ",     COLOR_PRIMARY,        SCREEN_TIMING_SETUP,         false, true};
    items[n++] = {"PID",     " ", COLOR_ACCENT,         SCREEN_PID_SETUP,             false, true};
    items[n++] = {"",     " ",     COLOR_INFO,           SCREEN_STATISTICS,            false, true};
    items[n++] = {"",     "",        COLOR_SUCCESS,        SCREEN_TREND_GRAPH,           false, true};
    items[n++] = {"",   " ",     COLOR_WARNING,        SCREEN_CALIBRATION,           true,  true};
    items[n++] = {"",     " ",   COLOR_TEXT_SECONDARY, SCREEN_ABOUT,                 false, true};
    items[n++] = {"",   "",        COLOR_PRIMARY,        SCREEN_HELP,                  false, true};
    items[n++] = {"",   " ",   COLOR_ACCENT,         SCREEN_STATE_DIAGRAM,         false, true};

    //   (subtitle )
    items[n++] = {"",
                  (currentLang == LANG_KO) ? "" : "English",
                  COLOR_INFO,
                  SCREEN_SETTINGS,   //   (index 8)
                  false, true};

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    items[n++] = {"",   " ", COLOR_SUCCESS,        SCREEN_HEALTH,                true,  true};
#endif

#ifdef ENABLE_SMART_ALERTS
    items[n++] = {"",
                  " ",
                  COLOR_MANAGER,
                  SCREEN_SMART_ALERT_CONFIG,
                  true, true};
#endif

#ifdef ENABLE_VOICE_ALERTS
    items[n++] = {"",
                  voiceAlert.isOnline() ? "" : "",
                  COLOR_DEVELOPER,
                  SCREEN_VOICE_SETTINGS,
                  true, true};
#endif

    *count = n;
}

// ================================================================
//  
// ================================================================
namespace SettingsLayout {
    constexpr uint8_t  COLS      = 3;
    constexpr int16_t  START_Y   = HEADER_HEIGHT + SPACING_SM;
    constexpr int16_t  CARD_W    = (SCREEN_WIDTH - SPACING_SM * 4) / COLS;
    constexpr int16_t  CARD_H    = 58;
    constexpr int16_t  CARD_GAP  = SPACING_SM;
}

// ================================================================
//     [U10][U11]
// ================================================================
void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG_DARK);   // [U11] TFT_BLACK  COLOR_BG_DARK
    drawHeader("");

    MenuItem items[20];
    uint8_t  count = 0;
    buildMenuItems(items, &count);

    uint8_t rows = (count + SettingsLayout::COLS - 1) / SettingsLayout::COLS;

    for (uint8_t i = 0; i < count; i++) {
        uint8_t  row = i / SettingsLayout::COLS;
        uint8_t  col = i % SettingsLayout::COLS;
        int16_t  cx  = SPACING_SM + col * (SettingsLayout::CARD_W + SettingsLayout::CARD_GAP);
        int16_t  cy  = SettingsLayout::START_Y
                       + row * (SettingsLayout::CARD_H + SettingsLayout::CARD_GAP);

        bool accessible = !items[i].requiresManager ||
                          !systemController.isOperatorMode();

        CardConfig card = {
            .x = cx, .y = cy,
            .w = SettingsLayout::CARD_W,
            .h = SettingsLayout::CARD_H,
            .bgColor     = accessible ? COLOR_BG_CARD : COLOR_BG_DARK,
            .borderColor = accessible ? items[i].accentColor : COLOR_TEXT_DISABLED
        };
        drawCard(card);

        //   
        tft.fillRect(cx + 2, cy + 2,
                     SettingsLayout::CARD_W - 4, 4,
                     accessible ? items[i].accentColor : COLOR_TEXT_DISABLED);

        // 
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(accessible ? COLOR_TEXT_PRIMARY : COLOR_TEXT_DISABLED);
        tft.setCursor(cx + 6, cy + 12);
        tft.print(items[i].title);

        //  ( 9 + "." )
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);    // [U11]
        tft.setCursor(cx + 6, cy + 28);
        char sub[12];
        strncpy(sub, items[i].subtitle, 9);
        if (strlen(items[i].subtitle) > 9) {
            sub[9] = '.'; sub[10] = '\0';
        }
        tft.print(sub);

        //   
        if (items[i].requiresManager && !accessible) {
            tft.fillCircle(cx + SettingsLayout::CARD_W - 10,
                           cy + 10, 6, COLOR_WARNING);
            tft.setTextSize(1);
            tft.setTextColor(COLOR_BG_DARK);
            tft.setCursor(cx + SettingsLayout::CARD_W - 13, cy + 7);
            tft.print("!");
        }
    }

    //    ()
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    if (!systemController.isOperatorMode()) {
        MaintenanceLevel level = healthMonitor.getMaintenanceLevel();
        if (level >= MAINTENANCE_REQUIRED) {
            int16_t btnY = SettingsLayout::START_Y
                           + rows * (SettingsLayout::CARD_H + SettingsLayout::CARD_GAP);
            ButtonConfig maintBtn = {
                .x = SPACING_SM, .y = btnY,
                .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2), .h = 30,
                .label = "   ",
                .style = BTN_SUCCESS, .enabled = true
            };
            drawButton(maintBtn);
        }
    }
#endif

    NavButton nav[] = {{"", BTN_OUTLINE, true}};
    drawNavBar(nav, 1);
}

// ================================================================
//    [U10]  items  
// ================================================================
void handleSettingsTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();

    //  
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    if (y >= navY) {
        uiManager.setScreen(SCREEN_MAIN);
        return;
    }

    //   
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    if (!systemController.isOperatorMode()) {
        MenuItem items[20]; uint8_t count = 0;
        buildMenuItems(items, &count);
        uint8_t rows = (count + SettingsLayout::COLS - 1) / SettingsLayout::COLS;
        int16_t btnY = SettingsLayout::START_Y
                       + rows * (SettingsLayout::CARD_H + SettingsLayout::CARD_GAP);
        if (healthMonitor.getMaintenanceLevel() >= MAINTENANCE_REQUIRED &&
            x >= SPACING_SM && x <= SCREEN_WIDTH - SPACING_SM &&
            y >= btnY       && y <= btnY + 30) {
            healthMonitor.performMaintenance();
            uiManager.showToast("  ", COLOR_SUCCESS);
            uiManager.requestRedraw();
            return;
        }
    }
#endif

    //   
    MenuItem items[20]; uint8_t count = 0;
    buildMenuItems(items, &count);

    for (uint8_t i = 0; i < count; i++) {
        uint8_t  col = i % SettingsLayout::COLS;
        uint8_t  row = i / SettingsLayout::COLS;
        int16_t  cx  = SPACING_SM + col * (SettingsLayout::CARD_W + SettingsLayout::CARD_GAP);
        int16_t  cy  = SettingsLayout::START_Y
                       + row * (SettingsLayout::CARD_H + SettingsLayout::CARD_GAP);

        if (x >= cx && x <= cx + SettingsLayout::CARD_W &&
            y >= cy && y <= cy + SettingsLayout::CARD_H) {

            //   
            if (items[i].requiresManager && systemController.isOperatorMode()) {
                // [U3]    + PIN  
                showAccessDeniedAsync(items[i].title);
                // PIN    
                showPinInputScreen(SystemMode::MANAGER,
                    [](bool ok, SystemMode) {
                        if (ok) uiManager.requestRedraw();
                    });
                return;
            }

            //   (index 8, screen == SCREEN_SETTINGS)
            if (i == 8) {
                currentLang = (currentLang == LANG_EN) ? LANG_KO : LANG_EN;
                config.language = (uint8_t)currentLang;
#ifdef ENABLE_VOICE_ALERTS
                if (voiceAlert.isOnline()) {
                    voiceAlert.setLanguage((currentLang == LANG_KO)
                                           ? LANG_KO : LANG_EN);
                }
#endif
                // saveConfig();  // 
                uiManager.requestRedraw();
                return;
            }

            uiManager.setScreen(items[i].screen);
            return;
        }
    }
}
