// ================================================================
// UI_AccessControl.h - PIN       
// [U2] canAccessScreen()      
// [U4] PIN  UI  
// ================================================================
#pragma once

#include <Arduino.h>
#include "../include/Config.h"
#include "../include/UITheme.h"

// ================================================================
// [U2] canAccessScreen()   (ManagerUI.h, UI_Screens.h  )
// ================================================================
bool canAccessScreen(ScreenType screen);

// ================================================================
// [U4] PIN  
// ================================================================

// PIN    
using PinResultCallback = void(*)(bool success, SystemMode targetMode);

// PIN   
// targetMode: MANAGER or DEVELOPER
// onResult  : / 
void showPinInputScreen(SystemMode targetMode, PinResultCallback onResult);

// PIN    (UIManager  )
void drawPinInputScreen();

// PIN    
void handlePinInputTouch(uint16_t x, uint16_t y);

// PIN   
bool isPinScreenActive();

//    (PIN  )
void handleKeyboardOnPinScreen(uint8_t key);

// ================================================================
// [U3]    (vTaskDelay )
// ================================================================

//     ( )
//  UIManager.showMessage() 
void showAccessDeniedAsync(const char* screenName);

//  :  showAccessDenied()    
inline void showAccessDenied(const char* screenName) {
    showAccessDeniedAsync(screenName);
}
