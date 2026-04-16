// ================================================================
// UITheme.h -  UI  
// ================================================================
#pragma once

#include <Arduino.h>

// ================================================================
//   - Material Design 
// ================================================================
namespace UITheme {
    //    
    constexpr uint16_t COLOR_PRIMARY       = 0x0E7F;  //  ()
    constexpr uint16_t COLOR_PRIMARY_DARK  = 0x0A5F;  //  
    constexpr uint16_t COLOR_SECONDARY     = 0xFD20;  //  ()
    constexpr uint16_t COLOR_ACCENT        = 0x07FF;  //   ()
    
    constexpr uint16_t COLOR_SUCCESS       = 0x07E0;  //  (/)
    constexpr uint16_t COLOR_WARNING       = 0xFFE0;  //  ()
    constexpr uint16_t COLOR_DANGER        = 0xF800;  //  ()
    constexpr uint16_t COLOR_INFO          = 0x04BF;  //  ()
    
    constexpr uint16_t COLOR_BG_DARK       = 0x0000;  //  ()
    constexpr uint16_t COLOR_BG_CARD       = 0x2124;  //   ()
    constexpr uint16_t COLOR_BG_ELEVATED   = 0x31A6;  //   ()
    
    constexpr uint16_t COLOR_TEXT_PRIMARY  = 0xFFFF;  //  ( )
    constexpr uint16_t COLOR_TEXT_SECONDARY= 0xBDF7;  //  ( )
    constexpr uint16_t COLOR_TEXT_DISABLED = 0x7BEF;  //   ()
    
    //   
    constexpr uint16_t COLOR_BORDER        = 0x4208;  //  
    constexpr uint16_t COLOR_DIVIDER       = 0x2945;  //   
    
    //    
    constexpr uint16_t COLOR_MANAGER       = 0xFBE0;  // 
    constexpr uint16_t COLOR_DEVELOPER     = 0xF81F;  // 
    
    // ================================================================
    // 
    // ================================================================
    constexpr uint8_t TEXT_SIZE_LARGE      = 3;       // 
    constexpr uint8_t TEXT_SIZE_MEDIUM     = 2;       // 
    constexpr uint8_t TEXT_SIZE_SMALL      = 1;       // 
    
    // ================================================================
    //   (8px )
    // ================================================================
    constexpr uint8_t SPACING_XS           = 4;
    constexpr uint8_t SPACING_SM           = 8;
    constexpr uint8_t SPACING_MD           = 16;
    constexpr uint8_t SPACING_LG           = 24;
    constexpr uint8_t SPACING_XL           = 32;
    
    // ================================================================
    //  
    // ================================================================
    constexpr uint16_t SCREEN_WIDTH        = 320;
    constexpr uint16_t SCREEN_HEIGHT       = 480;
    
    constexpr uint8_t  HEADER_HEIGHT       = 30;
    constexpr uint8_t  FOOTER_HEIGHT       = 40;
    constexpr uint16_t CONTENT_HEIGHT      = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;
    
    constexpr uint8_t  CARD_RADIUS         = 8;
    constexpr uint8_t  BUTTON_RADIUS       = 6;
    constexpr uint8_t  BADGE_RADIUS        = 12;
    
    // ================================================================
    //  
    // ================================================================
    constexpr uint16_t BUTTON_WIDTH_FULL   = 140;
    constexpr uint16_t BUTTON_WIDTH_HALF   = 68;
    constexpr uint16_t BUTTON_HEIGHT       = 36;
    
    constexpr uint16_t CARD_PADDING        = 12;
    
    // ================================================================
    //    (  )
    // ================================================================
    //   
    constexpr int16_t  HEALTH_ICON_X       = SCREEN_WIDTH - 60;
    constexpr int16_t  HEALTH_ICON_Y       = 5;
    constexpr int16_t  HEALTH_ICON_W       = 30;
    constexpr int16_t  HEALTH_ICON_H       = 25;
    
    //   
    constexpr int16_t  MANAGER_BADGE_OFFSET = 100;
    
    //   
    constexpr int16_t  BADGE_X_OFFSET      = 6;   //   
    constexpr int16_t  BADGE_Y_OFFSET      = 8;   //  
    constexpr int16_t  BADGE_WIDTH         = 35;
    constexpr int16_t  BADGE_HEIGHT        = 14;
}
