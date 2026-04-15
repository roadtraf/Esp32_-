// ================================================================
// UI_Screen_Main.cpp -    
// [U1] UITheme / UIComponents  
// [U8] screenNeedsRedraw / currentScreen    UIManager 
// ================================================================
#include "UIComponents.h"
#include "UITheme.h"
#include "UIManager.h"
#include "Config.h"
#include <NTPClient.h>
#include "SensorManager.h"
#include "SystemController.h"
#include "ErrorHandler.h"

using namespace UIComponents;
using namespace UITheme;

extern TFT_GFX tft;
extern SensorManager   sensorManager;
extern SystemController systemController;
extern UIManager       uiManager;
extern bool            errorActive;
extern ErrorInfo       currentError;
bool canAccessScreen(int screenId);
// ================================================================
//    ( )
// ================================================================
namespace MainLayout {
    //  ( )
    constexpr int16_t STATUS_BAR_Y  = HEADER_HEIGHT;
    constexpr int16_t STATUS_BAR_H  = 24;

    //   
    constexpr int16_t CARD_ROW1_Y   = STATUS_BAR_Y + STATUS_BAR_H + SPACING_SM;
    constexpr int16_t CARD_H        = 80;
    constexpr int16_t CARD_W        = (SCREEN_WIDTH - SPACING_SM * 3) / 2;  // 2
    constexpr int16_t CARD_COL1_X   = SPACING_SM;
    constexpr int16_t CARD_COL2_X   = SPACING_SM * 2 + CARD_W;

    //    ( )
    constexpr int16_t PUMP_CARD_Y   = CARD_ROW1_Y + CARD_H + SPACING_SM;
    constexpr int16_t PUMP_CARD_H   = 56;
    constexpr int16_t PUMP_CARD_W   = SCREEN_WIDTH - SPACING_SM * 2;

    //    
    constexpr int16_t BTN_ROW_Y     = PUMP_CARD_Y + PUMP_CARD_H + SPACING_SM;
    constexpr int16_t BTN_H         = 44;
    constexpr int16_t BTN_W_LARGE   = 140;
    constexpr int16_t BTN_W_SMALL   = 80;

    //    (Footer  )
    constexpr int16_t EVENT_ROW_Y   = SCREEN_HEIGHT - FOOTER_HEIGHT - 22;
}

// ================================================================
//  :    
// ================================================================
static uint16_t pressureColor(float kpa) {
    if (kpa <= PRESSURE_TRIP_KPA)   return COLOR_DANGER;
    if (kpa <= PRESSURE_ALARM_KPA)  return COLOR_WARNING;
    return COLOR_SUCCESS;
}

// ================================================================
//  :    
// ================================================================
static uint16_t tempColor(float c) {
    if (c >= TEMP_TRIP_C)   return COLOR_DANGER;
    if (c >= TEMP_ALARM_C)  return COLOR_WARNING;
    return COLOR_SUCCESS;
}

// ================================================================
//  : tft.textWidth()    X   [U7]
// ================================================================
static int16_t centeredX(const char* text, uint8_t textSize,
                          int16_t areaX, int16_t areaW) {
    tft.setTextSize(textSize);
    int16_t tw = tft.textWidth(text);
    return areaX + (areaW - tw) / 2;
}

// ================================================================
//   (WiFi / MQTT / NTP / E-Stop )
// ================================================================
static void drawStatusBar() {
    int16_t y = MainLayout::STATUS_BAR_Y;
    int16_t h = MainLayout::STATUS_BAR_H;

    tft.fillRect(0, y, SCREEN_WIDTH, h, COLOR_BG_CARD);
    tft.drawFastHLine(0, y + h - 1, SCREEN_WIDTH, COLOR_DIVIDER);

    // WiFi 
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    drawBadge(SPACING_SM, y + 4,
              wifiOk ? "WiFi" : "NoNet",
              wifiOk ? BADGE_SUCCESS : BADGE_DANGER);

    // MQTT  (extern  )
    extern bool mqttConnected;
    drawBadge(SPACING_SM + 52, y + 4,
              mqttConnected ? "MQTT" : "MQTT?",
              mqttConnected ? BADGE_SUCCESS : BADGE_WARNING);

    // E-Stop    
    if (errorActive) {
        int16_t bx = centeredX("!         !", TEXT_SIZE_SMALL,
                                SPACING_SM + 110,
                                SCREEN_WIDTH - 200);
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(bx, y + 6);
        tft.print("!         !");
    }

    //  (NTP  )
    extern bool ntpSynced;
    if (ntpSynced) {
        extern NTPClient ntpClient;
        String t = ntpClient.getFormattedTime();
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        int16_t tx = SCREEN_WIDTH - SPACING_SM - tft.textWidth(t.c_str());
        tft.setCursor(tx, y + 6);
        tft.print(t);
    }
}

// ================================================================
//    ( / )
// ================================================================
static void drawSensorCards() {
    float pressure = sensorManager.getPressure();
    float temp     = sensorManager.getTemperature();
    float current  = sensorManager.getCurrent();

    //    
    {
        CardConfig card = {
            .x = MainLayout::CARD_COL1_X,
            .y = MainLayout::CARD_ROW1_Y,
            .w = MainLayout::CARD_W,
            .h = MainLayout::CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = pressureColor(pressure),
            .elevated = false
        };
        drawCard(card);

        // 
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("");

        //  ( )
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", pressure);
        tft.setTextSize(3);
        tft.setTextColor(pressureColor(pressure));
        tft.setCursor(card.x + CARD_PADDING,
                      card.y + CARD_PADDING + 14);
        tft.print(buf);

        // 
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING,
                      card.y + MainLayout::CARD_H - CARD_PADDING - 12);
        tft.print("kPa");

        //   
        float pct = constrain(
            (pressure - PRESSURE_MIN_KPA) /
            (PRESSURE_MAX_KPA - PRESSURE_MIN_KPA) * 100.0f,
            0.0f, 100.0f);
        drawProgressBar(card.x + CARD_PADDING,
                        card.y + MainLayout::CARD_H - 10,
                        card.w - CARD_PADDING * 2, 6,
                        pct, pressureColor(pressure));
    }

    //    
    {
        CardConfig card = {
            .x = MainLayout::CARD_COL2_X,
            .y = MainLayout::CARD_ROW1_Y,
            .w = MainLayout::CARD_W,
            .h = MainLayout::CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = tempColor(temp),
            .elevated = false
        };
        drawCard(card);

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("");

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", temp);
        tft.setTextSize(3);
        tft.setTextColor(tempColor(temp));
        tft.setCursor(card.x + CARD_PADDING,
                      card.y + CARD_PADDING + 14);
        tft.print(buf);

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING,
                      card.y + MainLayout::CARD_H - CARD_PADDING - 12);
        tft.print("\xB0""C");  // C ( )

        //   
        char cBuf[16];
        snprintf(cBuf, sizeof(cBuf), "%.2fA", current);
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + card.w - CARD_PADDING - tft.textWidth(cBuf),
                      card.y + CARD_PADDING);
        tft.print(cBuf);
    }
}

// ================================================================
//    ( )
// ================================================================
static void drawPumpCard() {
    extern float pumpDutyCycle;
    extern bool  pumpRunning;

    CardConfig card = {
        .x = SPACING_SM,
        .y = MainLayout::PUMP_CARD_Y,
        .w = MainLayout::PUMP_CARD_W,
        .h = MainLayout::PUMP_CARD_H,
        .bgColor = COLOR_BG_CARD,
        .borderColor = pumpRunning ? COLOR_PRIMARY : COLOR_BORDER,
        .elevated = false
    };
    drawCard(card);

    // 
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
    tft.print("");

    //  
    drawBadge(card.x + CARD_PADDING + 36, card.y + CARD_PADDING - 2,
              pumpRunning ? "" : "",
              pumpRunning ? BADGE_SUCCESS : BADGE_INFO);

    //  
    char dutyBuf[16];
    snprintf(dutyBuf, sizeof(dutyBuf), "%.0f%%", pumpDutyCycle);
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(pumpRunning ? COLOR_PRIMARY : COLOR_TEXT_DISABLED);
    tft.setCursor(card.x + CARD_PADDING,
                  card.y + CARD_PADDING + 16);
    tft.print(dutyBuf);

    // 
    drawProgressBar(card.x + CARD_PADDING,
                    card.y + MainLayout::PUMP_CARD_H - 12,
                    card.w - CARD_PADDING * 2, 8,
                    pumpDutyCycle,
                    pumpRunning ? COLOR_PRIMARY : COLOR_TEXT_DISABLED);

    //   ()
    extern bool valveState[3];
    const char* vLabels[] = {"V1", "V2", "V3"};
    for (uint8_t i = 0; i < 3; i++) {
        int16_t vx = card.x + card.w - CARD_PADDING - (3 - i) * 42;
        drawBadge(vx, card.y + CARD_PADDING + 14,
                  vLabels[i],
                  valveState[i] ? BADGE_SUCCESS : BADGE_INFO);
    }
}

// ================================================================
//   
// ================================================================
static void drawControlButtons() {
    int16_t y    = MainLayout::BTN_ROW_Y;
    int16_t h    = MainLayout::BTN_H;
    int16_t gap  = SPACING_SM;

    SystemPermissions perms = systemController.getPermissions();

    //  
    ButtonConfig startBtn = {
        .x = SPACING_SM,
        .y = y,
        .w = MainLayout::BTN_W_LARGE,
        .h = h,
        .label = "   ",
        .style = BTN_SUCCESS,
        .enabled = perms.canStart
    };
    drawButton(startBtn);

    //  
    ButtonConfig stopBtn = {
        .x = (int16_t)(SPACING_SM + MainLayout::BTN_W_LARGE + gap),
        .y = y,
        .w = MainLayout::BTN_W_LARGE,
        .h = h,
        .label = "   ",
        .style = BTN_DANGER,
        .enabled = perms.canStop
    };
    drawButton(stopBtn);

    //  
    ButtonConfig settingsBtn = {
        .x = (int16_t)(SPACING_SM + (MainLayout::BTN_W_LARGE + gap) * 2),
        .y = y,
        .w = MainLayout::BTN_W_SMALL,
        .h = h,
        .label = "",
        .style = BTN_OUTLINE,
        .enabled = true
    };
    drawButton(settingsBtn);

    //   (   )
    ButtonConfig alarmBtn = {
        .x = (int16_t)(SPACING_SM + (MainLayout::BTN_W_LARGE + gap) * 2
                       + MainLayout::BTN_W_SMALL + gap),
        .y = y,
        .w = MainLayout::BTN_W_SMALL,
        .h = h,
        .label = errorActive ? "!" : "",
        .style = errorActive ? BTN_DANGER : BTN_OUTLINE,
        .enabled = true
    };
    drawButton(alarmBtn);
}

// ================================================================
//   
// ================================================================
static void drawEventRow() {
    int16_t y = MainLayout::EVENT_ROW_Y;
    tft.fillRect(0, y, SCREEN_WIDTH, 20, COLOR_BG_DARK);

    tft.setTextSize(1);
    if (errorActive) {
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(SPACING_SM, y + 4);
        tft.printf(" : %s", currentError.message);
    } else {
        tft.setTextColor(COLOR_TEXT_DISABLED);
        tft.setCursor(SPACING_SM, y + 4);
        tft.print(" :   ");
    }
}

/// ================================================================
// drawMainScreen - 메인 화면
// ================================================================
void drawMainScreen() {
    Serial.println("[UI] drawMainScreen called"); Serial.flush();
    Serial.printf("[UI] fillScreen color: 0x%04X\n", COLOR_BG_DARK); Serial.flush();
 
    tft.fillScreen(0xF800);  // 테스트: 전체 빨간색
    return;                   // 나머지 건너뜀 (테스트 후 제거)
 
    tft.fillScreen(COLOR_BG_DARK);
    drawHeader("진공제어");
    drawStatusBar();
    drawSensorCards();
    drawPumpCard();
    drawControlButtons();
    drawEventRow();
 
    NavButton nav[] = {
        {"시작", BTN_PRIMARY,   true},
        {"정지", BTN_OUTLINE, true},
        {"설정", BTN_OUTLINE,   true}
    };
    drawNavBar(nav, 3);
}

// ================================================================
//    [U8] currentScreen    uiManager.setScreen()
// ================================================================
void handleMainTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();  //    

    int16_t btnY = MainLayout::BTN_ROW_Y;
    int16_t btnH = MainLayout::BTN_H;
    int16_t gap  = SPACING_SM;

    SystemPermissions perms = systemController.getPermissions();

    //    
    if (perms.canStart) {
        ButtonConfig startBtn = {
            .x = SPACING_SM, .y = btnY,
            .w = MainLayout::BTN_W_LARGE, .h = btnH,
            .label = "", .style = BTN_SUCCESS, .enabled = true
        };
        if (isButtonPressed(startBtn, x, y)) {
            extern void startVacuumCycle();
            startVacuumCycle();
            uiManager.showToast(" ", COLOR_SUCCESS);
            uiManager.requestRedraw();
            return;
        }
    }

    //    
    if (perms.canStop) {
        ButtonConfig stopBtn = {
            .x = (int16_t)(SPACING_SM + MainLayout::BTN_W_LARGE + gap),
            .y = btnY,
            .w = MainLayout::BTN_W_LARGE, .h = btnH,
            .label = "", .style = BTN_DANGER, .enabled = true
        };
        if (isButtonPressed(stopBtn, x, y)) {
            extern void stopVacuumCycle();
            stopVacuumCycle();
            uiManager.showToast("", COLOR_DANGER);
            uiManager.requestRedraw();
            return;
        }
    }

    //    
    {
        ButtonConfig settingsBtn = {
            .x = (int16_t)(SPACING_SM + (MainLayout::BTN_W_LARGE + gap) * 2),
            .y = btnY,
            .w = MainLayout::BTN_W_SMALL, .h = btnH,
            .label = "", .style = BTN_OUTLINE, .enabled = true
        };
        if (isButtonPressed(settingsBtn, x, y)) {
            uiManager.setScreen(SCREEN_SETTINGS);
            return;
        }
    }

    //    
    {
        ButtonConfig alarmBtn = {
            .x = (int16_t)(SPACING_SM + (MainLayout::BTN_W_LARGE + gap) * 2
                           + MainLayout::BTN_W_SMALL + gap),
            .y = btnY,
            .w = MainLayout::BTN_W_SMALL, .h = btnH,
            .label = "", .style = BTN_OUTLINE, .enabled = true
        };
        if (isButtonPressed(alarmBtn, x, y)) {
            uiManager.setScreen(SCREEN_ALARM);
            return;
        }
    }

    //  NavBar 
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    if (y >= navY) {
        int16_t bw = (SCREEN_WIDTH - SPACING_SM * 4) / 3;
        if (x >= SPACING_SM + (bw + SPACING_SM) && x < SPACING_SM + (bw + SPACING_SM) * 2) {
            uiManager.setScreen(SCREEN_TREND_GRAPH);
        } else if (x >= SPACING_SM + (bw + SPACING_SM) * 2) {
            uiManager.setScreen(SCREEN_STATISTICS);
        }
        return;
    }

    //       Health  
    if (y < HEADER_HEIGHT &&
        x >= HEALTH_ICON_X && x <= HEALTH_ICON_X + HEALTH_ICON_W) {
        if (true || canAccessScreen(SCREEN_HEALTH)) {
            uiManager.setScreen(SCREEN_HEALTH);
        }
    }
}
