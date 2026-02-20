// ================================================================
// SystemController.cpp - 시스템 모드 및 접근 제어 구현
// ================================================================
#include "../include/SystemController.h"
#include "../include/Config.h"

// 전역 인스턴스
SystemController systemController;

// ================================================================
// 비밀번호 해시 (간단한 구현 - 실제로는 더 강력한 해시 사용 권장)
// ================================================================
namespace {
    uint32_t simpleHash(const char* str) {
        uint32_t hash = 5381;
        while (*str) {
            hash = ((hash << 5) + hash) + *str++;
        }
        return hash;
    }
    
    // 비밀번호 해시값 (Config.h에서 정의된 비밀번호의 해시)
    const uint32_t MANAGER_HASH = simpleHash(MANAGER_PASSWORD);
    const uint32_t DEVELOPER_HASH = simpleHash(DEVELOPER_PASSWORD);
}

// ================================================================
// 생성자
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
// 초기화
// ================================================================
void SystemController::begin() {
    Serial.println("[SystemController] 초기화 중...");
    
    // Preferences 초기화
    prefs.begin("sysctrl", false);
    
    // 마지막 모드 로드 (선택 사항)
    // loadLastMode();
    
    // 기본 모드로 시작
    currentMode = SystemMode::OPERATOR;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    Serial.printf("[SystemController] 초기 모드: %s\n", getModeString());
    Serial.println("[SystemController] 초기화 완료");
}

// ================================================================
// 모드 전환 - 작업자 모드
// ================================================================
bool SystemController::enterOperatorMode() {
    if (currentMode == SystemMode::OPERATOR) {
        Serial.println("[SystemController] 이미 작업자 모드입니다");
        return true;
    }
    
    previousMode = currentMode;
    currentMode = SystemMode::OPERATOR;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    logModeChange(previousMode, currentMode);
    
    Serial.println("[SystemController] ✓ 작업자 모드로 전환");
    return true;
}

// ================================================================
// 모드 전환 - 관리자 모드
// ================================================================
bool SystemController::enterManagerMode(const char* password) {
    // 잠금 확인
    if (isLockedOut()) {
        uint32_t remaining = getLockoutRemainingTime();
        Serial.printf("[SystemController] ✗ 계정 잠금: %lu초 남음\n", remaining / 1000);
        return false;
    }
    
    // 비밀번호 확인
    if (!verifyPassword(password, SystemMode::MANAGER)) {
        recordFailedLogin();
        Serial.println("[SystemController] ✗ 비밀번호 오류");
        return false;
    }
    
    // 성공
    resetLoginAttempts();
    previousMode = currentMode;
    currentMode = SystemMode::MANAGER;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    logModeChange(previousMode, currentMode);
    
    Serial.println("[SystemController] ✓ 관리자 모드 진입");
    Serial.printf("[SystemController] 자동 로그아웃: %lu분\n", autoLogoutTimeout / 60000);
    return true;
}

// ================================================================
// 모드 전환 - 개발자 모드
// ================================================================
bool SystemController::enterDeveloperMode(const char* password) {
    // 잠금 확인
    if (isLockedOut()) {
        uint32_t remaining = getLockoutRemainingTime();
        Serial.printf("[SystemController] ✗ 계정 잠금: %lu초 남음\n", remaining / 1000);
        return false;
    }
    
    // 비밀번호 확인
    if (!verifyPassword(password, SystemMode::DEVELOPER)) {
        recordFailedLogin();
        Serial.println("[SystemController] ✗ 비밀번호 오류");
        return false;
    }
    
    // 성공
    resetLoginAttempts();
    previousMode = currentMode;
    currentMode = SystemMode::DEVELOPER;
    modeChangeTime = millis();
    lastActivityTime = millis();
    
    logModeChange(previousMode, currentMode);
    
    Serial.println("[SystemController] ✓ 개발자 모드 진입");
    Serial.println("[SystemController] (자동 로그아웃 비활성화)");
    return true;
}

// ================================================================
// 비밀번호 검증
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
// 권한 확인
// ================================================================
SystemPermissions SystemController::getPermissions() const {
    SystemPermissions perms = {0};
    
    switch (currentMode) {
        case SystemMode::OPERATOR:
            // 작업자: 기본 제어만
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
            // 관리자: 작업자 + 설정/캘리브레이션/모니터링
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
            // 개발자: 모든 권한
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
    
    // 간단한 문자열 비교로 권한 확인
    if (strcmp(action, "start") == 0) return perms.canStart;
    if (strcmp(action, "stop") == 0) return perms.canStop;
    if (strcmp(action, "calibrate") == 0) return perms.canCalibrate;
    if (strcmp(action, "settings") == 0) return perms.canChangeSettings;
    if (strcmp(action, "test") == 0) return perms.canRunTests;
    if (strcmp(action, "debug") == 0) return perms.canAccessDebug;
    
    // 기본적으로 거부
    return false;
}

// ================================================================
// 자동 로그아웃
// ================================================================
void SystemController::setAutoLogout(bool enable, uint32_t timeoutMs) {
    autoLogoutEnabled = enable;
    autoLogoutTimeout = timeoutMs;
    
    Serial.printf("[SystemController] 자동 로그아웃: %s (%lu분)\n",
                  enable ? "활성화" : "비활성화",
                  timeoutMs / 60000);
}

void SystemController::updateActivity() {
    lastActivityTime = millis();
}

void SystemController::checkAutoLogout() {
    // 작업자 모드나 개발자 모드는 자동 로그아웃 안 함
    if (currentMode == SystemMode::OPERATOR || currentMode == SystemMode::DEVELOPER) {
        return;
    }
    
    if (!autoLogoutEnabled) {
        return;
    }
    
    uint32_t elapsed = millis() - lastActivityTime;
    if (elapsed >= autoLogoutTimeout) {
        Serial.println("\n[SystemController] ⏱️ 자동 로그아웃 (타임아웃)");
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
// 로그인 시도 관리
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
    
    Serial.printf("[SystemController] 로그인 실패 (%d/%d)\n", 
                  loginAttempts, MAX_LOGIN_ATTEMPTS);
    
    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        lockoutEndTime = millis() + LOCKOUT_DURATION;
        Serial.printf("[SystemController] ⚠️ 계정 잠금: %lu초\n", LOCKOUT_DURATION / 1000);
    }
}

void SystemController::resetLoginAttempts() {
    loginAttempts = 0;
    lockoutEndTime = 0;
}

// ================================================================
// 모드 문자열
// ================================================================
const char* SystemController::getModeString() const {
    return getModeString(currentMode);
}

const char* SystemController::getModeString(SystemMode mode) const {
    switch (mode) {
        case SystemMode::OPERATOR:  return "작업자";
        case SystemMode::MANAGER:   return "관리자";
        case SystemMode::DEVELOPER: return "개발자";
        default:                    return "알 수 없음";
    }
}

// ================================================================
// 로그 기록
// ================================================================
void SystemController::logModeChange(SystemMode from, SystemMode to) {
    char logBuf[128];
    snprintf(logBuf, sizeof(logBuf),
             "[%lu] Mode: %s → %s",
             millis(),
             getModeString(from),
             getModeString(to));
    
    Serial.println(logBuf);
    
    // TODO: SD 카드에 저장
    // TODO: MQTT로 전송
}

// ================================================================
// 상태 저장/로드
// ================================================================
void SystemController::saveLastMode() {
    prefs.putUChar("lastMode", (uint8_t)currentMode);
}

void SystemController::loadLastMode() {
    uint8_t mode = prefs.getUChar("lastMode", (uint8_t)SystemMode::OPERATOR);
    // 보안상 항상 작업자 모드로 시작
    currentMode = SystemMode::OPERATOR;
}

// ================================================================
// 디버그
// ================================================================
void SystemController::printStatus() const {
    Serial.println("\n========== 시스템 컨트롤러 상태 ==========");
    Serial.printf("현재 모드:       %s\n", getModeString());
    Serial.printf("이전 모드:       %s\n", getModeString(previousMode));
    Serial.printf("모드 변경 시간:  %lu ms 전\n", millis() - modeChangeTime);
    
    if (currentMode != SystemMode::OPERATOR && autoLogoutEnabled) {
        uint32_t remaining = getRemainingTime();
        Serial.printf("남은 시간:       %lu분 %lu초\n", 
                      remaining / 60000, (remaining % 60000) / 1000);
    }
    
    if (isLockedOut()) {
        uint32_t remaining = getLockoutRemainingTime();
        Serial.printf("잠금 상태:       ⚠️ %lu초 남음\n", remaining / 1000);
    } else {
        Serial.printf("로그인 시도:     %d/%d\n", loginAttempts, MAX_LOGIN_ATTEMPTS);
    }
    
    Serial.println("==========================================\n");
}