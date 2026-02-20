// ================================================================
// Lang.cpp  —  다국어 문자열 테이블 및 출력 헬퍼
// ================================================================
#include "Lang.h"
#include "LovyanGFX_Config.hpp"   // tft (LGFX 인스턴스)

Language currentLang = LANG_EN;

// ─── 폰트 자동 전환 출력 ──────────────────────────────────────
// 한글 문자열이 포함되면 lgfxJapanGothic_16 폰트 사용,
// 아니면 기본 폰트(nullptr)로 유지.
// 간단한 판별: 첫 바이트가 0xE0 이상이면 한글 UTF-8 가능성 높음.
static bool hasKorean(const char* s) {
  while (*s) {
    if ((unsigned char)*s >= 0xE0) return true;
    s++;
  }
  return false;
}

void printL(int16_t x, int16_t y, const char* str) {
  if (hasKorean(str)) {
    tft.setFont(&fonts::lgfxJapanGothic_16);
  } else {
    tft.setFont(nullptr);   // 기본 폰트
  }
  tft.setCursor(x, y);
  tft.print(str);
  tft.setFont(nullptr);     // 복귀
}

void printL(int16_t x, int16_t y, LangKey key) {
  printL(x, y, L(key));
}

// ─── EN 테이블 ────────────────────────────────────────────────
static const char* STR_EN[LANG_KEY_COUNT] = {
  /* BACK */              "BACK",
  /* PREV */              "PREV",
  /* NEXT */              "NEXT",
  /* OK */                "OK",
  /* CANCEL */            "CANCEL",
  /* START_BTN */         "START",
  /* STOP_BTN */          "STOP",
  /* MENU_BTN */          "MENU",
  /* RESET_BTN */         "RESET",

  /* TITLE_VACUUM_CTRL */ "VACUUM CONTROL",
  /* TITLE_SETTINGS */    "SETTINGS",
  /* TITLE_TIMING */      "TIMING SETUP",
  /* TITLE_PID */         "PID SETUP",
  /* TITLE_STATISTICS */  "STATISTICS",
  /* TITLE_ALARM */       "ALARM",
  /* TITLE_ABOUT */       "ABOUT",
  /* TITLE_HELP */        "HELP",
  /* TITLE_CALIBRATION */ "CALIBRATION",
  /* TITLE_TREND */       "TREND GRAPH",
  /* TITLE_STATE_DIAGRAM*/"STATE DIAGRAM",

  /* LBL_STATE */         "STATE:",
  /* LBL_PRESSURE */      "PRESSURE:",
  /* LBL_TARGET */        "TARGET:",
  /* LBL_CURRENT */       "CURRENT:",
  /* LBL_MODE */          "MODE:",
  /* LBL_CYCLES */        "CYCLES:",
  /* LBL_TEMPERATURE */   "TEMP:",

  /* MENU_TIMING */       "1. Timing Setup",
  /* MENU_PID */          "2. PID Setup",
  /* MENU_STATS */        "3. Statistics",
  /* MENU_TREND */        "4. Trend Graph",
  /* MENU_CAL */          "5. Calibration",
  /* MENU_ABOUT */        "6. About",
  /* MENU_HELP */         "7. Help",
  /* MENU_STATEDIAG */    "8. State Diagram",
  /* MENU_LANGUAGE */     "9. Language: EN",

  /* TIM_VAC_ON */        "Vac On Time:",
  /* TIM_VAC_HOLD */      "Vac Hold Time:",
  /* TIM_VAC_BREAK */     "Vac Break Time:",
  /* TIM_WAIT_REM */      "Wait Rem Time:",
  /* HINT_TAP */          "^ Tap value to edit",

  /* PID_TARGET */        "Target Pressure:",
  /* PID_HYSTERESIS */    "Hysteresis:",

  /* STAT_TOTAL */        "Total Cycles:",
  /* STAT_SUCCESS */      "Successful:",
  /* STAT_FAILED */       "Failed:",
  /* STAT_ERRORS */       "Total Errors:",
  /* STAT_UPTIME */       "Uptime (sec):",
  /* STAT_MIN_PRESS */    "Min Pressure:",
  /* STAT_MAX_PRESS */    "Max Pressure:",
  /* STAT_AVG_CURR */     "Avg Current:",

  /* ALARM_CODE */        "Error Code:",
  /* ALARM_SEVERITY */    "Severity:",
  /* ALARM_MSG */         "Message:",
  /* ALARM_RETRY */       "Retry Count:",
  /* ALARM_NONE */        "No Active Alarms",

  /* CAL_PRESS_TITLE */   "1. Calibrate Pressure Sensor",
  /* CAL_PRESS_S1 */      "- Remove vacuum load",
  /* CAL_PRESS_S2 */      "- Ensure atmospheric pressure",
  /* CAL_PRESS_S3 */      "- Press CAL PRESSURE button",
  /* CAL_CURR_TITLE */    "2. Calibrate Current Sensor",
  /* CAL_CURR_S1 */       "- Turn off all loads",
  /* CAL_CURR_S2 */       "- Ensure zero current",
  /* CAL_CURR_S3 */       "- Press CAL CURRENT button",
  /* BTN_CAL_PRESS */     "CAL PRESSURE",
  /* BTN_CAL_CURR */      "CAL CURRENT",

  /* TREND_PRESS_LABEL */ "Pressure (kPa)",
  /* TREND_CURR_LABEL */  "Current (A)",
  /* TREND_TARGET_LIMIT*/ "Target/Limit",
  /* TREND_WRITING */     "Writing...",
  /* TREND_NODATA */      "No data!",
  /* TREND_EXPORTING */   "Exporting...",
  /* TREND_SUCCESS */     "Success!",
  /* TREND_FAILED */      "Failed!",
  /* BTN_EXPORT */        "EXPORT",
  /* BTN_SCALE */         "SCALE",
  /* BTN_ANIM */          "ANIM",
  /* LEGEND_PRESS */      "Pressure",
  /* LEGEND_CURR */       "Current",

  /* SD_NOW */            "NOW:",
  /* SD_HINT */           "Tap a state to see conditions",
  /* SD_GLOBAL_TRIGGER */ "Global Trigger:",
  /* SD_GLOBAL_DETAIL */  "Overcurr(>6A)->ERR | EStop->EMRG",

  /* SN_IDLE */           "IDLE",
  /* SN_VAC_ON */         "VACUUM ON",
  /* SN_VAC_HOLD */       "VACUUM HOLD",
  /* SN_VAC_BREAK */      "VACUUM BREAK",
  /* SN_WAIT_REM */       "WAIT REMOVAL",
  /* SN_COMPLETE */       "COMPLETE",
  /* SN_ERROR */          "ERROR",
  /* SN_EMERGENCY */      "EMERGENCY",
  /* SN_UNKNOWN */        "UNKNOWN",

  /* TEMP_NORMAL */       "Normal",
  /* TEMP_WARNING */      "Warning",
  /* TEMP_CRITICAL */     "Critical",
  /* TEMP_OVERHEAT */     "Overheat!",

  /* LANG_LABEL */        "Language",
  /* LANG_CURRENT_NAME */ "English",

  /* POPUP_DEL */         "DEL",
};

// ─── KO 테이블 ────────────────────────────────────────────────
static const char* STR_KO[LANG_KEY_COUNT] = {
  "뒤로",// 뒤로
  "이전",// 이전
  "다음",// 다음
  "확인",// 확인
  "취소",// 취소
  "시작",// 시작
  "정지",// 정지
  "메뉴",// 메뉴
  "리셋",// 리셋

  "진공 제어",// 진공 제어
  "설정",// 설정
  "타이밍 설정",// 타이밍 설정
  "PID 설정",// PID 설정
  "통계",// 통계
  "알람",// 알림
  "정보",// 정보
  "도움말",// 도움말
  "캘리브레이션",// 캘리브레이션
  "추세 그래프",// 추세 그래프
  "상태 다이어그램",// 상태 다이어그램

  "상태:",// 상태:
  "압력:",// 압력:  (압→정압)
  "목표:",// 목표:  — 유지
  "전류:",// 전류:
  "모드:",// 모드:
  "사이클:",// 사이클:
  "온도:",// 온도:

  "1. 타이밍 설정",// 타이밍 설정
  "2. PID 설정",// PID 설정
  "3. 통계",// 통계
  "4. 추세 그래프",// 추세 그래프
  "5. 캘리브레이션",// 캘리브레이션
  "6. 정보",// 정보
  "7. 도움말",// 도움말
  "8. 상태 다이어그램",// 상태 다이어그램
  "9. 언어: 한글",// 언어: 한글

  "진공 ON 시간:",// 진공 ON 시간:
  "진공 유지 시간:",// 진공 유지 시간:
  "진공 해제 시간:",// 진공 해제 시간:
  "제거 대기 시간:",// 제품 대기 시간:
  "^ 값을 탭하여 편집",// 값을 터치하여 편집

  "목표 압력:",// 목표 압력:
  "히스테리시스:",// 히스테레시스:

  "총 사이클:",// 총 사이클:
  "성공:",// 성공:
  "실패:",// 실패:
  "총 에러:",// 총 에러:
  "가동 시간 (초):",// 전운 시간 (sec):
  "최소 압력:",// 최소 압력:
  "최대 압력:",// 최대 압력:
  "평균 전류:",// 평균 전류:  — 유지

  "에러 코드:",// 에러 코드:
  "심각도:",// 심각도:
  "메시지:",// 메시지:
  "재시도 횟수:",// 재시도 횟수:
  "활성 알람 없음",// 활성 알림 없음  — 유지

  "1. 압력 센서 캘리브레이션",  // CAL_PRESS_TITLE
  "- 진공 부하 제거",// 진공 부하 제거
  "- 대기압 상태 확인",// 대기압 상태 확인
  "- CAL PRESSURE 버튼 누름",// 버튼 누름
  "2. 전류 센서 캘리브레이션",  // CAL_CURR_TITLE
  "- 부하 모두 끄기",// 모든 부하 끄기  — 유지
  "- 영 전류 상태 확인",// 영 전류 상태 확인
  "- CAL CURRENT 버튼 누름",// 버튼 누름
  "압력 캘리",// 압력 캘리브레이션
  "전류 캘리",// 전류 캘리브레이션

  "압력 (kPa)",// 압력 (kPa)
  "전류 (A)",// 전류 (A)
  "목표/제한",// 목표/제한
  "저장 중...",// 저장 중...
  "데이터 없음!",// 데이터 없음!  — 유지
  "내보내기 중...",// 내보내기 중...
  "완료!",// 완료!
  "실패!",// 실패!
  "내보내기",// 내보내기
  "스케일",// 스케일
  "애니메이션",// 동작
  "압력",// 압력
  "전류",// 전류

  "현재:",// 현재:  — 유지
  "상태를 터치하면 조건 표시",// 상태블록 터치하면 조건 표시 — 유지
  "글로벌 트리거:",// 글로벌 트리거:
  "과전류(>6A)->ERR | 비상정지->EMRG",  // SD_GLOBAL_DETAIL

  "대기",// 대기
  "진공 ON",// 진공 ON
  "진공 유지",// 진공 유지
  "진공 해제",// 진공 해제
  "제거 대기",// 제품 대기
  "완료",// 완료
  "에러",// 에러
  "비상 정지",// 비상 정지
  "알 수 없음",// 알 수 없음

  "정상",// 정상
  "경고",// 경고
  "위험",// 위험
  "과열!",// 과열!

  "언어",// 언어
  "한글",// 한글

  "삭제",// 삭제
};

// ─── L() ──────────────────────────────────────────────────────
const char* L(LangKey key) {
  if (key >= LANG_KEY_COUNT) return "?";
  return (currentLang == LANG_KO) ? STR_KO[key] : STR_EN[key];
}

void setLanguage(Language lang) {
  currentLang = lang;
}

// ════════════════════════════════════════════════════════════════
//  Help 페이지 본문
// ════════════════════════════════════════════════════════════════
// ── EN ──
static const char* HP0_EN[] = {
  "BASIC OPERATION:",
  "1. Press START or limit switch",
  "2. Vacuum ON phase begins",
  "3. HOLD phase maintains pressure",
  "4. BREAK phase releases pressure",
  "5. WAIT for box removal (sensor)",
  "6. COMPLETE - ready for next cycle",
};
static const char* HP1_EN[] = {
  "CONTROL MODES:",
  "MANUAL: Fixed PWM, no feedback",
  "AUTO:   Time-based, reliable",
  "PID:    Pressure feedback ctrl",
  "        Most accurate mode",
};
static const char* HP2_EN[] = {
  "USB KEYPAD CONTROLS:",
  "1:START  2:STOP   3:MODE",
  "4:RESET  5:STATS  6:ABOUT",
  "7:TIMING 8:TREND  9:HELP",
  "0:MAIN   *:MENU   .:DIAG",
  "+/-: Page nav   BS: Back",
};
static const char* HP3_EN[] = {
  "SAFETY FEATURES:",
  "- Emergency stop (NC)",
  "- Overcurrent protect (6.0A)",
  "- Pump/valve interlock",
  "- Sensor health monitor",
  "- Watchdog timer (10 sec)",
  "- Auto error recovery",
  "- Dual power cutoff CH3+CH4",
};
static const char* HP4_EN[] = {
  "DATA LOGGING:",
  "Cycle: /logs/cycle_log.csv",
  "Error: /logs/error_log.csv",
  "Daily: /reports/daily_YYYYMMDD",
  "Reports generated at midnight",
};
static const char* HP5_EN[] = {
  "NETWORK FEATURES:",
  "WiFi: Auto reconnect",
  "MQTT: Status pub (2s), Cmds",
  "OTA:  Wireless FW update",
  "      HTTP server port 80",
};

// ── KO ──
static const char* HP0_KO[] = {
  "\uAE30\uBD80 \uC791\uB3A8:",                                     // 기본 작동:
  "1. START \uB610\uB294 \uB9AC\uBD80\uC2A4\uC704\uC2A4\uB85C \uC2DC\uC791", // 시작
  "2. \uC9C4\uACF5 ON \uB8DC\uAC80 \uC2DC\uC791",                  // 진공 ON 단계
  "3. \uC9C4\uACF5 \uC720\uC9C0 \uB8DC\uAC80 (\uC815\uB9D1 \uC720\uC9C0)", // 유지
  "4. \uC9C4\uACF5 \uD574\uC81C \uB8DC\uAC80 (\uC815\uB9D1 \uD574\uC81C)", // 해제
  "5. \uC81C\uD568 \uC81C\uC815 \uB3C0\uAE30 (\uAD80\uC77C \uC센\uC2A4)", // 제품 대기
  "6. \uC6B3\uC740 - \uB2E4\uC740 \uC0AC\uC774\uD074 \uC885\uBE58", // 완료
};
static const char* HP1_KO[] = {
  "\uC81C\uC5B4 \uBD80\uB3C4:",                                     // 제어 모드:
  "\uC218\uB3A8: \uACE0\uC815 PWM, \uD3B8\uBD80\uB9D1 \uC5C6\uC740", // 수동
  "\uC790\uB3A8: \uC2DC\uC04C \uGE30\uC0BC, \uC548\uC815\uC801",   // 자동
  "PID: \uC815\uB9D1 \uD3B8\uBD80\uB9D1 \uC81C\uC5B4",             // PID
  "     \uAC80\uC7B0 \uC815\uBC00\uB9D1 \uC81C\uC5B4",             // 정밀
};
static const char* HP2_KO[] = {
  "USB \uD82C\uD3D8\uB9D1 \uC81C\uC5B4:",                          // USB 키패드 제어:
  "1:\uC2DC\uC791 2:\uC815\uC9C0  3:\uBD80\uB3C4",                 // 시작 정지 모드
  "4:\uB9AC\uC0C1 5:\uD1B5\uC81C  6:\uC815\uBCF4",                 // 리셋 통계 정보
  "7:\uD0DC\uC774\uB9BC 8:\uADF8\uB798\uD504 9:\uB3C4\uC6B8\uB9D1", // 타이밍 그래프 도움말
  "0:\uBD80\uB8CC *:\uBD80\uB450  .:\uB2E4\uC774\uC5B4\uADF8\uB78C", // 메인 메뉴 다이어그램
  "+/-: \uBD80\uC9C4 \uD0DC\uC7B4  BS: \uBF50\uB85C",              // 페이지 탐색 뒤로
};
static const char* HP3_KO[] = {
  "\uC548\uC804 \uAE30\uB2A5:",                                     // 안전 기능:
  "- \uBE44\uC0C8\uC815\uC9C0 \uBE44\uD1B0 (NC)",                  // 비상정지 버튼
  "- \uACFC\uC804\uB978 \uBD80\uD638 (6.0A)",                      // 과전류 보호
  "- \uD3B8\uBD80/\uBC18\uB81C \uC775\uC0A4\uB85C\uAE30",         // 펌프/밸브 인터록
  "- \uC센\uC2A4 \uC0C1\uD0DC \uBBA8\uC2D4\uB9D1\uAE30",         // 센서 상태 모니터링
  "- \uC6DC\uCED8\uB3A8 \uD0DC\uC774\uBFA8 (10\uC804)",           // 워치dog 타이머
  "- \uC790\uB3A8 \uC5D0\uB9AC \uBD80\uAE30",                     // 자동 에러 복구
  "- \uC774\uC911 \uC804\uC6B0 \uC81C\uB8FC CH3+CH4",             // 이중 전원 차단
};
static const char* HP4_KO[] = {
  "\uB3C4\uC774 \uB85C\uAE30:",                                     // 데이터 로깅:
  "\uC0AC\uC774\uD074: /logs/cycle_log.csv",                       // 사이클 로그
  "\uC5D0\uB9AC: /logs/error_log.csv",                             // 에러 로그
  "\uC77C\uBB50: /reports/daily_YYYYMMDD",                         // 일별 보고서
  "\uC790\uC815\uC5B4 \uC790\uB3A8 \uC0DD\uC131",                 // 자정에 자동 생성
};
static const char* HP5_KO[] = {
  "\uB124\uD2B8\uC6B0\uAE30 \uAE30\uB2A5:",                       // 네트워크 기능:
  "WiFi: \uC790\uB3A8 \uC7AC\uC5B4\uC5D8\uC81C\uC7A5",           // 자동 재연결
  "MQTT: \uC0C1\uD0DC \uBD80\uC5B8 (2\uC804), \uC6D0\uAC83 \uBD80\uB9BC", // 상태 발행 원격 명령
  "OTA: \uBD80\uC740 \uD3B8\uC6B0\uC5B4 \uC5E5\uB370\uC774\uD2B8", // 무선 펌웨어 업데이트
  "     HTTP \uC11C\uBE44\uC2A4 \uBD80\uD1B0 80",                  // HTTP 서버 포트 80
};

const char* HELP_TITLE[2][6] = {
  { "Basic Operation", "Control Modes", "Keypad Controls",
    "Safety Features", "Data Logging",  "Network Features" },
  { "\uAE30\uBD80 \uC791\uB3A8", "\uC81C\uC5B4 \uBD80\uB3C4", "USB \uD82C\uD3D8\uB9D1 \uC81C\uC5B4",
    "\uC548\uC804 \uAE30\uB2A5", "\uB3C4\uC774 \uB85C\uAE30",  "\uB124\uD2B8\uC6B0\uAE30 \uAE30\uB2A5" },
};

const char* const* HELP_LINES[2][6] = {
  { HP0_EN, HP1_EN, HP2_EN, HP3_EN, HP4_EN, HP5_EN },
  { HP0_KO, HP1_KO, HP2_KO, HP3_KO, HP4_KO, HP5_KO },
};

const uint8_t HELP_LINE_CNT[6] = { 7, 5, 6, 8, 5, 5 };

// ════════════════════════════════════════════════════════════════
//  State Diagram — 진입·종료 배열
// ════════════════════════════════════════════════════════════════
const char* SD_ENTER[2][8] = {
  { // EN
    "Enter: START / Recovery / E-Stop",
    "Enter: limitSwitch ON / START",
    "Enter: onTime(AUTO) / Target(PID)",
    "Enter: holdTime expired",
    "Enter: breakTime expired",
    "Enter: photoSensor OFF",
    "Enter: Overcurr(>6A) / Timeout",
    "Enter: E-Stop SW (NC->LOW)",
  },
  { // KO
    "\uC9C4\uC785: START, \uBD80\uAE30\uC6B3\uC740, \uBE44\uC0C8\uBD80\uAE30",  // 진입: START, 복구완료, 비상복귀
    "\uC9C4\uC785: limitSwitch ON / START",
    "\uC9C4\uC785: onTime\uBD80\uAE30(AUTO) / \uBD80\uAC23\uC815\uB9D1(PID)",
    "\uC9C4\uC785: holdTime \uBD80\uAE30",
    "\uC9C4\uC785: breakTime \uBD80\uAE30",
    "\uC9C4\uC785: photoSensor OFF (\uC81C\uD568\uAC10\uC9C0)",
    "\uC9C4\uC785: \uACFC\uC804\uB978(>6A) / \uD0DC\uC77C\uC5FC\uC740(10s)",
    "\uC9C4\uC785: \uBE44\uC0C8\uC815\uC9C0 SW (NC->LOW)",
  },
};

const char* SD_EXIT[2][8] = {
  { // EN
    "Exit: limitSwitch ON -> VAC_ON",
    "Exit: onTime/Target -> VAC_HOLD",
    "Exit: holdTime exp -> VAC_BREAK",
    "Exit: breakTime exp -> WAIT_REM",
    "Exit: Detect->COMP / Timeout->ERR",
    "Exit: 1s auto -> IDLE",
    "Exit: Recovery -> IDLE",
    "Exit: E-Stop ret(HIGH) -> IDLE",
  },
  { // KO
    "\uC885\uC740: limitSwitch ON -> VACUUM_ON",
    "\uC885\uC740: onTime/\uBD80\uAC23\uC815\uB9D1 -> VACUUM_HOLD",
    "\uC885\uC740: holdTime\uBD80\uAE30 -> VACUUM_BREAK",
    "\uC885\uC740: breakTime\uBD80\uAE30 -> WAIT_REMOVAL",
    "\uC885\uC740: \uAC10\uC9C0->COMPLETE / \uD0DC\uC77C\uC5FC\uC740->ERROR",
    "\uC885\uC740: 1s\uD55C \uC790\uB3A8 -> IDLE",
    "\uC885\uC740: \uBD80\uAE30\uC131\uACF5 -> IDLE",
    "\uC885\uC740: \uBE44\uC0C8\uC815\uC9C0 \uBD80\uAE30(HIGH) -> IDLE",
  },
};

// ── 블록 라벨 ─────────────────────────────────────────────────
const char* SD_BLOCK_LABEL[2][8] = {
  { "IDLE", "VAC ON", "VAC HOLD", "VAC BRK", "WAIT", "DONE", "ERROR", "EMRG" },
  { "\uB3C0\uAE30", "\uC9C4\uACF5ON", "\uC720\uC9C0", "\uD574\uC81C",
    "\uC81C\uD568\uB3C0", "\uC6B3\uC740", "\uC5D0\uB9AC", "\uBE44\uC0C8" },
};
