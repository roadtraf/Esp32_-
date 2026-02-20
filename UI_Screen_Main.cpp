// ================================================================
// UI_Screen_Main.cpp - 메인 화면 완전 재작성
// [U1] UITheme / UIComponents 전면 적용
// [U8] screenNeedsRedraw / currentScreen 직접 접근 → UIManager 경유
// ================================================================
#include "../include/UIComponents.h"
#include "../include/UITheme.h"
#include "../include/UIManager.h"
#include "../include/Config.h"
#include "../include/SensorManager.h"
#include "../include/SystemController.h"
#include "../include/ErrorHandler.h"

using namespace UIComponents;
using namespace UITheme;

extern LGFX            tft;
extern SensorManager   sensorManager;
extern SystemController systemController;
extern UIManager       uiManager;
extern bool            errorActive;
extern ErrorInfo       currentError;

// ================================================================
// 내부 레이아웃 상수 (매직넘버 제거)
// ================================================================
namespace MainLayout {
    // 상태바 (헤더 아래)
    constexpr int16_t STATUS_BAR_Y  = HEADER_HEIGHT;
    constexpr int16_t STATUS_BAR_H  = 24;

    // 센서 카드 영역
    constexpr int16_t CARD_ROW1_Y   = STATUS_BAR_Y + STATUS_BAR_H + SPACING_SM;
    constexpr int16_t CARD_H        = 80;
    constexpr int16_t CARD_W        = (SCREEN_WIDTH - SPACING_SM * 3) / 2;  // 2열
    constexpr int16_t CARD_COL1_X   = SPACING_SM;
    constexpr int16_t CARD_COL2_X   = SPACING_SM * 2 + CARD_W;

    // 펌프 상태 카드 (전체 너비)
    constexpr int16_t PUMP_CARD_Y   = CARD_ROW1_Y + CARD_H + SPACING_SM;
    constexpr int16_t PUMP_CARD_H   = 56;
    constexpr int16_t PUMP_CARD_W   = SCREEN_WIDTH - SPACING_SM * 2;

    // 하단 조작 버튼 영역
    constexpr int16_t BTN_ROW_Y     = PUMP_CARD_Y + PUMP_CARD_H + SPACING_SM;
    constexpr int16_t BTN_H         = 44;
    constexpr int16_t BTN_W_LARGE   = 140;
    constexpr int16_t BTN_W_SMALL   = 80;

    // 최근 이벤트 줄 (Footer 바로 위)
    constexpr int16_t EVENT_ROW_Y   = SCREEN_HEIGHT - FOOTER_HEIGHT - 22;
}

// ================================================================
// 내부 헬퍼: 압력값 → 상태 색상
// ================================================================
static uint16_t pressureColor(float kpa) {
    if (kpa <= PRESSURE_TRIP_KPA)   return COLOR_DANGER;
    if (kpa <= PRESSURE_ALARM_KPA)  return COLOR_WARNING;
    return COLOR_SUCCESS;
}

// ================================================================
// 내부 헬퍼: 온도값 → 상태 색상
// ================================================================
static uint16_t tempColor(float c) {
    if (c >= TEMP_TRIP_C)   return COLOR_DANGER;
    if (c >= TEMP_ALARM_C)  return COLOR_WARNING;
    return COLOR_SUCCESS;
}

// ================================================================
// 내부 헬퍼: tft.textWidth() 기반 중앙 정렬 X 계산  [U7]
// ================================================================
static int16_t centeredX(const char* text, uint8_t textSize,
                          int16_t areaX, int16_t areaW) {
    tft.setTextSize(textSize);
    int16_t tw = tft.textWidth(text);
    return areaX + (areaW - tw) / 2;
}

// ================================================================
// 상태바 그리기 (WiFi / MQTT / NTP / E-Stop 표시)
// ================================================================
static void drawStatusBar() {
    int16_t y = MainLayout::STATUS_BAR_Y;
    int16_t h = MainLayout::STATUS_BAR_H;

    tft.fillRect(0, y, SCREEN_WIDTH, h, COLOR_BG_CARD);
    tft.drawFastHLine(0, y + h - 1, SCREEN_WIDTH, COLOR_DIVIDER);

    // WiFi 상태
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    drawBadge(SPACING_SM, y + 4,
              wifiOk ? "WiFi" : "NoNet",
              wifiOk ? BADGE_SUCCESS : BADGE_DANGER);

    // MQTT 상태 (extern 변수 사용)
    extern bool mqttConnected;
    drawBadge(SPACING_SM + 52, y + 4,
              mqttConnected ? "MQTT" : "MQTT?",
              mqttConnected ? BADGE_SUCCESS : BADGE_WARNING);

    // E-Stop 활성 시 경고 배지
    if (errorActive) {
        int16_t bx = centeredX("! 경  보  발  생  !", TEXT_SIZE_SMALL,
                                SPACING_SM + 110,
                                SCREEN_WIDTH - 200);
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_DANGER);
        tft.setCursor(bx, y + 6);
        tft.print("! 경  보  발  생  !");
    }

    // 시각 (NTP 동기화 시)
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
// 센서 카드 그리기 (압력 / 온도)
// ================================================================
static void drawSensorCards() {
    float pressure = sensorManager.getPressure();
    float temp     = sensorManager.getTemperature();
    float current  = sensorManager.getCurrent();

    // ── 압력 카드 ──
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

        // 라벨
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("압력");

        // 값 (큰 폰트)
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", pressure);
        tft.setTextSize(3);
        tft.setTextColor(pressureColor(pressure));
        tft.setCursor(card.x + CARD_PADDING,
                      card.y + CARD_PADDING + 14);
        tft.print(buf);

        // 단위
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(card.x + CARD_PADDING,
                      card.y + MainLayout::CARD_H - CARD_PADDING - 12);
        tft.print("kPa");

        // 설정값 대비 프로그레스바
        float pct = constrain(
            (pressure - PRESSURE_MIN_KPA) /
            (PRESSURE_MAX_KPA - PRESSURE_MIN_KPA) * 100.0f,
            0.0f, 100.0f);
        drawProgressBar(card.x + CARD_PADDING,
                        card.y + MainLayout::CARD_H - 10,
                        card.w - CARD_PADDING * 2, 6,
                        pct, pressureColor(pressure));
    }

    // ── 온도 카드 ──
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
        tft.print("온도");

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
        tft.print("\xB0""C");  // °C (멀티바이트 회피)

        // 전류 보조 표시
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
// 펌프 상태 카드 (전체 너비)
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

    // 라벨
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
    tft.print("펌프");

    // 상태 배지
    drawBadge(card.x + CARD_PADDING + 36, card.y + CARD_PADDING - 2,
              pumpRunning ? "운전" : "정지",
              pumpRunning ? BADGE_SUCCESS : BADGE_INFO);

    // 듀티 값
    char dutyBuf[16];
    snprintf(dutyBuf, sizeof(dutyBuf), "%.0f%%", pumpDutyCycle);
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(pumpRunning ? COLOR_PRIMARY : COLOR_TEXT_DISABLED);
    tft.setCursor(card.x + CARD_PADDING,
                  card.y + CARD_PADDING + 16);
    tft.print(dutyBuf);

    // 프로그레스바
    drawProgressBar(card.x + CARD_PADDING,
                    card.y + MainLayout::PUMP_CARD_H - 12,
                    card.w - CARD_PADDING * 2, 8,
                    pumpDutyCycle,
                    pumpRunning ? COLOR_PRIMARY : COLOR_TEXT_DISABLED);

    // 밸브 상태 (우측)
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
// 조작 버튼 행
// ================================================================
static void drawControlButtons() {
    int16_t y    = MainLayout::BTN_ROW_Y;
    int16_t h    = MainLayout::BTN_H;
    int16_t gap  = SPACING_SM;

    SystemPermissions perms = systemController.getPermissions();

    // 시작 버튼
    ButtonConfig startBtn = {
        .x = SPACING_SM,
        .y = y,
        .w = MainLayout::BTN_W_LARGE,
        .h = h,
        .label = "▶ 시  작",
        .style = BTN_SUCCESS,
        .enabled = perms.canStart
    };
    drawButton(startBtn);

    // 정지 버튼
    ButtonConfig stopBtn = {
        .x = (int16_t)(SPACING_SM + MainLayout::BTN_W_LARGE + gap),
        .y = y,
        .w = MainLayout::BTN_W_LARGE,
        .h = h,
        .label = "■ 정  지",
        .style = BTN_DANGER,
        .enabled = perms.canStop
    };
    drawButton(stopBtn);

    // 설정 버튼
    ButtonConfig settingsBtn = {
        .x = (int16_t)(SPACING_SM + (MainLayout::BTN_W_LARGE + gap) * 2),
        .y = y,
        .w = MainLayout::BTN_W_SMALL,
        .h = h,
        .label = "설정",
        .style = BTN_OUTLINE,
        .enabled = true
    };
    drawButton(settingsBtn);

    // 알람 버튼 (에러 활성 시 강조)
    ButtonConfig alarmBtn = {
        .x = (int16_t)(SPACING_SM + (MainLayout::BTN_W_LARGE + gap) * 2
                       + MainLayout::BTN_W_SMALL + gap),
        .y = y,
        .w = MainLayout::BTN_W_SMALL,
        .h = h,
        .label = errorActive ? "!알람" : "알람",
        .style = errorActive ? BTN_DANGER : BTN_OUTLINE,
        .enabled = true
    };
    drawButton(alarmBtn);
}

// ================================================================
// 최근 이벤트 줄
// ================================================================
static void drawEventRow() {
    int16_t y = MainLayout::EVENT_ROW_Y;
    tft.fillRect(0, y, SCREEN_WIDTH, 20, COLOR_BG_DARK);

    tft.setTextSize(1);
    if (errorActive) {
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(SPACING_SM, y + 4);
        tft.printf("최근 이벤트: %s", currentError.message);
    } else {
        tft.setTextColor(COLOR_TEXT_DISABLED);
        tft.setCursor(SPACING_SM, y + 4);
        tft.print("최근 이벤트: 정상 운전 중");
    }
}

// ================================================================
// 메인 화면 전체 그리기  [U1]
// ================================================================
void drawMainScreen() {
    tft.fillScreen(COLOR_BG_DARK);

    drawHeader("진공 제어 시스템");
    drawStatusBar();
    drawSensorCards();
    drawPumpCard();
    drawControlButtons();
    drawEventRow();

    // 하단 네비게이션 (설정 / 알람 빠른 접근)
    NavButton nav[] = {
        {"메인", BTN_PRIMARY,   true},
        {"그래프", BTN_OUTLINE, true},
        {"이력", BTN_OUTLINE,   true}
    };
    drawNavBar(nav, 3);
}

// ================================================================
// 터치 처리  [U8] currentScreen 직접 대입 → uiManager.setScreen()
// ================================================================
void handleMainTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();  // 자동 로그아웃 타이머 리셋

    int16_t btnY = MainLayout::BTN_ROW_Y;
    int16_t btnH = MainLayout::BTN_H;
    int16_t gap  = SPACING_SM;

    SystemPermissions perms = systemController.getPermissions();

    // ── 시작 버튼 ──
    if (perms.canStart) {
        ButtonConfig startBtn = {
            .x = SPACING_SM, .y = btnY,
            .w = MainLayout::BTN_W_LARGE, .h = btnH,
            .label = "", .style = BTN_SUCCESS, .enabled = true
        };
        if (isButtonPressed(startBtn, x, y)) {
            extern void startVacuumCycle();
            startVacuumCycle();
            uiManager.showToast("진공 시작", COLOR_SUCCESS);
            uiManager.requestRedraw();
            return;
        }
    }

    // ── 정지 버튼 ──
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
            uiManager.showToast("정지됨", COLOR_DANGER);
            uiManager.requestRedraw();
            return;
        }
    }

    // ── 설정 버튼 ──
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

    // ── 알람 버튼 ──
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

    // ── NavBar ──
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

    // ── 헤더 건강도 아이콘 터치 → Health 화면 ──
    if (y < HEADER_HEIGHT &&
        x >= HEALTH_ICON_X && x <= HEALTH_ICON_X + HEALTH_ICON_W) {
        if (canAccessScreen(SCREEN_HEALTH)) {
            uiManager.setScreen(SCREEN_HEALTH);
        }
    }
}
