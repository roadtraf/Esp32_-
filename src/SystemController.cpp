// ================================================================
// SystemController.cpp -      
// ================================================================
#include "SystemController.h"
#include "Config.h"

//  
SystemController systemController;

// ================================================================
//   (  -      )
// ================================================================
namespace {
    uint32_t simpleHash(const char* str) {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + *str++;
        }
        return hash;
    }
    
    //   (Config.h   )
    const uint32_t MANAGER_HASH = simpleHash(MANAGER_PASSWORD);
    const uint32_t DEVELOPER_HASH = simpleHash(DEVELOPER_PASSWORD);
}

// ================================================================
// 
// ================================================================
SystemController::SystemController()
    : currentMode(SystemMode::OPERATOR),
      previousMode(SystemMode::OPERATOR),
      modeChangeTime(0),
      lastActivityTime(0),
      autoLogoutEnabled(true),
      autoLogoutTimeout(AUTO_LOGOUT_TIME),
      loginAttempts(0),
      lockoutEndTime(0)
{
}

// ================================================================
// 
// ================================================================
void SystemController::begin() {
    Serial.println("[SystemController]  ...");
    
    // Preferences 
    prefs.begin("sysctrl", false);
    
    //    ( )
    // loadLastMode();
    
    //   
    currentMode = SystemMode::OPERATOR;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    Serial.printf("[SystemController]  : %s\n", getModeString());
    Serial.println("[SystemController]  ");
}

// ================================================================
//   -  
// ================================================================
bool SystemController::enterOperatorMode() {
    if (currentMode == SystemMode::OPERATOR) {
        Serial.println("[SystemController]   ");
        return true;
    }
    
    previousMode = currentMode;
    currentMode = SystemMode::OPERATOR;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    logModeChange(previousMode, currentMode);
    
    Serial.println("[SystemController]    ");
    return true;
}

// ================================================================
//   -  
// ================================================================
bool SystemController::enterManagerMode(const char* password) {
    //  
    if (isLockedOut()) {
        uint32_t remaining = getLockoutRemainingTime();
        Serial.printf("[SystemController]   : %lu \n", remaining / 1000);
        return false;
    }
    
    //  
    if (!verifyPassword(password, SystemMode::MANAGER)) {
        recordFailedLogin();
        Serial.println("[SystemController]   ");
        return false;
    }
    
    // 
    resetLoginAttempts();
    previousMode = currentMode;
    currentMode = SystemMode::MANAGER;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    logModeChange(previousMode, currentMode);
    
    Serial.println("[SystemController]    ");
    Serial.printf("[SystemController]  : %lu\n", autoLogoutTimeout / 60000);
    return true;
}

// ================================================================
//   -  
// ================================================================
bool SystemController::enterDeveloperMode(const char* password) {
    //  
    if (isLockedOut()) {
        uint32_t remaining = getLockoutRemainingTime();
        Serial.printf("[SystemController]   : %lu \n", remaining / 1000);
        return false;
    }
    
    //  
    if (!verifyPassword(password, SystemMode::DEVELOPER)) {
        recordFailedLogin();
        Serial.println("[SystemController]   ");
        return false;
    }
    
    // 
    resetLoginAttempts();
    previousMode = currentMode;
    currentMode = SystemMode::DEVELOPER;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    logModeChange(previousMode, currentMode);
    
    Serial.println("[SystemController]    ");
    Serial.println("[SystemController] (  )");
    return true;
}

// ================================================================
//  
// ================================================================
bool SystemController::verifyPassword(const char* password, SystemMode targetMode) {
    if (!password || strlen(password) == 0) {
        return false;
    }
    
    uint32_t inputHash = simpleHash(password);
    
    switch (targetMode) {
        case SystemMode::MANAGER:
            return inputHash == MANAGER_HASH;
        case SystemMode::DEVELOPER:
            return inputHash == DEVELOPER_HASH;
        default:
            return false;
    }
}

// ================================================================
//  
// ================================================================
SystemPermissions SystemController::getPermissions() const {
    SystemPermissions perms = {0};
    
    switch (currentMode) {
        case SystemMode::OPERATOR:
            // :  
            perms.canStart = true;
            perms.canStop = true;
            perms.canPause = true;
            perms.canReset = false;
            
            perms.canCalibrate = false;
            perms.canChangeSettings = false;
            perms.canAccessAdvanced = false;
            
            perms.canViewStatistics = true;
            perms.canViewHealth = false;
            perms.canViewLogs = false;
            perms.canExportData = false;
            
            perms.canRunTests = false;
            perms.canAccessDebug = false;
            perms.canViewSystemInfo = false;
            perms.canModifySystem = false;
            
            perms.canAccessAllScreens = false;
            perms.canChangeUISettings = false;
            break;
            
        case SystemMode::MANAGER:
            // :  + //
            perms.canStart = true;
            perms.canStop = true;
            perms.canPause = true;
            perms.canReset = true;
            
            perms.canCalibrate = true;
            perms.canChangeSettings = true;
            perms.canAccessAdvanced = true;
            
            perms.canViewStatistics = true;
            perms.canViewHealth = true;
            perms.canViewLogs = true;
            perms.canExportData = true;
            
            perms.canRunTests = false;
            perms.canAccessDebug = false;
            perms.canViewSystemInfo = true;
            perms.canModifySystem = false;
            
            perms.canAccessAllScreens = true;
            perms.canChangeUISettings = true;
            break;
            
        case SystemMode::DEVELOPER:
            // :  
            perms.canStart = true;
            perms.canStop = true;
            perms.canPause = true;
            perms.canReset = true;
            
            perms.canCalibrate = true;
            perms.canChangeSettings = true;
            perms.canAccessAdvanced = true;
            
            perms.canViewStatistics = true;
            perms.canViewHealth = true;
            perms.canViewLogs = true;
            perms.canExportData = true;
            
            perms.canRunTests = true;
            perms.canAccessDebug = true;
            perms.canViewSystemInfo = true;
            perms.canModifySystem = true;
            
            perms.canAccessAllScreens = true;
            perms.canChangeUISettings = true;
            break;
    }
    
    return perms;
}

bool SystemController::hasPermission(const char* action) const {
    SystemPermissions perms = getPermissions();
    
    //     
    if (strcmp(action, "start") == 0) return perms.canStart;
    if (strcmp(action, "stop") == 0) return perms.canStop;
    if (strcmp(action, "calibrate") == 0) return perms.canCalibrate;
    if (strcmp(action, "settings") == 0) return perms.canChangeSettings;
    if (strcmp(action, "test") == 0) return perms.canRunTests;
    if (strcmp(action, "debug") == 0) return perms.canAccessDebug;
    
    //  
    return false;
}

// ================================================================
//  
// ================================================================
void SystemController::setAutoLogout(bool enable, uint32_t timeoutMs) {
    autoLogoutEnabled = enable;
    autoLogoutTimeout = timeoutMs;
    
    Serial.printf("[SystemController]  : %s (%lu)\n",
                  enable ? "" : "",
                  timeoutMs / 60000);
}

void SystemController::updateActivity() {
    lastActivityTime = millis();
}

void SystemController::checkAutoLogout() {
    //        
    if (currentMode == SystemMode::OPERATOR || currentMode == SystemMode::DEVELOPER) {
        return;
    }
    
    if (!autoLogoutEnabled) {
        return;
    }
    
    uint32_t elapsed = millis() - lastActivityTime;
    if (elapsed >= autoLogoutTimeout) {
        Serial.println("\n[SystemController]    ()");
        enterOperatorMode();
    }
}

uint32_t SystemController::getRemainingTime() const {
    if (!autoLogoutEnabled || currentMode == SystemMode::OPERATOR) {
        return 0;
    }
    
    uint32_t elapsed = millis() - lastActivityTime;
    if (elapsed >= autoLogoutTimeout) {
        return 0;
    }
    
    return autoLogoutTimeout - elapsed;
}

// ================================================================
//   
// ================================================================
bool SystemController::isLockedOut() const {
    if (loginAttempts < MAX_LOGIN_ATTEMPTS) {
        return false;
    }
    
    return millis() < lockoutEndTime;
}

uint32_t SystemController::getLockoutRemainingTime() const {
    if (!isLockedOut()) {
        return 0;
    }
    
    uint32_t now = millis();
    if (now >= lockoutEndTime) {
        return 0;
    }
    
    return lockoutEndTime - now;
}

void SystemController::recordFailedLogin() {
    loginAttempts++;
    
    Serial.printf("[SystemController]   (%d/%d)\n", 
                  loginAttempts, MAX_LOGIN_ATTEMPTS);
    
    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        lockoutEndTime = millis() + LOCKOUT_DURATION;
        Serial.printf("[SystemController]   : %lu\n", LOCKOUT_DURATION / 1000);
    }
}

void SystemController::resetLoginAttempts() {
    loginAttempts = 0;
    lockoutEndTime = 0;
}

// ================================================================
//  
// ================================================================
const char* SystemController::getModeString() const {
    return getModeString(currentMode);
}

const char* SystemController::getModeString(SystemMode mode) const {
    switch (mode) {
        case SystemMode::OPERATOR:  return "";
        case SystemMode::MANAGER:   return "";
        case SystemMode::DEVELOPER: return "";
        default:                    return "  ";
    }
}

// ================================================================
//  
// ================================================================
void SystemController::logModeChange(SystemMode from, SystemMode to) {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf),
             "[%lu] Mode: %s  %s",
             millis(),
             getModeString(from),
             getModeString(to));
    
    Serial.println(logBuf);
    
    // TODO: SD  
    // TODO: MQTT 
}

// ================================================================
//  /
// ================================================================
void SystemController::saveLastMode() {
    prefs.putUChar("lastMode", (uint8_t)currentMode);
}

void SystemController::loadLastMode() {
    uint8_t mode = prefs.getUChar("lastMode", (uint8_t)SystemMode::OPERATOR);
    //     
    currentMode = SystemMode::OPERATOR;
}

// ================================================================
// 
// ================================================================
void SystemController::printStatus() const {
    Serial.println("\n==========    ==========");
    Serial.printf(" :       %s\n", getModeString());
    Serial.printf(" :       %s\n", getModeString(previousMode));
    Serial.printf("  :  %lu ms \n", millis() - modeChangeTime);
    
    if (currentMode != SystemMode::OPERATOR && autoLogoutEnabled) {
        uint32_t remaining = getRemainingTime();
        Serial.printf(" :       %lu %lu\n", 
                      remaining / 60000, (remaining % 60000) / 1000);
    }
    
    if (isLockedOut()) {
        uint32_t remaining = getLockoutRemainingTime();
        Serial.printf(" :        %lu \n", remaining / 1000);
    } else {
        Serial.printf(" :     %d/%d\n", loginAttempts, MAX_LOGIN_ATTEMPTS);
    }
    
    Serial.println("==========================================\n");
}