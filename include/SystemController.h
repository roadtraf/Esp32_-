// ================================================================
// SystemController.h -     
// Phase 1: //  
// ================================================================
#pragma once

#include <Arduino.h>
#include <Preferences.h>

// ================================================================
//   
// ================================================================
enum class SystemMode {
    OPERATOR,      //   () - Level 1
    MANAGER,       //   - Level 2
    DEVELOPER      //   - Level 3
};

// ================================================================
//  
// ================================================================
struct SystemPermissions {
    //   
    bool canStart;              //  
    bool canStop;               //  
    bool canPause;              //  
    bool canReset;              //  
    
    //   
    bool canCalibrate;          //  
    bool canChangeSettings;     //  
    bool canAccessAdvanced;     //   
    
    //  
    bool canViewStatistics;     //  
    bool canViewHealth;         //  
    bool canViewLogs;           //  
    bool canExportData;         //  
    
    // / 
    bool canRunTests;           //  
    bool canAccessDebug;        //  
    bool canViewSystemInfo;     //  
    bool canModifySystem;       //  
    
    // UI 
    bool canAccessAllScreens;   //   
    bool canChangeUISettings;   // UI  
};

// ================================================================
//   
// ================================================================
class SystemController {
private:
    SystemMode currentMode;
    SystemMode previousMode;
    
    uint32_t modeChangeTime;
    uint32_t lastActivityTime;
    
    //   
    bool autoLogoutEnabled;
    uint32_t autoLogoutTimeout;  // 
    
    //   
    uint8_t loginAttempts;
    uint32_t lockoutEndTime;
    const uint8_t MAX_LOGIN_ATTEMPTS = 3;
    const uint32_t LOCKOUT_DURATION = 60000;  // 1
    
    // Preferences for persistent storage
    Preferences prefs;
    
    //  
    void saveLastMode();
    void loadLastMode();
    bool verifyPassword(const char* password, SystemMode targetMode);
    void resetLoginAttempts();
    void logModeChange(SystemMode from, SystemMode to);
    
public:
    SystemController();
    
    // 
    void begin();
    
    //  
    bool enterOperatorMode();
    bool enterManagerMode(const char* password);
    bool enterDeveloperMode(const char* password);
    
    //  
    SystemMode getMode() const { return currentMode; }
    const char* getModeString() const;
    const char* getModeString(SystemMode mode) const;
    
    //  
    SystemPermissions getPermissions() const;
    bool hasPermission(const char* action) const;
    
    //  
    void setAutoLogout(bool enable, uint32_t timeoutMs = 300000);
    void updateActivity();
    void checkAutoLogout();
    bool isAutoLogoutEnabled() const { return autoLogoutEnabled; }
    uint32_t getRemainingTime() const;
    
    //   
    bool isLockedOut() const;
    uint32_t getLockoutRemainingTime() const;
    void recordFailedLogin();
    
    //  
    bool isOperatorMode() const { return currentMode == SystemMode::OPERATOR; }
    bool isManagerMode() const { return currentMode == SystemMode::MANAGER; }
    bool isDeveloperMode() const { return currentMode == SystemMode::DEVELOPER; }
    
    // 
    void printStatus() const;
};

//  
extern SystemController systemController;