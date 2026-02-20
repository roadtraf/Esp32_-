// ================================================================
// CommandHandler.h - 시리얼 명령 처리 모듈 (v3.9.3 String 최적화)
// ================================================================
#pragma once

#include <Arduino.h>

// 상수 정의
constexpr size_t CMD_BUFFER_SIZE = 64;
constexpr size_t PASSWORD_BUFFER_SIZE = 64;

// ================================================================
// 시리얼 명령 처리 클래스 (String 최적화 버전)
// ================================================================
class CommandHandler {
public:
    // 초기화
    void begin();
    
    // 메인 명령 처리
    void handleSerialCommands();
    
private:
    // 모드별 명령 처리 (const char* 사용)
    void handleOperatorCommands(const char* cmd);
    bool handleManagerCommands(const char* cmd);
    bool handleDeveloperCommands(const char* cmd);
    
    // 모드 전환 처리 (const char* 사용)
    void handleModeSwitch(const char* cmd);
    
    // 유틸리티 (버퍼 기반)
    bool readCommand(char* buffer, size_t bufferSize);
    bool waitForPassword(char* buffer, size_t bufferSize, uint32_t timeout = 30000);
    
    // 내부 버퍼
    char cmdBuffer[CMD_BUFFER_SIZE];
    char passwordBuffer[PASSWORD_BUFFER_SIZE];
};

// 전역 인스턴스
extern CommandHandler commandHandler;
