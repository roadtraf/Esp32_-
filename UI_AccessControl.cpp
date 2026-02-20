// ================================================================
// UI_AccessControl.cpp - PIN 입력 화면 및 접근 제어 구현
// [U2] canAccessScreen() 단일 구현 (ManagerUI.cpp / UI_Screens.cpp 중복 제거)
// [U3] vTaskDelay 블로킹 → UIManager 비동기 메시지로 교체
// [U4] PIN 입력 화면 신규 구현 (숫자 패드 + 잠금 해제)
// 추가 기능: PIN 화면 키보드 입력 추가
// - handleKeyboardOnPinScreen() 함수로 키보드 입력 지원
// - 숫자 0~9: PIN 입력
// - Enter/확인: PIN 검증
// - Backspace/Delete: 한 자 지우기
// - ESC: 취소
// ================================================================
#include "../include/UI_AccessControl.h"
#include "../include/UIComponents.h"
#include "../include/UITheme.h"
#include "../include/UIManager.h"
#include "../include/SystemController.h"

using namespace UIComponents;
using namespace UITheme;

extern LGFX tft;
extern UIManager uiManager;
extern SystemController systemController;

// ================================================================
// [U2] canAccessScreen() — 단일 구현체
// ================================================================
bool canAccessScreen(ScreenType screen) {
    SystemMode mode = systemController.getMode();

    if (mode == SystemMode::OPERATOR) {
        switch (screen) {
            case SCREEN_CALIBRATION:
            case SCREEN_SMART_ALERT_CONFIG:
            case SCREEN_VOICE_SETTINGS:
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
            case SCREEN_HEALTH:
            case SCREEN_HEALTH_TREND:
#endif
#ifdef ENABLE_ADVANCED_ANALYSIS
            case SCREEN_ADVANCED_ANALYSIS:
            case SCREEN_FAILURE_PREDICTION:
            case SCREEN_COMPONENT_LIFE:
            case SCREEN_OPTIMIZATION:
            case SCREEN_COMPREHENSIVE_REPORT:
            case SCREEN_COST_ANALYSIS:
#endif
                return false;
            default:
                return true;
        }
    }
    return true;
}

// ================================================================
// PIN 입력 내부 상태
// ================================================================
namespace {
    constexpr uint8_t PIN_MAX_DIGITS = 4;
    constexpr uint8_t KEYPAD_COLS    = 3;
    constexpr uint8_t KEYPAD_ROWS    = 4;

    // 레이아웃 상수
    constexpr int16_t OVERLAY_X   = 60;
    constexpr int16_t OVERLAY_Y   = 20;
    constexpr int16_t OVERLAY_W   = SCREEN_WIDTH  - 120;
    constexpr int16_t OVERLAY_H   = SCREEN_HEIGHT - 40;

    constexpr int16_t PAD_X       = OVERLAY_X + 20;
    constexpr int16_t PAD_Y       = OVERLAY_Y + 90;
    constexpr int16_t KEY_W       = 60;
    constexpr int16_t KEY_H       = 44;
    constexpr int16_t KEY_GAP     = 6;

    // 숫자 패드 레이블
    const char* KEY_LABELS[KEYPAD_ROWS][KEYPAD_COLS] = {
        {"1", "2", "3"},
        {"4", "5", "6"},
        {"7", "8", "9"},
        {"←", "0", "OK"}
    };

    // 상태
    bool            g_pinActive     = false;
    char            g_pinInput[PIN_MAX_DIGITS + 1] = {0};
    uint8_t         g_pinLen        = 0;
    SystemMode      g_targetMode    = SystemMode::MANAGER;
    PinResultCallback g_callback    = nullptr;

    // 잠금 표시 (로그인 시도 초과)
    bool            g_locked        = false;
    uint32_t        g_lockEndMs     = 0;
}

// ================================================================
// 내부: 키 좌표 계산
// ================================================================
static void keyRect(uint8_t row, uint8_t col,
                    int16_t* ox, int16_t* oy, int16_t* ow, int16_t* oh) {
    *ox = PAD_X + col * (KEY_W + KEY_GAP);
    *oy = PAD_Y + row * (KEY_H + KEY_GAP);
    *ow = KEY_W;
    *oh = KEY_H;
}

// ================================================================
// [U4] PIN 화면 그리기
// ================================================================
void drawPinInputScreen() {
    if (!g_pinActive) return;

    // 반투명 오버레이 배경
    tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG_DARK);
    tft.fillRoundRect(OVERLAY_X, OVERLAY_Y, OVERLAY_W, OVERLAY_H,
                      12, COLOR_BG_CARD);
    tft.drawRoundRect(OVERLAY_X, OVERLAY_Y, OVERLAY_W, OVERLAY_H,
                      12,
                      (g_targetMode == SystemMode::MANAGER)
                          ? COLOR_MANAGER : COLOR_DEVELOPER);

    // 제목
    const char* title = (g_targetMode == SystemMode::MANAGER)
                        ? "관리자 모드 전환"
                        : "개발자 모드 전환";
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    int16_t titleX = OVERLAY_X + (OVERLAY_W - tft.textWidth(title)) / 2;
    tft.setCursor(titleX, OVERLAY_Y + 16);
    tft.print(title);

    // 잠금 상태 표시
    if (g_locked) {
        uint32_t remaining = (g_lockEndMs > millis())
                             ? (g_lockEndMs - millis()) / 1000 : 0;
        char lockMsg[48];
        snprintf(lockMsg, sizeof(lockMsg),
                 "잠김 - %lu초 후 재시도 가능", remaining);
        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_DANGER);
        int16_t lx = OVERLAY_X + (OVERLAY_W - tft.textWidth(lockMsg)) / 2;
        tft.setCursor(lx, OVERLAY_Y + 44);
        tft.print(lockMsg);
        return;
    }

    // PIN 입력 표시 (●로 가림)
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    int16_t hintX = OVERLAY_X + (OVERLAY_W - tft.textWidth("PIN 4자리 입력")) / 2;
    tft.setCursor(hintX, OVERLAY_Y + 42);
    tft.print("PIN 4자리 입력");

    // 입력된 자리 수 표시 (●●○○)
    int16_t dotStartX = OVERLAY_X + OVERLAY_W / 2 - PIN_MAX_DIGITS * 14;
    for (uint8_t i = 0; i < PIN_MAX_DIGITS; i++) {
        uint16_t clr = (i < g_pinLen) ? COLOR_PRIMARY : COLOR_BORDER;
        tft.fillCircle(dotStartX + i * 28 + 14, OVERLAY_Y + 68, 8, clr);
    }

    // 숫자 패드
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            int16_t kx, ky, kw, kh;
            keyRect(r, c, &kx, &ky, &kw, &kh);

            // 특수키 색상
            bool isOk  = (r == 3 && c == 2);
            bool isDel = (r == 3 && c == 0);
            uint16_t bg = isOk  ? COLOR_PRIMARY :
                          isDel ? COLOR_BG_ELEVATED :
                                  COLOR_BG_CARD;
            uint16_t bd = isOk  ? COLOR_PRIMARY :
                                  COLOR_BORDER;

            tft.fillRoundRect(kx, ky, kw, kh, BUTTON_RADIUS, bg);
            tft.drawRoundRect(kx, ky, kw, kh, BUTTON_RADIUS, bd);

            tft.setTextSize(TEXT_SIZE_MEDIUM);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            int16_t lx = kx + (kw - tft.textWidth(KEY_LABELS[r][c])) / 2;
            int16_t ly = ky + (kh - 8 * TEXT_SIZE_MEDIUM) / 2;
            tft.setCursor(lx, ly);
            tft.print(KEY_LABELS[r][c]);
        }
    }

    // 취소 버튼
    int16_t cancelX = OVERLAY_X + 16;
    int16_t cancelY = OVERLAY_Y + OVERLAY_H - 44;
    tft.fillRoundRect(cancelX, cancelY, 80, 32, BUTTON_RADIUS, COLOR_BG_ELEVATED);
    tft.drawRoundRect(cancelX, cancelY, 80, 32, BUTTON_RADIUS, COLOR_BORDER);
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(cancelX + (80 - tft.textWidth("취소")) / 2, cancelY + 10);
    tft.print("취소");
}

// ================================================================
// PIN 화면 터치 처리
// ================================================================
void handlePinInputTouch(uint16_t x, uint16_t y) {
    if (!g_pinActive) return;

    // 잠금 상태 갱신
    if (g_locked) {
        if (millis() >= g_lockEndMs) {
            g_locked = false;
            drawPinInputScreen();
        }
        return;
    }

    // 취소 버튼
    int16_t cancelX = OVERLAY_X + 16;
    int16_t cancelY = OVERLAY_Y + OVERLAY_H - 44;
    if (x >= cancelX && x <= cancelX + 80 &&
        y >= cancelY && y <= cancelY + 32) {
        g_pinActive = false;
        g_pinLen    = 0;
        memset(g_pinInput, 0, sizeof(g_pinInput));
        if (g_callback) g_callback(false, g_targetMode);
        uiManager.requestRedraw();
        return;
    }

    // 숫자 패드 터치 확인
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            int16_t kx, ky, kw, kh;
            keyRect(r, c, &kx, &ky, &kw, &kh);

            if (x >= kx && x <= kx + kw && y >= ky && y <= ky + kh) {
                const char* label = KEY_LABELS[r][c];

                if (strcmp(label, "←") == 0) {
                    // 지우기
                    if (g_pinLen > 0) {
                        g_pinLen--;
                        g_pinInput[g_pinLen] = '\0';
                        drawPinInputScreen();
                    }
                } else if (strcmp(label, "OK") == 0) {
                    // PIN 검증
                    bool ok = false;
                    if (g_targetMode == SystemMode::MANAGER) {
                        ok = systemController.enterManagerMode(g_pinInput);
                    } else {
                        ok = systemController.enterDeveloperMode(g_pinInput);
                    }

                    g_pinLen = 0;
                    memset(g_pinInput, 0, sizeof(g_pinInput));
                    g_pinActive = false;

                    if (ok) {
                        uiManager.showToast(
                            (g_targetMode == SystemMode::MANAGER)
                                ? "관리자 모드" : "개발자 모드",
                            COLOR_MANAGER);
                    } else {
                        // 잠금 여부 확인
                        if (systemController.isLockedOut()) {
                            g_locked    = true;
                            g_lockEndMs = millis() +
                                systemController.getLockoutRemainingTime();
                            drawPinInputScreen();
                            return;
                        }
                        uiManager.showToast("PIN 오류", COLOR_DANGER);
                    }

                    if (g_callback) g_callback(ok, g_targetMode);
                    uiManager.requestRedraw();
                } else {
                    // 숫자 입력
                    if (g_pinLen < PIN_MAX_DIGITS) {
                        g_pinInput[g_pinLen++] = label[0];
                        g_pinInput[g_pinLen]   = '\0';
                        drawPinInputScreen();
                    }
                }
                return;
            }
        }
    }
}

// ================================================================
// PIN 화면 표시 진입점
// ================================================================
void showPinInputScreen(SystemMode targetMode, PinResultCallback onResult) {
    g_pinActive   = true;
    g_pinLen      = 0;
    g_targetMode  = targetMode;
    g_callback    = onResult;
    g_locked      = systemController.isLockedOut();
    if (g_locked) {
        g_lockEndMs = millis() + systemController.getLockoutRemainingTime();
    }
    memset(g_pinInput, 0, sizeof(g_pinInput));
    drawPinInputScreen();
}

bool isPinScreenActive() {
    return g_pinActive;
}

// ================================================================
// [U3] 비동기 접근 거부 알림 — vTaskDelay 제거
// ================================================================
void showAccessDeniedAsync(const char* screenName) {
    char msg[64];
    snprintf(msg, sizeof(msg), "'%s' — 관리자 권한 필요", screenName);
    // UIManager.showMessage()는 타이머 기반 자동 소멸 (블로킹 없음)
    uiManager.showMessage(msg, 2500);
}

// ================================================================
// 키보드 입력 처리 (PIN 화면 전용)
// ================================================================
void handleKeyboardOnPinScreen(uint8_t key) {
    if (!g_pinActive) return;

    // 잠금 상태 체크
    if (g_locked) {
        if (millis() >= g_lockEndMs) {
            g_locked = false;
            drawPinInputScreen();
        }
        return;
    }

    // 숫자 입력 (0~9)
    if (key >= '0' && key <= '9') {
        if (g_pinLen < PIN_MAX_DIGITS) {
            g_pinInput[g_pinLen++] = key;
            g_pinInput[g_pinLen] = '\0';
            drawPinInputScreen();
            Serial.printf("[PIN] 키보드 입력: %c (길이: %d)\n", key, g_pinLen);
        }
        return;
    }

    // Enter (확인) — OK 버튼과 동일
    if (key == '\r' || key == '\n') {
        Serial.println("[PIN] 키보드 Enter — PIN 검증");

        // PIN 검증
        bool ok = false;
        if (g_targetMode == SystemMode::MANAGER) {
            ok = systemController.enterManagerMode(g_pinInput);
        } else {
            ok = systemController.enterDeveloperMode(g_pinInput);
        }

        g_pinLen = 0;
        memset(g_pinInput, 0, sizeof(g_pinInput));
        g_pinActive = false;

        if (ok) {
            uiManager.showToast(
                (g_targetMode == SystemMode::MANAGER)
                    ? "관리자 모드" : "개발자 모드",
                COLOR_MANAGER);
        } else {
            // 잠금 여부 확인
            if (systemController.isLockedOut()) {
                g_locked    = true;
                g_lockEndMs = millis() + systemController.getLockoutRemainingTime();
                drawPinInputScreen();
                return;
            }
            uiManager.showToast("PIN 오류", COLOR_DANGER);
        }

        if (g_callback) g_callback(ok, g_targetMode);
        uiManager.requestRedraw();
        return;
    }

    // Backspace / Delete (지우기) — ← 버튼과 동일
    if (key == 0x08 || key == 0x7F) {
        if (g_pinLen > 0) {
            g_pinLen--;
            g_pinInput[g_pinLen] = '\0';
            drawPinInputScreen();
            Serial.printf("[PIN] 키보드 Backspace (길이: %d)\n", g_pinLen);
        }
        return;
    }

    // ESC (취소) — 취소 버튼과 동일
    if (key == 0x1B) {
        Serial.println("[PIN] 키보드 ESC — 취소");
        g_pinActive = false;
        g_pinLen    = 0;
        memset(g_pinInput, 0, sizeof(g_pinInput));
        if (g_callback) g_callback(false, g_targetMode);
        uiManager.requestRedraw();
        return;
    }
}