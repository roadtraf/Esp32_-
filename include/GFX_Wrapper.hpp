// ============================================================
// GFX_Wrapper.hpp
// LovyanGFX → Arduino_GFX API 래퍼
// Waveshare ESP32-S3-Touch-LCD-3.5B (AXS15231B QSPI)
// ============================================================
#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <stdarg.h>

#ifndef LCD_QSPI_CS
#  define LCD_QSPI_CS  12
#endif
#ifndef LCD_QSPI_CLK
#  define LCD_QSPI_CLK  5
#endif
#ifndef LCD_QSPI_D0
#  define LCD_QSPI_D0   1
#endif
#ifndef LCD_QSPI_D1
#  define LCD_QSPI_D1   2
#endif
#ifndef LCD_QSPI_D2
#  define LCD_QSPI_D2   3
#endif
#ifndef LCD_QSPI_D3
#  define LCD_QSPI_D3   4
#endif
#ifndef LCD_BL_PIN
#  define LCD_BL_PIN    6
#endif
#ifndef I2C_SDA_PIN
#  define I2C_SDA_PIN   7
#endif
#ifndef I2C_SCL_PIN
#  define I2C_SCL_PIN   8
#endif

#define AXS15231B_TOUCH_ADDR  0x3B
#define AXS15231B_TOUCH_REG   0x02

#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_ORANGE  0xFD20
#define TFT_GREY    0x8410
#define TFT_DARKGREY 0x4208
#define TFT_NAVY    0x000F
#define TFT_MAROON  0x7800
#define TFT_PURPLE  0x780F
#define TFT_OLIVE   0x7BE0
#define TFT_DARKCYAN 0x03EF
#define TFT_TRANSPARENT 0xFFFF

class TFT_GFX {
public:
    TFT_GFX() {}
    bool begin() {
        _bus = new Arduino_ESP32QSPI(
            LCD_QSPI_CS, LCD_QSPI_CLK,
            LCD_QSPI_D0, LCD_QSPI_D1,
            LCD_QSPI_D2, LCD_QSPI_D3);
        _panel = new Arduino_AXS15231B(
            _bus, -1, 0, false, 320, 480);
        // no canvas
        _gfx = _panel;
        Serial.println("GFX: begin start"); Serial.flush();
        if (!_gfx->begin()) {
            Serial.println("GFX: begin FAIL"); Serial.flush();
            return false;
        }
        Serial.println("GFX: begin OK"); Serial.flush();
        pinMode(LCD_BL_PIN, OUTPUT);
        analogWrite(LCD_BL_PIN, 200);
        return true;
    }
    void init() { this->begin(); }
    void flush() { /* direct panel - no flush needed */ }
    void setBrightness(uint8_t val) { analogWrite(LCD_BL_PIN, val); }
    void fillScreen(uint16_t color) { _gfx->fillScreen(color); }
    void drawPixel(int32_t x, int32_t y, uint16_t color) { _gfx->drawPixel(x, y, color); }
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) { _gfx->drawLine(x0, y0, x1, y1, color); }
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) { _gfx->drawFastHLine(x, y, w, color); }
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) { _gfx->drawFastVLine(x, y, h, color); }
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) { _gfx->drawRect(x, y, w, h, color); }
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) { _gfx->fillRect(x, y, w, h, color); }
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) { _gfx->drawRoundRect(x, y, w, h, r, color); }
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) { _gfx->fillRoundRect(x, y, w, h, r, color); }
    void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color) { _gfx->drawCircle(x, y, r, color); }
    void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) { _gfx->fillCircle(x, y, r, color); }
    void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) { _gfx->drawTriangle(x0, y0, x1, y1, x2, y2, color); }
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) { _gfx->fillTriangle(x0, y0, x1, y1, x2, y2, color); }
    void setFont(const GFXfont* font = nullptr) { _gfx->setFont(font); _font = font; }
    void setTextSize(uint8_t s) { _gfx->setTextSize(s); _textSize = s; }
    void setTextColor(uint16_t fg) { _fgColor = fg; _bgColor = TFT_TRANSPARENT; _gfx->setTextColor(fg); }
    void setTextColor(uint16_t fg, uint16_t bg) { _fgColor = fg; _bgColor = bg; _gfx->setTextColor(fg, bg); }
    void setCursor(int32_t x, int32_t y) { _cursorX = x; _cursorY = y; _gfx->setCursor(x, y); }
    template<typename T> void print(T val)   { _gfx->print(val); }
    template<typename T> void println(T val) { _gfx->println(val); }
    void print(const char* s)   { _gfx->print(s); }
    void println(const char* s) { _gfx->println(s); }
    void println()              { _gfx->println(); }
    void printf(const char* fmt, ...) {
        char buf[256]; va_list args;
        va_start(args, fmt); vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
        _gfx->print(buf);
    }
    void drawString(const char* str, int32_t x, int32_t y) { _gfx->setCursor(x, y); _gfx->print(str); }
    void drawString(const String& str, int32_t x, int32_t y) { drawString(str.c_str(), x, y); }
    void drawString(const char* str, int32_t x, int32_t y, uint8_t) { drawString(str, x, y); }
    void drawCentreString(const char* str, int32_t cx, int32_t y, uint8_t = 1) {
        int16_t tw = _textWidth(str); _gfx->setCursor(cx - tw / 2, y); _gfx->print(str);
    }
    void drawCentreString(const String& str, int32_t cx, int32_t y, uint8_t font = 1) { drawCentreString(str.c_str(), cx, y, font); }
    void drawRightString(const char* str, int32_t x, int32_t y, uint8_t = 1) {
        int16_t tw = _textWidth(str); _gfx->setCursor(x - tw, y); _gfx->print(str);
    }
    void drawRightString(const String& str, int32_t x, int32_t y, uint8_t font = 1) { drawRightString(str.c_str(), x, y, font); }
    int16_t textWidth(const char* str)   { return _textWidth(str); }
    int16_t textWidth(const String& str) { return _textWidth(str.c_str()); }
    int16_t fontHeight() {
        if (_font) return (int16_t)(_font->yAdvance * _textSize);
        return (int16_t)(8 * _textSize);
    }
    void setRotation(uint8_t r) { _gfx->setRotation(r); }
    int16_t width()  { return _gfx->width(); }
    int16_t height() { return _gfx->height(); }
    void startWrite() { _gfx->startWrite(); }
    void endWrite()   { _gfx->endWrite(); }
    void setSwapBytes(bool) {}
    void pushColors(uint16_t* data, uint32_t len, bool = true) { _gfx->draw16bitRGBBitmap(_cursorX, _cursorY, data, len, 1); }
    void pushColors(uint8_t* data, uint32_t len, bool swap = true) { pushColors(reinterpret_cast<uint16_t*>(data), len / 2, swap); }
    bool getTouch(uint16_t* x, uint16_t* y) {
        uint8_t buf[7] = {0};
        Wire.beginTransmission(AXS15231B_TOUCH_ADDR);
        Wire.write(AXS15231B_TOUCH_REG);
        if (Wire.endTransmission(false) != 0) return false;
        Wire.requestFrom(AXS15231B_TOUCH_ADDR, 7);
        for (int i = 0; i < 7 && Wire.available(); i++) buf[i] = Wire.read();
        uint8_t points = buf[0] & 0x0F;
        if (points == 0) return false;
        *x = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
        *y = ((uint16_t)(buf[4] & 0x0F) << 8) | buf[5];
        if (*x >= 320) *x = 319;
        if (*y >= 480) *y = 479;
        return true;
    }
    Arduino_GFX* gfx() { return _gfx; }

private:
    Arduino_ESP32QSPI*  _bus    = nullptr;
    Arduino_AXS15231B*  _panel  = nullptr;
    Arduino_Canvas*     _canvas = nullptr;
    Arduino_GFX*        _gfx    = nullptr;
    const GFXfont* _font     = nullptr;
    uint8_t        _textSize = 1;
    uint16_t       _fgColor  = TFT_WHITE;
    uint16_t       _bgColor  = TFT_TRANSPARENT;
    int32_t        _cursorX  = 0;
    int32_t        _cursorY  = 0;
    int16_t _textWidth(const char* str) {
        int16_t x1, y1; uint16_t w, h;
        _gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        return (int16_t)w;
    }
};

#ifdef GFX_WRAPPER_IMPLEMENTATION
TFT_GFX tft;
#else
extern TFT_GFX tft;
#endif