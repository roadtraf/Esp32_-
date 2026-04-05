
#include "Config.h"                 // Arduino.h 포함
#include "LovyanGFX_Config.hpp"
#include <SPIFFS.h>

// ==================== 한글 폰트 초기화 ====================
bool initKoreanFont() {
  // VLW 폰트 로드 (SPIFFS에서)
  if (SPIFFS.exists("/NanumGothic16.vlw")) {
    // LovyanGFX는 VLW 파일을 직접 로드 가능
    // tft.loadFont("/NanumGothic16.vlw", SPIFFS);
    Serial.println("[폰트] 한글 폰트 로드 성공");
    return true;
  } else {
    Serial.println("[폰트] 한글 폰트 파일 없음");
    // 대신 내장 일본어 폰트 사용 (한글 포함)
    // tft.setFont(&fonts::lgfxJapanGothic_16);
    return false;
  }
}

// ==================== 한글 텍스트 출력 ====================
void printKoreanText(int16_t x, int16_t y, const char* text) {
  // LovyanGFX 내장 폰트 사용
  tft.setFont(&fonts::lgfxJapanGothic_16);
  tft.setCursor(x, y);
  tft.print(text);
  tft.setFont(nullptr); // 기본 폰트로 복귀
}

// 사용 예시:
// printKoreanText(10, 50, "진공 제어 시스템");
// printKoreanText(10, 80, "압력: -80 kPa");


