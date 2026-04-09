// ================================================================
// ManagerUI.h -   UI 
// Phase 2:   
// ================================================================
#pragma once

#include <Arduino.h>
#include "GFX_Wrapper.hpp"
#include "SystemController.h"
#include "UI_AccessControl.h"

// ================================================================
//  UI 
// ================================================================

//     
void drawManagerBadge();

//    
void drawLogoutTimer();

//   
void drawManagerMenu();

//   
void drawManagerSettingsScreen();

//   
bool showPermissionDialog(const char* action);

//    (TFT )
bool showPasswordDialog(SystemMode targetMode);

// ================================================================
//     
// ================================================================

//   
void showAccessDenied(const char* screenName);

// ================================================================
//   
// ================================================================

//   
void drawAdvancedStatistics();

//   
void drawSensorHistory();

//   
void drawSystemDiagnostics();