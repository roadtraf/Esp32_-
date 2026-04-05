// ================================================================
// UI_Screen_Settings.cpp - 설정 화면 개선판
// [U10] 메뉴 데이터/터치 코드 통합 (중복 배열 제거)
// [U11] UITheme 색상 통일 (TFT_* 직접 사용 제거)
// [U3]  PIN 화면 연동 (관리자 전환 시 PIN 요구)
// [U7]  textWidth() 기반 정렬
// ================================================================
#include "../include/UIComponents.h"
#include "../include/UITheme.h"
#include "../include/UIManager.h"
#include "../include/UI_AccessControl.h"
#include "../include/Config.h"
#include "../include/SystemController.h"
#include "../include/Lang.h"

#ifdef ENABLE_VOICE_ALERTS
#include "../include/VoiceAlert.h"
extern VoiceAlert voiceAlert;
#endif

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
#include "../include/HealthMonitor.h"
extern HealthMonitor healthMonitor;
#endif

using namespace UIComponents;
using namespace UITheme;

extern LGFX             tft;
extern UIManager        uiManager;
extern SystemController systemController;
extern Language         currentLang;

// ================================================================
// [U10] 메뉴 정의 — 단일 배열, 그리기/터치 모두 여기서 참조
// ================================================================
struct MenuItem {
    const char*  title;
    const char*  subtitle;
    uint16_t     accentColor;
    ScreenType   screen;
    bool         requiresManager;   // OPERATOR 접근 불가
    bool         enabled;           // 컴파일 타임 기능 활성화 여부
};

// 메뉴 항목 빌드 함수 (런타임 subtitle 결정)
static void buildMenuItems(MenuItem items[], uint8_t* count) {
    uint8_t n = 0;

    items[n++] = {"타이밍",   "시간 설정",     COLOR_PRIMARY,        SCREEN_TIMING_SETUP,         false, true};
    items[n++] = {"PID",     "제어 파라미터", COLOR_ACCENT,         SCREEN_PID_SETUP,             false, true};
    items[n++] = {"통계",     "사용 기록",     COLOR_INFO,           SCREEN_STATISTICS,            false, true};
    items[n++] = {"추세",     "그래프",        COLOR_SUCCESS,        SCREEN_TREND_GRAPH,           false, true};
    items[n++] = {"캘리브",   "센서 조정",     COLOR_WARNING,        SCREEN_CALIBRATION,           true,  true};
    items[n++] = {"정보",     "시스템 정보",   COLOR_TEXT_SECONDARY, SCREEN_ABOUT,                 false, true};
    items[n++] = {"도움말",   "사용법",        COLOR_PRIMARY,        SCREEN_HELP,                  false, true};
    items[n++] = {"상태도",   "시스템 상태",   COLOR_ACCENT,         SCREEN_STATE_DIAGRAM,         false, true};

    // 언어 토글 (subtitle 동적)
    items[n++] = {"언어",
                  (currentLang == LANG_KO) ? "한국어" : "English",
                  COLOR_INFO,
                  SCREEN_SETTINGS,   // 특수 처리 (index 8)
                  false, true};

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    items[n++] = {"건강도",   "예측 유지보수", COLOR_SUCCESS,        SCREEN_HEALTH,                true,  true};
#endif

#ifdef ENABLE_SMART_ALERTS
    items[n++] = {"알림",
                  "스마트 알림",
                  COLOR_MANAGER,
                  SCREEN_SMART_ALERT_CONFIG,
                  true, true};
#endif

#ifdef ENABLE_VOICE_ALERTS
    items[n++] = {"음성",
                  voiceAlert.isOnline() ? "활성" : "비활성",
                  COLOR_DEVELOPER,
                  SCREEN_VOICE_SETTINGS,
                  true, true};
#endif

    *count = n;
}

// ================================================================
// 레이아웃 상수
// ================================================================
namespace SettingsLayout {
    constexpr uint8_t  COLS      = 3;
    constexpr int16_t  START_Y   = HEADER_HEIGHT + SPACING_SM;
    constexpr int16_t  CARD_W    = (SCREEN_WIDTH - SPACING_SM * 4) / COLS;
    constexpr int16_t  CARD_H    = 58;
    constexpr int16_t  CARD_GAP  = SPACING_SM;
}

// ================================================================
// 설정 화면 그리기  [U10][U11]
// ================================================================
void drawSettingsScreen() {
    tft.fillScreen(COLOR_BG_DARK);   // [U11] TFT_BLACK → COLOR_BG_DARK
    drawHeader("설정");

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

        // 상단 색상 바
        tft.fillRect(cx + 2, cy + 2,
                     SettingsLayout::CARD_W - 4, 4,
                     accessible ? items[i].accentColor : COLOR_TEXT_DISABLED);

        // 타이틀
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(accessible ? COLOR_TEXT_PRIMARY : COLOR_TEXT_DISABLED);
        tft.setCursor(cx + 6, cy + 12);
        tft.print(items[i].title);

        // 서브타이틀 (최대 9자 + "." 잘라냄)
        tft.setTextSize(1);
        tft.setTextColor(COLOR_TEXT_SECONDARY);    // [U11]
        tft.setCursor(cx + 6, cy + 28);
        char sub[12];
        strncpy(sub, items[i].subtitle, 9);
        if (strlen(items[i].subtitle) > 9) {
            sub[9] = '.'; sub[10] = '\0';
        }
        tft.print(sub);

        // 관리자 잠금 아이콘
        if (items[i].requiresManager && !accessible) {
            tft.fillCircle(cx + SettingsLayout::CARD_W - 10,
                           cy + 10, 6, COLOR_WARNING);
            tft.setTextSize(1);
            tft.setTextColor(COLOR_BG_DARK);
            tft.setCursor(cx + SettingsLayout::CARD_W - 13, cy + 7);
            tft.print("!");
        }
    }

    // 유지보수 완료 버튼 (조건부)
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    if (!systemController.isOperatorMode()) {
        MaintenanceLevel level = healthMonitor.getMaintenanceLevel();
        if (level >= MAINTENANCE_REQUIRED) {
            int16_t btnY = SettingsLayout::START_Y
                           + rows * (SettingsLayout::CARD_H + SettingsLayout::CARD_GAP);
            ButtonConfig maintBtn = {
                .x = SPACING_SM, .y = btnY,
                .w = (int16_t)(SCREEN_WIDTH - SPACING_SM * 2), .h = 30,
                .label = "✓ 유지보수 완료 처리",
                .style = BTN_SUCCESS, .enabled = true
            };
            drawButton(maintBtn);
        }
    }
#endif

    NavButton nav[] = {{"뒤로", BTN_OUTLINE, true}};
    drawNavBar(nav, 1);
}

// ================================================================
// 터치 처리  [U10] 단일 items 배열 재사용
// ================================================================
void handleSettingsTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();

    // 뒤로 버튼
    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    if (y >= navY) {
        uiManager.setScreen(SCREEN_MAIN);
        return;
    }

    // 유지보수 완료 버튼
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
            uiManager.showToast("유지보수 완료 처리됨", COLOR_SUCCESS);
            uiManager.requestRedraw();
            return;
        }
    }
#endif

    // 메뉴 카드 터치
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

            // 접근 권한 확인
            if (items[i].requiresManager && systemController.isOperatorMode()) {
                // [U3] 비동기 접근 거부 + PIN 화면 제시
                showAccessDeniedAsync(items[i].title);
                // PIN 화면으로 관리자 전환 제안
                showPinInputScreen(SystemMode::MANAGER,
                    [](bool ok, SystemMode) {
                        if (ok) uiManager.requestRedraw();
                    });
                return;
            }

            // 언어 토글 (index 8, screen == SCREEN_SETTINGS)
            if (i == 8) {
                currentLang = (currentLang == LANG_EN) ? LANG_KO : LANG_EN;
                config.language = (uint8_t)currentLang;
#ifdef ENABLE_VOICE_ALERTS
                if (voiceAlert.isOnline()) {
                    voiceAlert.setLanguage((currentLang == LANG_KO)
                                           ? LANGUAGE_KOREAN : LANGUAGE_ENGLISH);
                }
#endif
                saveConfig();
                uiManager.requestRedraw();
                return;
            }

            uiManager.setScreen(items[i].screen);
            return;
        }
    }
}
