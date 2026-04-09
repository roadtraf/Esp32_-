// ================================================================
// SafeMode.h -    (Phase 3-1)
// ================================================================
#pragma once

#include <Arduino.h>
#include <Preferences.h>

// ================================================================
//   
// ================================================================
#define SAFE_MODE_MAX_BOOT_FAILURES    3    //     
#define SAFE_MODE_BOOT_TIMEOUT         30   //     ()
#define SAFE_MODE_PREFERENCE_KEY       "safemode"

// ================================================================
//   
// ================================================================
enum BootFailureReason {
    BOOT_SUCCESS,
    BOOT_WATCHDOG_TIMEOUT,
    BOOT_CONFIG_CORRUPTED,
    BOOT_HARDWARE_FAILURE,
    BOOT_MEMORY_ERROR,
    BOOT_SENSOR_FAILURE,
    BOOT_NETWORK_TIMEOUT,
    BOOT_UNKNOWN_ERROR
};

// ================================================================
//   
// ================================================================
enum SafeModeOption {
    SAFE_RESTORE_BACKUP,        //  
    SAFE_FACTORY_RESET,         //  
    SAFE_DIAGNOSTIC_MODE,       //  
    SAFE_CONTINUE_ANYWAY,       //  
    SAFE_REBOOT                 // 
};

// ================================================================
//  
// ================================================================
struct BootInfo {
    uint32_t bootCount;            //   
    uint32_t successfulBoots;      //   
    uint32_t failedBoots;          //   
    uint32_t consecutiveFailures;  //   
    BootFailureReason lastFailure; //   
    uint32_t lastBootTime;         //   
};

// ================================================================
//    
// ================================================================
class SafeMode {
public:
    //   
    void begin();
    
    //    
    bool checkBootStatus();           //   
    void markBootStart();             //   
    void markBootSuccess();           //   
    void markBootFailure(BootFailureReason reason);  //   
    
    //    
    bool isInSafeMode();              //   
    void enterSafeMode(BootFailureReason reason);    //   
    void exitSafeMode();              //   
    
    //    
    bool restoreFromBackup();         //  
    bool factoryReset();              //  
    bool diagnosticMode();            //   
    
    //    
    BootInfo getBootInfo();
    uint32_t getConsecutiveFailures();
    BootFailureReason getLastFailureReason();
    bool shouldEnterSafeMode();
    
    //   
    void resetBootStats();
    void printBootInfo();
    void printSafeModeStatus();
    
    //  UI  
    void drawSafeModeScreen();
    SafeModeOption handleSafeModeTouch(uint16_t x, uint16_t y);

private:
    BootInfo bootInfo;
    bool inSafeMode;
    bool bootSuccessMarked;
    uint32_t bootStartTime;
    
    // Preferences 
    Preferences prefs;
    
    //    
    void loadBootInfo();
    void saveBootInfo();
    void incrementBootCount();
    void resetFailureCount();
    const char* getFailureReasonString(BootFailureReason reason);
};

// ================================================================
//  
// ================================================================
extern SafeMode safeMode;

// ================================================================
//  
// ================================================================
#define SAFE_MODE_CHECK() safeMode.checkBootStatus()
#define SAFE_MODE_SUCCESS() safeMode.markBootSuccess()
#define SAFE_MODE_FAIL(reason) safeMode.markBootFailure(reason)
