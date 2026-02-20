// ================================================================
// SystemController.h - 시스템 모드 및 접근 제어
// Phase 1: 작업자/관리자/개발자 이원화 시스템
// ================================================================
#pragma once

#include <Arduino.h>
#include <Preferences.h>

// ================================================================
// 시스템 모드 정의
// ================================================================
enum class SystemMode {
    OPERATOR,      // 작업자 모드 (기본) - Level 1
    MANAGER,       // 관리자 모드 - Level 2
    DEVELOPER      // 개발자 모드 - Level 3
};

// ================================================================
// 권한 구조체
// ================================================================
struct SystemPermissions {
    // 기본 제어 권한
    bool canStart;              // 시작 가능
    bool canStop;               // 정지 가능
    bool canPause;              // 일시정지 가능
    bool canReset;              // 리셋 가능
    
    // 고급 제어 권한
    bool canCalibrate;          // 캘리브레이션 실행
    bool canChangeSettings;     // 설정 변경
    bool canAccessAdvanced;     // 고급 기능 접근
    
    // 모니터링 권한
    bool canViewStatistics;     // 통계 보기
    bool canViewHealth;         // 건강도 보기
    bool canViewLogs;           // 로그 보기
    bool canExportData;         // 데이터 내보내기
    
    // 테스트/디버그 권한
    bool canRunTests;           // 테스트 실행
    bool canAccessDebug;        // 디버그 기능
    bool canViewSystemInfo;     // 시스템 정보
    bool canModifySystem;       // 시스템 수정
    
    // UI 권한
    bool canAccessAllScreens;   // 모든 화면 접근
    bool canChangeUISettings;   // UI 설정 변경
};

// ================================================================
// 시스템 컨트롤러 클래스
// ================================================================
class SystemController {
private:
    SystemMode currentMode;
    SystemMode previousMode;
    
    uint32_t modeChangeTime;
    uint32_t lastActivityTime;
    
    // 자동 로그아웃 설정
    bool autoLogoutEnabled;
    uint32_t autoLogoutTimeout;  // 밀리초
    
    // 로그인 시도 제한
    uint8_t loginAttempts;
    uint32_t lockoutEndTime;
    const uint8_t MAX_LOGIN_ATTEMPTS = 3;
    const uint32_t LOCKOUT_DURATION = 60000;  // 1분
    
    // Preferences for persistent storage
    Preferences prefs;
    
    // 내부 메서드
    void saveLastMode();
    void loadLastMode();
    bool verifyPassword(const char* password, SystemMode targetMode);
    void resetLoginAttempts();
    void logModeChange(SystemMode from, SystemMode to);
    
public:
    SystemController();
    
    // 초기화
    void begin();
    
    // 모드 전환
    bool enterOperatorMode();
    bool enterManagerMode(const char* password);
    bool enterDeveloperMode(const char* password);
    
    // 현재 모드
    SystemMode getMode() const { return currentMode; }
    const char* getModeString() const;
    const char* getModeString(SystemMode mode) const;
    
    // 권한 확인
    SystemPermissions getPermissions() const;
    bool hasPermission(const char* action) const;
    
    // 자동 로그아웃
    void setAutoLogout(bool enable, uint32_t timeoutMs = 300000);
    void updateActivity();
    void checkAutoLogout();
    bool isAutoLogoutEnabled() const { return autoLogoutEnabled; }
    uint32_t getRemainingTime() const;
    
    // 로그인 시도 관리
    bool isLockedOut() const;
    uint32_t getLockoutRemainingTime() const;
    void recordFailedLogin();
    
    // 상태 정보
    bool isOperatorMode() const { return currentMode == SystemMode::OPERATOR; }
    bool isManagerMode() const { return currentMode == SystemMode::MANAGER; }
    bool isDeveloperMode() const { return currentMode == SystemMode::DEVELOPER; }
    
    // 디버그
    void printStatus() const;
};

// 전역 인스턴스
extern SystemController systemController;