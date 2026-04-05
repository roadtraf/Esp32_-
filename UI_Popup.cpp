// ================================================================
// UI_Popup.cpp - 숫자 입력 팝업 개선판
// [U9] +/- 단순 클릭 → long-press 연속 증가 + 빠른 입력 버튼 추가
// [U7] strlen()*N → tft.textWidth() 교체
// ================================================================
#include "../include/UIComponents.h"
#include "../include/UITheme.h"
#include "../include/UIManager.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

extern LGFX      tft;
extern UIManager uiManager;

// ================================================================
// 팝업 내부 상태
// ================================================================
namespace PopupState {
    bool       active        = false;
    float      value         = 0;
    float      minVal        = 0;
    float      maxVal        = 0;
    float      step          = 0;
    uint8_t    decimals      = 0;
    const char* label        = nullptr;
    float*     targetFloat   = nullptr;
    uint32_t*  targetU32     = nullptr;

    // [U9] Long-press 연속 증가 상태
    bool       btnPlusHeld   = false;
    bool       btnMinusHeld  = false;
    uint32_t   holdStartMs   = 0;
    uint32_t   lastRepeatMs  = 0;
    constexpr uint32_t HOLD_DELAY_MS   = 600;   // 이 시간 후 연속 시작
    constexpr uint32_t REPEAT_FAST_MS  = 80;    // 연속 증가 간격
    constexpr uint32_t REPEAT_BOOST_MS = 2000;  // 이 이상 누르면 10× 속도
}

// ================================================================
// 레이아웃 상수
// ================================================================
namespace PopupLayout {
    constexpr int16_t OX = 40;
    constexpr int16_t OY = 70;
    constexpr int16_t OW = SCREEN_WIDTH  - 80;
    constexpr int16_t OH = SCREEN_HEIGHT - 140;

    constexpr int16_t VAL_Y      = OY + 52;
    constexpr int16_t VAL_H      = 40;

    constexpr int16_t BTN_Y      = OY + OH - 60;
    constexpr int16_t BTN_H      = 44;
    constexpr int16_t BTN_MINUS_X = OX + 12;
    constexpr int16_t BTN_PLUS_X  = OX + OW - 72;
    constexpr int16_t BTN_W       = 60;

    // 빠른 값 버튼 (±10×)
    constexpr int16_t FAST_BTN_Y  = OY + OH - 110;
    constexpr int16_t FAST_BTN_H  = 32;
    constexpr int16_t FAST_BTN_W  = 52;

    // OK / Cancel
    constexpr int16_t OK_X        = OX + OW / 2 - 60;
    constexpr int16_t OK_W        = 56;
    constexpr int16_t CANCEL_X    = OX + OW / 2 + 8;
    constexpr int16_t CANCEL_W    = 56;
    constexpr int16_t OKCANCEL_Y  = BTN_Y;
    constexpr int16_t OKCANCEL_H  = BTN_H;
}

// ================================================================
// 값 클램프 헬퍼
// ================================================================
static float clampVal(float v) {
    return constrain(v, PopupState::minVal, PopupState::maxVal);
}

// ================================================================
// 값 표시 영역만 부분 갱신 (전체 재그리기 없이 빠름)
// ================================================================
static void refreshValueArea() {
    int16_t vx = PopupLayout::OX + 12;
    int16_t vy = PopupLayout::VAL_Y;
    int16_t vw = PopupLayout::OW - 24;

    tft.fillRect(vx, vy, vw, PopupLayout::VAL_H, COLOR_BG_DARK);

    char buf[24];
    if (PopupState::decimals == 0)
        snprintf(buf, sizeof(buf), "%d", (int)PopupState::value);
    else {
        char fmt[10];
        snprintf(fmt, sizeof(fmt), "%%.%df", PopupState::decimals);
        snprintf(buf, sizeof(buf), fmt, PopupState::value);
    }

    tft.setTextSize(4);
    tft.setTextColor(COLOR_PRIMARY);
    // [U7] textWidth 기반 중앙 정렬
    int16_t tw = tft.textWidth(buf);
    tft.setCursor(vx + (vw - tw) / 2, vy + 4);
    tft.print(buf);
}

// ================================================================
// 팝업 전체 그리기
// ================================================================
void drawNumericPopup() {
    using namespace PopupLayout;

    // 배경 오버레이
    tft.fillRoundRect(OX, OY, OW, OH, 10, COLOR_BG_CARD);
    tft.drawRoundRect(OX, OY, OW, OH, 10, COLOR_BORDER);

    // 제목 라벨
    tft.setTextSize(TEXT_SIZE_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    // [U7] textWidth 기반 중앙 정렬
    int16_t lw = tft.textWidth(PopupState::label ? PopupState::label : "");
    tft.setCursor(OX + (OW - lw) / 2, OY + 12);
    if (PopupState::label) tft.print(PopupState::label);

    // 범위 표시
    char rangeBuf[32];
    if (PopupState::decimals == 0)
        snprintf(rangeBuf, sizeof(rangeBuf), "(%d ~ %d)",
                 (int)PopupState::minVal, (int)PopupState::maxVal);
    else
        snprintf(rangeBuf, sizeof(rangeBuf), "(%.1f ~ %.1f)",
                 PopupState::minVal, PopupState::maxVal);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DISABLED);
    int16_t rw = tft.textWidth(rangeBuf);
    tft.setCursor(OX + (OW - rw) / 2, OY + 30);
    tft.print(rangeBuf);

    // 값 영역
    refreshValueArea();

    // [U9] 빠른 버튼 (-10× / +10×)
    float bigStep = PopupState::step * 10.0f;

    char fMinusBuf[12], fPlusBuf[12];
    if (PopupState::decimals == 0) {
        snprintf(fMinusBuf, sizeof(fMinusBuf), "-%d", (int)bigStep);
        snprintf(fPlusBuf,  sizeof(fPlusBuf),  "+%d", (int)bigStep);
    } else {
        snprintf(fMinusBuf, sizeof(fMinusBuf), "-%.1f", bigStep);
        snprintf(fPlusBuf,  sizeof(fPlusBuf),  "+%.1f", bigStep);
    }

    // 빠른 감소 버튼
    ButtonConfig fastMinus = {
        .x = (int16_t)(OX + 12),
        .y = FAST_BTN_Y,
        .w = FAST_BTN_W, .h = FAST_BTN_H,
        .label = fMinusBuf,
        .style = BTN_OUTLINE, .enabled = true
    };
    drawButton(fastMinus);

    // 빠른 증가 버튼
    ButtonConfig fastPlus = {
        .x = (int16_t)(OX + OW - 12 - FAST_BTN_W),
        .y = FAST_BTN_Y,
        .w = FAST_BTN_W, .h = FAST_BTN_H,
        .label = fPlusBuf,
        .style = BTN_OUTLINE, .enabled = true
    };
    drawButton(fastPlus);

    // 프로그레스바 (현재 값 위치)
    float pct = (PopupState::maxVal > PopupState::minVal)
                ? (PopupState::value - PopupState::minVal)
                  / (PopupState::maxVal - PopupState::minVal) * 100.0f
                : 0;
    drawProgressBar(OX + 12, FAST_BTN_Y + FAST_BTN_H + 6,
                    OW - 24, 6, pct, COLOR_PRIMARY);

    // [U9] - / + 메인 버튼 (long-press 표시)
    ButtonConfig minusBtn = {
        .x = BTN_MINUS_X, .y = BTN_Y,
        .w = BTN_W, .h = BTN_H,
        .label = "−",
        .style = BTN_DANGER, .enabled = true
    };
    drawButton(minusBtn);

    ButtonConfig plusBtn = {
        .x = BTN_PLUS_X, .y = BTN_Y,
        .w = BTN_W, .h = BTN_H,
        .label = "+",
        .style = BTN_SUCCESS, .enabled = true
    };
    drawButton(plusBtn);

    // 안내 텍스트
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_DISABLED);
    const char* hint = "길게 누르면 빠르게 변경";
    int16_t hw = tft.textWidth(hint);
    tft.setCursor(OX + (OW - hw) / 2, BTN_Y - 12);
    tft.print(hint);

    // OK / Cancel
    ButtonConfig okBtn = {
        .x = OK_X, .y = OKCANCEL_Y,
        .w = OK_W, .h = OKCANCEL_H,
        .label = "확인",
        .style = BTN_PRIMARY, .enabled = true
    };
    drawButton(okBtn);

    ButtonConfig cancelBtn = {
        .x = CANCEL_X, .y = OKCANCEL_Y,
        .w = CANCEL_W, .h = OKCANCEL_H,
        .label = "취소",
        .style = BTN_OUTLINE, .enabled = true
    };
    drawButton(cancelBtn);
}

// ================================================================
// 팝업 터치 처리 (long-press 연속 증가 포함)  [U9]
// ================================================================
void handleNumericPopupTouch(uint16_t x, uint16_t y) {
    using namespace PopupLayout;
    using namespace PopupState;

    // 빠른 버튼 (-10× / +10×)
    float bigStep = step * 10.0f;

    if (x >= OX + 12 && x <= OX + 12 + FAST_BTN_W &&
        y >= FAST_BTN_Y && y <= FAST_BTN_Y + FAST_BTN_H) {
        value = clampVal(value - bigStep);
        refreshValueArea();
        return;
    }
    if (x >= OX + OW - 12 - FAST_BTN_W &&
        x <= OX + OW - 12 &&
        y >= FAST_BTN_Y && y <= FAST_BTN_Y + FAST_BTN_H) {
        value = clampVal(value + bigStep);
        refreshValueArea();
        return;
    }

    // - 버튼 (long-press 시작)
    if (x >= BTN_MINUS_X && x <= BTN_MINUS_X + BTN_W &&
        y >= BTN_Y       && y <= BTN_Y + BTN_H) {
        btnMinusHeld = true;
        btnPlusHeld  = false;
        holdStartMs  = millis();
        value = clampVal(value - step);
        refreshValueArea();
        return;
    }

    // + 버튼 (long-press 시작)
    if (x >= BTN_PLUS_X && x <= BTN_PLUS_X + BTN_W &&
        y >= BTN_Y      && y <= BTN_Y + BTN_H) {
        btnPlusHeld  = true;
        btnMinusHeld = false;
        holdStartMs  = millis();
        value = clampVal(value + step);
        refreshValueArea();
        return;
    }

    // 버튼 영역 밖 터치 → long-press 해제
    btnPlusHeld  = false;
    btnMinusHeld = false;

    // OK
    ButtonConfig okBtn = {
        .x = OK_X, .y = OKCANCEL_Y, .w = OK_W, .h = OKCANCEL_H,
        .label = "", .style = BTN_PRIMARY, .enabled = true
    };
    if (isButtonPressed(okBtn, x, y)) {
        if (targetFloat) *targetFloat = value;
        if (targetU32)   *targetU32   = (uint32_t)value;
        saveConfig();
        active = false;
        uiManager.requestRedraw();
        return;
    }

    // Cancel
    ButtonConfig cancelBtn = {
        .x = CANCEL_X, .y = OKCANCEL_Y, .w = CANCEL_W, .h = OKCANCEL_H,
        .label = "", .style = BTN_OUTLINE, .enabled = true
    };
    if (isButtonPressed(cancelBtn, x, y)) {
        active = false;
        uiManager.requestRedraw();
        return;
    }
}

// ================================================================
// [U9] Long-press 연속 증가 루프 처리
//      UIManager::update() 에서 매 프레임 호출
// ================================================================
void updatePopupLongPress() {
    using namespace PopupState;
    if (!active) return;
    if (!btnPlusHeld && !btnMinusHeld) return;

    uint32_t now      = millis();
    uint32_t heldMs   = now - holdStartMs;

    if (heldMs < HOLD_DELAY_MS) return;

    uint32_t interval = (heldMs > REPEAT_BOOST_MS)
                        ? REPEAT_FAST_MS / 4   // 매우 빠름
                        : REPEAT_FAST_MS;

    if (now - lastRepeatMs < interval) return;
    lastRepeatMs = now;

    float s = (heldMs > REPEAT_BOOST_MS) ? step * 5.0f : step;

    if (btnPlusHeld)  value = clampVal(value + s);
    if (btnMinusHeld) value = clampVal(value - s);

    refreshValueArea();
}

// ================================================================
// 진입점: float / uint32 팝업 열기
// ================================================================
void openNumericPopup(const char* lbl, float curVal,
                      float minV, float maxV,
                      float stp, uint8_t dec, float* tgt) {
    PopupState::active       = true;
    PopupState::value        = curVal;
    PopupState::minVal       = minV;
    PopupState::maxVal       = maxV;
    PopupState::step         = stp;
    PopupState::decimals     = dec;
    PopupState::label        = lbl;
    PopupState::targetFloat  = tgt;
    PopupState::targetU32    = nullptr;
    PopupState::btnPlusHeld  = false;
    PopupState::btnMinusHeld = false;
    drawNumericPopup();
}

void openNumericPopupU32(const char* lbl, uint32_t curVal,
                         uint32_t minV, uint32_t maxV,
                         uint32_t stp, uint32_t* tgt) {
    PopupState::active       = true;
    PopupState::value        = (float)curVal;
    PopupState::minVal       = (float)minV;
    PopupState::maxVal       = (float)maxV;
    PopupState::step         = (float)stp;
    PopupState::decimals     = 0;
    PopupState::label        = lbl;
    PopupState::targetFloat  = nullptr;
    PopupState::targetU32    = tgt;
    PopupState::btnPlusHeld  = false;
    PopupState::btnMinusHeld = false;
    drawNumericPopup();
}

bool isNumericPopupActive() {
    return PopupState::active;
}

// ================================================================
// 통합 팝업 터치 처리 (기존 handlePopupTouch 대체)
// ================================================================
bool handlePopupTouch(uint16_t x, uint16_t y) {
#ifdef ENABLE_PREDICTIVE_MAINTENANCE
    if (handleMaintenanceAlertTouch(x, y)) return true;
#endif
    if (PopupState::active) {
        handleNumericPopupTouch(x, y);
        return true;
    }
    return false;
}
