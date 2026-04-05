// ================================================================
// UI_Screen_Alarm.cpp - 경보 화면 개선판
// [U6] 조치 가이드 추가
// [U7] strlen()*6 → tft.textWidth() 교체
// [U8] screenNeedsRedraw/currentScreen → uiManager 경유
// ================================================================
#include "../include/UIComponents.h"
#include "../include/UITheme.h"
#include "../include/UIManager.h"
#include "../include/Config.h"

using namespace UIComponents;
using namespace UITheme;

extern LGFX             tft;
extern UIManager        uiManager;
extern bool             errorActive;
extern ErrorInfo        currentError;
extern ErrorInfo        errorHistory[];
extern uint8_t          errorHistIdx;
extern uint8_t          errorHistCnt;

// ================================================================
// [U6] 에러 코드별 조치 가이드 매핑
// ================================================================
struct ActionGuide {
    uint16_t codeMin;
    uint16_t codeMax;
    const char* step1;
    const char* step2;
    const char* step3;
};

static const ActionGuide ACTION_GUIDES[] = {
    // 압력 관련 (E100~E199)
    {100, 149, "펌프 동작 확인",      "배관 누기 점검",     "압력 센서 점검"},
    {150, 199, "트립 값 확인",         "펌프 즉시 정지",     "원인 제거 후 재시작"},
    // 온도 관련 (E200~E299)
    {200, 249, "냉각 계통 점검",        "통풍구 막힘 확인",   "주변 온도 확인"},
    {250, 299, "즉시 전원 차단",        "열 손상 부품 확인",  "냉각 후 재기동"},
    // 통신 관련 (E300~E399)
    {300, 399, "네트워크 연결 확인",    "MQTT 브로커 확인",   "ESP 재시작 고려"},
    // 센서 관련 (E400~E499)
    {400, 499, "센서 배선 점검",        "센서 교체 고려",     "캘리브레이션 실행"},
    // 기본값
    {0,   999, "시스템 로그 확인",      "관리자에게 문의",    "필요시 재시작"},
};

static const ActionGuide* findGuide(uint16_t code) {
    for (auto& g : ACTION_GUIDES) {
        if (code >= g.codeMin && code <= g.codeMax) return &g;
    }
    return &ACTION_GUIDES[sizeof(ACTION_GUIDES) / sizeof(ACTION_GUIDES[0]) - 1];
}

// ================================================================
// 레이아웃 상수
// ================================================================
namespace AlarmLayout {
    constexpr int16_t STATUS_CARD_Y  = HEADER_HEIGHT + SPACING_SM;
    constexpr int16_t STATUS_CARD_H  = 68;

    constexpr int16_t ACTION_CARD_Y  = STATUS_CARD_Y + STATUS_CARD_H + SPACING_SM;
    constexpr int16_t ACTION_CARD_H  = 76;  // [U6] 조치 가이드 영역

    constexpr int16_t HIST_LABEL_Y   = ACTION_CARD_Y + ACTION_CARD_H + SPACING_XS;
    constexpr int16_t HIST_ITEM_H    = 38;
    constexpr int16_t HIST_ITEM_GAP  = 4;
    constexpr uint8_t HIST_MAX_SHOW  = 3;   // 3개로 줄이고 조치 카드 확보
}

// ================================================================
// 경보 화면 그리기
// ================================================================
void drawAlarmScreen() {
    tft.fillScreen(COLOR_BG_DARK);
    drawHeader("경보 / 이력");

    // ── 현재 상태 카드 ──
    {
        CardConfig card = {
            .x = SPACING_SM,
            .y = AlarmLayout::STATUS_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = AlarmLayout::STATUS_CARD_H,
            .bgColor  = errorActive ? COLOR_DANGER : COLOR_SUCCESS,
            .elevated = true
        };
        drawCard(card);

        if (errorActive) {
            drawIconWarning(card.x + CARD_PADDING, card.y + 14, COLOR_TEXT_PRIMARY);

            // 에러 제목
            tft.setTextSize(TEXT_SIZE_MEDIUM);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING);
            tft.print("경보 발생!");

            // 에러 메시지 (텍스트 너비 안전하게 잘라냄)
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 22);
            char shortMsg[40];
            strncpy(shortMsg, currentError.message, sizeof(shortMsg) - 1);
            shortMsg[sizeof(shortMsg) - 1] = '\0';
            tft.print(shortMsg);

            // 에러 코드 + 심각도 배지
            char codeBuf[8];
            snprintf(codeBuf, sizeof(codeBuf), "E%03d", currentError.code);
            drawBadge(card.x + card.w - CARD_PADDING - 40,
                      card.y + CARD_PADDING,
                      codeBuf,
                      (currentError.severity >= SEVERITY_CRITICAL)
                          ? BADGE_DANGER : BADGE_WARNING);

            // 클리어 버튼
            SystemPermissions perms = systemController.getPermissions();
            if (perms.canReset) {
                ButtonConfig clearBtn = {
                    .x = (int16_t)(card.x + card.w - 76),
                    .y = (int16_t)(card.y + CARD_PADDING + 26),
                    .w = 66,
                    .h = 26,
                    .label = "클리어",
                    .style = BTN_OUTLINE,
                    .enabled = true
                };
                drawButton(clearBtn);
            }
        } else {
            drawIconCheck(card.x + CARD_PADDING, card.y + 14, COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_SIZE_MEDIUM);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING);
            tft.print("정상 운전 중");
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(card.x + CARD_PADDING + 28, card.y + CARD_PADDING + 22);
            tft.print("시스템에 이상이 없습니다");
        }
    }

    // ── [U6] 조치 가이드 카드 (에러 활성 시) ──
    if (errorActive) {
        const ActionGuide* guide = findGuide(currentError.code);

        CardConfig card = {
            .x = SPACING_SM,
            .y = AlarmLayout::ACTION_CARD_Y,
            .w = SCREEN_WIDTH - SPACING_SM * 2,
            .h = AlarmLayout::ACTION_CARD_H,
            .bgColor = COLOR_BG_CARD,
            .borderColor = COLOR_WARNING
        };
        drawCard(card);

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_WARNING);
        tft.setCursor(card.x + CARD_PADDING, card.y + CARD_PADDING);
        tft.print("▶ 조치 방법");

        const char* steps[] = {guide->step1, guide->step2, guide->step3};
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        for (uint8_t i = 0; i < 3; i++) {
            tft.setCursor(card.x + CARD_PADDING + 8,
                          card.y + CARD_PADDING + 16 + i * 18);
            char stepBuf[48];
            snprintf(stepBuf, sizeof(stepBuf), "%d. %s", i + 1, steps[i]);
            tft.print(stepBuf);
        }
    }

    // ── 에러 이력 ──
    {
        int16_t labelY = AlarmLayout::HIST_LABEL_Y;
        if (!errorActive) {
            // 에러 없으면 조치 카드 자리에 이력 올림
            labelY = AlarmLayout::ACTION_CARD_Y;
        }

        tft.setTextSize(TEXT_SIZE_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(SPACING_SM, labelY);
        char histTitle[24];
        snprintf(histTitle, sizeof(histTitle), "이력 (%u건)", errorHistCnt);
        tft.print(histTitle);

        int16_t listY = labelY + 18;
        uint8_t showCnt = (errorHistCnt < AlarmLayout::HIST_MAX_SHOW)
                           ? errorHistCnt
                           : AlarmLayout::HIST_MAX_SHOW;

        for (uint8_t i = 0; i < showCnt; i++) {
            int16_t iy = listY + i * (AlarmLayout::HIST_ITEM_H
                                      + AlarmLayout::HIST_ITEM_GAP);

            ErrorInfo* err = &errorHistory[
                (errorHistIdx - 1 - i + ERROR_HIST_MAX) % ERROR_HIST_MAX];

            CardConfig hCard = {
                .x = SPACING_SM,
                .y = iy,
                .w = SCREEN_WIDTH - SPACING_SM * 2,
                .h = AlarmLayout::HIST_ITEM_H,
                .bgColor = COLOR_BG_CARD
            };
            drawCard(hCard);

            // 에러 코드 + 색상
            uint16_t ec = (err->severity >= SEVERITY_CRITICAL)
                          ? COLOR_DANGER
                          : (err->severity >= SEVERITY_RECOVERABLE)
                            ? COLOR_WARNING : COLOR_INFO;
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(ec);
            tft.setCursor(hCard.x + CARD_PADDING, hCard.y + CARD_PADDING);
            char codeBuf[8];
            snprintf(codeBuf, sizeof(codeBuf), "E%03d", err->code);
            tft.print(codeBuf);

            // 메시지 (최대 28자)
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(hCard.x + CARD_PADDING + 44, hCard.y + CARD_PADDING);
            char msgBuf[29];
            strncpy(msgBuf, err->message, 28);
            msgBuf[28] = '\0';
            tft.print(msgBuf);

            // 경과 시간 — [U7] 우측 정렬에 textWidth() 사용
            uint32_t elapsed = (millis() - err->timestamp) / 1000;
            char timeBuf[16];
            if (elapsed < 60)
                snprintf(timeBuf, sizeof(timeBuf), "%lu초 전", elapsed);
            else if (elapsed < 3600)
                snprintf(timeBuf, sizeof(timeBuf), "%lu분 전", elapsed / 60);
            else
                snprintf(timeBuf, sizeof(timeBuf), "%lu시간 전", elapsed / 3600);

            tft.setTextSize(1);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            int16_t tw = tft.textWidth(timeBuf);           // [U7]
            tft.setCursor(hCard.x + hCard.w - CARD_PADDING - tw,
                          hCard.y + CARD_PADDING + 16);
            tft.print(timeBuf);
        }

        if (errorHistCnt == 0) {
            tft.setTextSize(TEXT_SIZE_SMALL);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            const char* noHist = "이력 없음";
            // [U7] textWidth() 사용
            int16_t nx = (SCREEN_WIDTH - tft.textWidth(noHist)) / 2;
            tft.setCursor(nx, listY + 24);
            tft.print(noHist);
        }
    }

    // ── 하단 네비게이션 ──
    SystemPermissions perms = systemController.getPermissions();
    bool canClearAll = perms.canChangeSettings && errorHistCnt > 0;

    if (canClearAll) {
        NavButton nav[] = {
            {"뒤로",     BTN_OUTLINE, true},
            {"전체삭제", BTN_DANGER,  true}
        };
        drawNavBar(nav, 2);
    } else {
        NavButton nav[] = {{"뒤로", BTN_OUTLINE, true}};
        drawNavBar(nav, 1);
    }
}

// ================================================================
// 터치 처리  [U8]
// ================================================================
void handleAlarmTouch(uint16_t x, uint16_t y) {
    uiManager.updateActivity();

    int16_t navY = SCREEN_HEIGHT - FOOTER_HEIGHT;

    // 클리어 버튼 (에러 활성 + 권한 있을 때)
    if (errorActive && systemController.getPermissions().canReset) {
        int16_t cx = SPACING_SM + (SCREEN_WIDTH - SPACING_SM * 2) - 76;
        int16_t cy = AlarmLayout::STATUS_CARD_Y + CARD_PADDING + 26;
        if (x >= cx && x <= cx + 66 && y >= cy && y <= cy + 26) {
            clearError();
            uiManager.requestRedraw();
            return;
        }
    }

    // 네비게이션
    if (y >= navY) {
        SystemPermissions perms = systemController.getPermissions();
        int16_t bw = (SCREEN_WIDTH - SPACING_SM * 3) / 2;

        // 뒤로
        if (x < SPACING_SM + bw) {
            uiManager.setScreen(SCREEN_MAIN);   // [U8]
            return;
        }

        // 전체 삭제
        if (perms.canChangeSettings && errorHistCnt > 0 &&
            x >= SPACING_SM + bw + SPACING_SM) {
            errorHistCnt = 0;
            errorHistIdx = 0;
            uiManager.showToast("이력 삭제됨", COLOR_INFO);
            uiManager.requestRedraw();
            return;
        }
    }
}
