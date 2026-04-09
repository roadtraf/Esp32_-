// ================================================================
// Lang.cpp         
// ================================================================
#include "Lang.h"
#include "GFX_Wrapper.hpp"  // tft (LGFX )

Language currentLang = LANG_EN;

//      
//    lgfxJapanGothic_16  ,
//   (nullptr) .
//  :   0xE0   UTF-8  .
static bool hasKorean(const char* s) {
  while (*s) {
    if ((unsigned char)*s >= 0xE0) return true;
    s++;
  }
  return false;
}

void printL(int16_t x, int16_t y, const char* str) {
  if (hasKorean(str)) {
    tft.setFont(nullptr);
    tft.setTextSize(2);
  } else {
    tft.setFont(nullptr);   //  
  }
  tft.setCursor(x, y);
  tft.print(str);
  tft.setFont(nullptr);     // 
}

void printL(int16_t x, int16_t y, LangKey key) {
  printL(x, y, L(key));
}

//  EN  
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

//  KO  
static const char* STR_KO[LANG_KEY_COUNT] = {
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 

  " ",//  
  "",// 
  " ",//  
  "PID ",// PID 
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 
  " ",//  
  " ",//  

  ":",// :
  ":",// :  ()
  ":",// :   
  ":",// :
  ":",// :
  ":",// :
  ":",// :

  "1.  ",//  
  "2. PID ",// PID 
  "3. ",// 
  "4.  ",//  
  "5. ",// 
  "6. ",// 
  "7. ",// 
  "8.  ",//  
  "9. : ",// : 

  " ON :",//  ON :
  "  :",//   :
  "  :",//   :
  "  :",//   :
  "^   ",//   

  " :",//  :
  ":",// :

  " :",//  :
  ":",// :
  ":",// :
  " :",//  :
  "  ():",//   (sec):
  " :",//  :
  " :",//  :
  " :",//  :   

  " :",//  :
  ":",// :
  ":",// :
  " :",//  :
  "  ",//      

  "1.   ",  // CAL_PRESS_TITLE
  "-   ",//   
  "-   ",//   
  "- CAL PRESSURE  ",//  
  "2.   ",  // CAL_CURR_TITLE
  "-   ",//      
  "-    ",//    
  "- CAL CURRENT  ",//  
  " ",//  
  " ",//  

  " (kPa)",//  (kPa)
  " (A)",//  (A)
  "/",// /
  " ...",//  ...
  " !",//  !   
  " ...",//  ...
  "!",// !
  "!",// !
  "",// 
  "",// 
  "",// 
  "",// 
  "",// 

  ":",// :   
  "   ",//      
  " :",//  :
  "(>6A)->ERR | ->EMRG",  // SD_GLOBAL_DETAIL

  "",// 
  " ON",//  ON
  " ",//  
  " ",//  
  " ",//  
  "",// 
  "",// 
  " ",//  
  "  ",//   

  "",// 
  "",// 
  "",// 
  "!",// !

  "",// 
  "",// 

  "",// 
};

//  L() 
const char* L(LangKey key) {
  if (key >= LANG_KEY_COUNT) return "?";
  return (currentLang == LANG_KO) ? STR_KO[key] : STR_EN[key];
}

void setLanguage(Language lang) {
  currentLang = lang;
}

// 
//  Help  
// 
//  EN 
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

//  KO 
static const char* HP0_KO[] = {
  "\uAE30\uBD80 \uC791\uB3A8:",                                     //  :
  "1. START \uB610\uB294 \uB9AC\uBD80\uC2A4\uC704\uC2A4\uB85C \uC2DC\uC791", // 
  "2. \uC9C4\uACF5 ON \uB8DC\uAC80 \uC2DC\uC791",                  //  ON 
  "3. \uC9C4\uACF5 \uC720\uC9C0 \uB8DC\uAC80 (\uC815\uB9D1 \uC720\uC9C0)", // 
  "4. \uC9C4\uACF5 \uD574\uC81C \uB8DC\uAC80 (\uC815\uB9D1 \uD574\uC81C)", // 
  "5. \uC81C\uD568 \uC81C\uC815  (\uAD80\uC77C )", //  
  "6. \uC6B3\uC740 - \uB2E4\uC740 \uC0AC\uC774\uD074 \uC885\uBE58", // 
};
static const char* HP1_KO[] = {
  "\uC81C\uC5B4 \uBD80\uB3C4:",                                     //  :
  "\uC218\uB3A8: \uACE0\uC815 PWM, \uD3B8\uBD80\uB9D1 \uC5C6\uC740", // 
  "\uC790\uB3A8: \uC2DC\uC04C , \uC548\uC815\uC801",   // 
  "PID: \uC815\uB9D1 \uD3B8\uBD80\uB9D1 \uC81C\uC5B4",             // PID
  "     \uAC80\uC7B0 \uC815\uBC00\uB9D1 \uC81C\uC5B4",             // 
};
static const char* HP2_KO[] = {
  "USB  \uC81C\uC5B4:",                          // USB  :
  "1:\uC2DC\uC791 2:\uC815\uC9C0  3:\uBD80\uB3C4",                 //   
  "4:\uB9AC\uC0C1 5:\uD1B5\uC81C  6:\uC815\uBCF4",                 //   
  "7:\uD0DC\uC774\uB9BC 8:\uADF8\uB798\uD504 9:\uB3C4\uC6B8\uB9D1", //   
  "0:\uBD80\uB8CC *:\uBD80\uB450  .:\uB2E4\uC774\uC5B4\uADF8\uB78C", //   
  "+/-: \uBD80\uC9C4 \uD0DC\uC7B4  BS: \uBF50\uB85C",              //   
};
static const char* HP3_KO[] = {
  "\uC548\uC804 \uAE30\uB2A5:",                                     //  :
  "- \uBE44\uC0C8\uC815\uC9C0 \uBE44\uD1B0 (NC)",                  //  
  "- \uACFC\uC804\uB978 \uBD80\uD638 (6.0A)",                      //  
  "- \uD3B8\uBD80/\uBC18\uB81C \uC775\uC0A4\uB85C\uAE30",         // / 
  "-  \uC0C1\uD0DC \uBBA8\uC2D4\uB9D1\uAE30",         //   
  "- \uC6DC\uCED8\uB3A8 \uD0DC\uC774\uBFA8 (10\uC804)",           // dog 
  "- \uC790\uB3A8 \uC5D0\uB9AC \uBD80\uAE30",                     //   
  "- \uC774\uC911 \uC804\uC6B0 \uC81C\uB8FC CH3+CH4",             //   
};
static const char* HP4_KO[] = {
  "\uB3C4\uC774 \uB85C\uAE30:",                                     //  :
  "\uC0AC\uC774\uD074: /logs/cycle_log.csv",                       //  
  "\uC5D0\uB9AC: /logs/error_log.csv",                             //  
  "\uC77C\uBB50: /reports/daily_YYYYMMDD",                         //  
  "\uC790\uC815\uC5B4 \uC790\uB3A8 \uC0DD\uC131",                 //   
};
static const char* HP5_KO[] = {
  "\uB124\uD2B8\uC6B0\uAE30 \uAE30\uB2A5:",                       //  :
  "WiFi: \uC790\uB3A8 \uC7AC\uC5B4\uC5D8\uC81C\uC7A5",           //  
  "MQTT: \uC0C1\uD0DC \uBD80\uC5B8 (2\uC804), \uC6D0\uAC83 \uBD80\uB9BC", //    
  "OTA: \uBD80\uC740 \uD3B8\uC6B0\uC5B4 \uC5E5\uB370\uC774\uD2B8", //   
  "     HTTP \uC11C\uBE44\uC2A4 \uBD80\uD1B0 80",                  // HTTP   80
};

const char* HELP_TITLE[2][6] = {
  { "Basic Operation", "Control Modes", "Keypad Controls",
    "Safety Features", "Data Logging",  "Network Features" },
  { "\uAE30\uBD80 \uC791\uB3A8", "\uC81C\uC5B4 \uBD80\uB3C4", "USB  \uC81C\uC5B4",
    "\uC548\uC804 \uAE30\uB2A5", "\uB3C4\uC774 \uB85C\uAE30",  "\uB124\uD2B8\uC6B0\uAE30 \uAE30\uB2A5" },
};

const char* const* HELP_LINES[2][6] = {
  { HP0_EN, HP1_EN, HP2_EN, HP3_EN, HP4_EN, HP5_EN },
  { HP0_KO, HP1_KO, HP2_KO, HP3_KO, HP4_KO, HP5_KO },
};

const uint8_t HELP_LINE_CNT[6] = { 7, 5, 6, 8, 5, 5 };

// 
//  State Diagram   
// 
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
    "\uC9C4\uC785: START, \uBD80\uAE30\uC6B3\uC740, \uBE44\uC0C8\uBD80\uAE30",  // : START, , 
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

//    
const char* SD_BLOCK_LABEL[2][8] = {
  { "IDLE", "VAC ON", "VAC HOLD", "VAC BRK", "WAIT", "DONE", "ERROR", "EMRG" },
  { "", "\uC9C4\uACF5ON", "\uC720\uC9C0", "\uD574\uC81C",
    "\uC81C\uD568\uB3C0", "\uC6B3\uC740", "\uC5D0\uB9AC", "\uBE44\uC0C8" },
};
