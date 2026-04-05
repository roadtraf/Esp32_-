#pragma once
// ================================================================
// Lang.h  —  EN / KO 다국어 지원
//
//  단순 문자열:    tft.print( L(BACK) );
//  폰트 자동 출력: printL(x, y, BACK);
//  printf 조합:   char buf[64];
//                 snprintf(buf, sizeof(buf), "%s %lu ms", L(TIM_VAC_ON), val);
//                 printL(x, y, buf);
//  언어 전환:     setLanguage(LANG_KO);
// ================================================================

enum Language : uint8_t {
  LANG_EN = 0,
  LANG_KO = 1
};

extern Language currentLang;

// ── 키 열거형 ─────────────────────────────────────────────────
enum LangKey : uint8_t {
  // 공통 버튼
  BACK, PREV, NEXT, OK, CANCEL,
  START_BTN, STOP_BTN, MENU_BTN, RESET_BTN,

  // 타이틀
  TITLE_VACUUM_CTRL, TITLE_SETTINGS, TITLE_TIMING, TITLE_PID,
  TITLE_STATISTICS, TITLE_ALARM, TITLE_ABOUT, TITLE_HELP,
  TITLE_CALIBRATION, TITLE_TREND, TITLE_STATE_DIAGRAM,

  // 메인 라벨
  LBL_STATE, LBL_PRESSURE, LBL_TARGET, LBL_CURRENT, LBL_MODE, LBL_CYCLES,
  LBL_TEMPERATURE,  // 신규: 온도

  // 설정 메뉴 항목
  MENU_TIMING, MENU_PID, MENU_STATS, MENU_TREND, MENU_CAL,
  MENU_ABOUT, MENU_HELP, MENU_STATEDIAG, MENU_LANGUAGE,

  // 타이밍
  TIM_VAC_ON, TIM_VAC_HOLD, TIM_VAC_BREAK, TIM_WAIT_REM,
  HINT_TAP,

  // PID
  PID_TARGET, PID_HYSTERESIS,

  // 통계
  STAT_TOTAL, STAT_SUCCESS, STAT_FAILED, STAT_ERRORS,
  STAT_UPTIME, STAT_MIN_PRESS, STAT_MAX_PRESS, STAT_AVG_CURR,

  // 알람
  ALARM_CODE, ALARM_SEVERITY, ALARM_MSG, ALARM_RETRY, ALARM_NONE,

  // 캘리브레이션
  CAL_PRESS_TITLE, CAL_PRESS_S1, CAL_PRESS_S2, CAL_PRESS_S3,
  CAL_CURR_TITLE,  CAL_CURR_S1,  CAL_CURR_S2,  CAL_CURR_S3,
  BTN_CAL_PRESS, BTN_CAL_CURR,

  // Trend Graph
  TREND_PRESS_LABEL, TREND_CURR_LABEL, TREND_TARGET_LIMIT,
  TREND_WRITING, TREND_NODATA, TREND_EXPORTING, TREND_SUCCESS, TREND_FAILED,
  BTN_EXPORT, BTN_SCALE, BTN_ANIM,
  LEGEND_PRESS, LEGEND_CURR,

  // State Diagram
  SD_NOW, SD_HINT, SD_GLOBAL_TRIGGER, SD_GLOBAL_DETAIL,

  // 상태명
  SN_IDLE, SN_VAC_ON, SN_VAC_HOLD, SN_VAC_BREAK,
  SN_WAIT_REM, SN_COMPLETE, SN_ERROR, SN_EMERGENCY, SN_UNKNOWN,

  // 온도 (신규)
  TEMP_NORMAL, TEMP_WARNING, TEMP_CRITICAL, TEMP_OVERHEAT,

  // 언어
  LANG_LABEL, LANG_CURRENT_NAME,

  // 팝업
  POPUP_DEL,

  LANG_KEY_COUNT   // ← 반드시 마지막
};

// ── API ───────────────────────────────────────────────────────
void        setLanguage(Language lang);
const char* L(LangKey key);                            // 키 → 문자열
void        printL(int16_t x, int16_t y, LangKey key); // 키로 폰트 자동 출력
void        printL(int16_t x, int16_t y, const char* str); // 문자열 직접 출력

// ── Help 페이지 본문 ──────────────────────────────────────────
extern const char*        HELP_TITLE[2][6];
extern const char* const* HELP_LINES[2][6];
extern const uint8_t      HELP_LINE_CNT[6];

// ── State Diagram 진입/종료 배열 ──────────────────────────────
extern const char* SD_ENTER[2][8];
extern const char* SD_EXIT[2][8];

// ── State Diagram 블록 라벨 ───────────────────────────────────
extern const char* SD_BLOCK_LABEL[2][8];
