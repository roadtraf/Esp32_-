
#include "Config.h"                 // Arduino.h 
#include "GFX_Wrapper.hpp"
#include <SPIFFS.h>

// ====================    ====================
bool initKoreanFont() {
  // VLW   (SPIFFS)
  if (SPIFFS.exists("/NanumGothic16.vlw")) {
    // LovyanGFX VLW    
    // tft.loadFont("/NanumGothic16.vlw", SPIFFS);
    Serial.println("[]    ");
    return true;
  } else {
    Serial.println("[]    ");
    //      ( )
    // tft.setFont(&fonts::lgfxJapanGothic_16);
    return false;
  }
}

// ====================    ====================
void printKoreanText(int16_t x, int16_t y, const char* text) {
  // LovyanGFX   
  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.print(text);
  tft.setFont(nullptr); //   
}

//  :
// printKoreanText(10, 50, "  ");
// printKoreanText(10, 80, ": -80 kPa");


