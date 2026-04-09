// ================================================================
// UI_Screen_EStop.cpp -     [U5]
// E-Stop      
// ================================================================
#include <driver/gpio.h>        // gpio_get_level()
#include "UIComponents.h"
#include "UITheme.h"
#include "UIManager.h"
#include "Config.h"
#include "HardenedConfig.h"
#include "SystemController.h"
#include "SensorManager.h"

using namespace UIComponents;
using namespace UITheme;

extern TFT_GFX tft;
extern UIManager        uiManager;
extern SystemController systemController;
extern SensorManager    sensorManager;
extern bool             errorActive;
extern ErrorInfo        currentError;

// ================================================================
//  
// ================================================================
namespace EStopLayout {
    //   
    constexpr int16_t BANNER_H     = 64;

    // / 
    constexpr int16_t CAUSE_CARD_Y = BANNER_H + SPACING_SM;
    constexpr int16_t CAUSE_CARD_H = 90;

    //  
    constexpr int16_t ACTION_CARD_Y = CAUSE_CARD_Y + CAUSE_CARD_H + SPACING_SM;
    constexpr int16_t ACTION_CARD_H = 88;

    //   
    constexpr int16_t HIST_Y       = ACTION_CARD_Y + ACTION_CARD_H + SPACING_SM;

    //  
    constexpr int16_t RELEASE_BTN_Y = SCREEN_HEIGHT - FOOTER_HEIGHT - 4;
    constexpr int16_t RELEASE_BTN_W = 200;
    constexpr int16_t RELEASE_BTN_H = 44;
}

// ================================================================
//    (  )
// ================================================================
static uint32_t g_estopStartMs   = 0;
static ScreenType g_prevScreen   = SCREEN_MAIN;  // E-Stop   

void recordEStopStart(ScreenType prevScreen) {
    g_estopStartMs = millis();
    g_prevScreen   = prevScreen;
}

// ================================================================
//   
// ================================================================
void drawEStopScreen() {
    tft.fillScreen(COLOR_BG_DARK);

    //     ( ,  ) 
    tft.fillRect(0, 0, SCREEN_WIDTH, EStopLayout::BANNER_H, COLOR_DANGER);

    //  :     (LovyanGFX setAdjacentColor )
    uint32_t elapsed = (millis() - g_estopStartMs) / 1000;
    bool blink = (elapsed % 2 == 0);

    tft.setTextSize(3);
    tft.setTextColor(blink ? TFT_WHITE : 0xF800);
    const char* title = "     ";
    int16_t tx = (SCREEN_WIDTH - tft.textWidth(title)) / 2;
    tft.setCursor(tx > 0 ? tx : 4, 10);
    tft.print(" ");

    //  
    uint32_t secs = (millis() - g_estopStartMs) / 1000;
    char timeStr[24];
    if (secs < 60) {
        snprintf(timeStr, sizeof(timeStr), ": %lu", secs);
    } else {
        snprintf(timeStr, sizeof(timeStr), ": %lu %lu",
                 secs / 60, secs % 60);
    }
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(TFT_WHITE);
    int16_t tmx = SCREEN_WIDTH - SPACING_SM - tft.textWidth(timeStr);
    tft.setCursor(tmx, EStopLayout::BANNER_H - 16);
    tft.print(timeStr);

    //  /  
    {
        CardConfig card = {
            .x = SPACING_SM,
            .y = EStopLayout::CAUSE_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = EStopLayout::CAUSE_CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = COLOR_DANGER
        };
        drawCard(card);

        drawIconWarning(card.x + CARD_PADDING, card.y + 16, COLOR_DANGER);

        // 
        tft.setTextSize(TEXT_SIZE_MEDIUM);
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING);
        tft.print(": ");
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        if (errorActive && strlen(currentError.message) > 0) {
            tft.print(currentError.message);
        } else {
            tft.print("  ");
        }

        //  
        float p = sensorManager.getPressure();
        float t = sensorManager.getTemperature();
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 24);
        char sensorStr[48];
        snprintf(sensorStr, sizeof(sensorStr),
                 ": %.1f kPa  |  : %.1fC", p, t);
        tft.print(sensorStr);

        //  
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 42);
        char tsStr[32];
        snprintf(tsStr, sizeof(tsStr), ": %lu ",
                 (millis() - g_estopStartMs) / 60000);
        tft.print(tsStr);

        //   
        tft.setTextColor(COLOR_SUCCESS);
        tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 60);
        tft.print("        ");
    }

    //     
    {
        CardConfig card = {
            .x = SPACING_SM,
            .y = EStopLayout::ACTION_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = EStopLayout::ACTION_CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = COLOR_WARNING
        };
        drawCard(card);

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("  ");

        tft.setTextColor(COLOR_TEXT_PRIMARY);
        const char* steps[] = {
            "1.    ",
            "2.      ",
            "3.  []  "
        };
        for (uint8_t i = 0; i < 3; i++) {
            tft.setCursor(card.x + CARD_PADDING + 8,
                          card.y + CARD_PADDING + 18 + i * 20);
            tft.print(steps[i]);
        }
    }

    //       
    {
        extern ErrorInfo errorHistory[];
        extern uint8_t   errorHistCnt;
        extern uint8_t   errorHistIdx;

        if (errorHistCnt > 1) {
            uint8_t prevIdx = (errorHistIdx - 2 + ERROR_HIST_MAX) % ERROR_HIST_MAX;
            tft.setTextSize(1);
            tft.setTextColor(COLOR_TEXT_DISABLED);
            tft.setCursor(SPACING_SM, EStopLayout::HIST_Y);

            uint32_t ago = (millis() - errorHistory[prevIdx].timestamp) / 1000;
            char histBuf[64];
            snprintf(histBuf, sizeof(histBuf),
                     ": %s  (%lu )",
                     errorHistory[prevIdx].message, ago);
            tft.print(histBuf);
        }
    }

    //    (   ) 
    bool btnPinReleased = (gpio_get_level((gpio_num_t)PIN_ESTOP) == 1);

    int16_t btnX = (SCREEN_WIDTH - EStopLayout::RELEASE_BTN_W) / 2;
    ButtonConfig releaseBtn = {
        .x = btnX,
        .y = EStopLayout::RELEASE_BTN_Y,
        .w = EStopLayout::RELEASE_BTN_W,
        .h = EStopLayout::RELEASE_BTN_H,
        .label = btnPinReleased ? " " : "  ...",
        .style = btnPinReleased ? BTN_SUCCESS : BTN_OUTLINE,
        .enabled = btnPinReleased
    };
    drawButton(releaseBtn);

    //    (  )
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_DANGER);
    const char* statusTxt = "   ";
    int16_t sx = (SCREEN_WIDTH - tft.textWidth(statusTxt)) / 2;
    tft.setCursor(sx, SCREEN_HEIGHT - 16);
    tft.print(statusTxt);
}

// ================================================================
//    
// ================================================================
void handleEStopTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();

    bool btnPinReleased = (gpio_get_level((gpio_num_t)PIN_ESTOP) == 1);
    if (!btnPinReleased) {
        uiManager.showToast("  ", COLOR_WARNING);
        return;
    }

    int16_t btnX = (SCREEN_WIDTH - EStopLayout::RELEASE_BTN_W) / 2;
    ButtonConfig releaseBtn = {
        .x = btnX,
        .y = EStopLayout::RELEASE_BTN_Y,
        .w = EStopLayout::RELEASE_BTN_W,
        .h = EStopLayout::RELEASE_BTN_H,
        .label = "",
        .style = BTN_SUCCESS,
        .enabled = true
    };

    if (isButtonPressed(releaseBtn, x, y)) {
        //     
        extern QueueHandle_t g_cmdQueue;
        // SystemCommand cmd;  // 
        // cmd.type = CommandType::RELEASE_ESTOP;
        // strncpy(cmd.origin, "UI_ESTOP", sizeof(cmd.origin) - 1);
        // xQueueSend(g_cmdQueue, &cmd, 0);

        uiManager.showToast(" ", COLOR_SUCCESS);
        //   
        uiManager.setScreen(g_prevScreen);
    }
}
