#pragma once
#include <Arduino.h>
// ================================================================
// Lang.h    EN / KO  
//
//   :    tft.print( L(BACK) );
//    : printL(x, y, BACK);
//  printf :   char buf[64];
//                 snprintf(buf, sizeof(buf), "%s %lu ms", L(TIM_VAC_ON), val);
//                 printL(x, y, buf);
//   :     setLanguage(LANG_KO);
// ================================================================

enum Language : uint8_t {
  LANG_EN = 0,
  LANG_KO = 1
};

extern Language currentLang;

//    
enum LangKey : uint8_t {
  //  
  BACK, PREV, NEXT, BTN_OK, BTN_CANCEL,
  START_BTN, STOP_BTN, MENU_BTN, RESET_BTN,

  // 
  TITLE_VACUUM_CTRL, TITLE_SETTINGS, TITLE_TIMING, TITLE_PID,
  TITLE_STATISTICS, TITLE_ALARM, TITLE_ABOUT, TITLE_HELP,
  TITLE_CALIBRATION, TITLE_TREND, TITLE_STATE_DIAGRAM,

  //  
  LBL_STATE, LBL_PRESSURE, LBL_TARGET, LBL_CURRENT, LBL_MODE, LBL_CYCLES,
  LBL_TEMPERATURE,  // : 

  //   
  MENU_TIMING, MENU_PID, MENU_STATS, MENU_TREND, MENU_CAL,
  MENU_ABOUT, MENU_HELP, MENU_STATEDIAG, MENU_LANGUAGE,

  // 
  TIM_VAC_ON, TIM_VAC_HOLD, TIM_VAC_BREAK, TIM_WAIT_REM,
  HINT_TAP,

  // PID
  PID_TARGET, PID_HYSTERESIS,

  // 
  STAT_TOTAL, STAT_SUCCESS, STAT_FAILED, STAT_ERRORS,
  STAT_UPTIME, STAT_MIN_PRESS, STAT_MAX_PRESS, STAT_AVG_CURR,

  // 
  ALARM_CODE, ALARM_SEVERITY, ALARM_MSG, ALARM_RETRY, ALARM_NONE,

  // 
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

  // 
  SN_IDLE, SN_VAC_ON, SN_VAC_HOLD, SN_VAC_BREAK,
  SN_WAIT_REM, SN_COMPLETE, SN_ERROR, SN_EMERGENCY, SN_UNKNOWN,

  //  ()
  TEMP_NORMAL, TEMP_WARNING, TEMP_CRITICAL, TEMP_OVERHEAT,

  // 
  LANG_LABEL, LANG_CURRENT_NAME,

  // 
  POPUP_DEL,

  LANG_KEY_COUNT   //   
};

//  API 
void        setLanguage(Language lang);
const char* L(LangKey key);                            //   
void        printL(int16_t x, int16_t y, LangKey key); //    
void        printL(int16_t x, int16_t y, const char* str); //   

//  Help   
extern const char*        HELP_TITLE[2][6];
extern const char* const* HELP_LINES[2][6];
extern const uint8_t      HELP_LINE_CNT[6];

//  State Diagram /  
extern const char* SD_ENTER[2][8];
extern const char* SD_EXIT[2][8];

//  State Diagram   
extern const char* SD_BLOCK_LABEL[2][8];
