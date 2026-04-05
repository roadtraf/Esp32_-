// ================================================================
// SerialCommands.h - 시리얼 명령어 처리 (v3.9.3 String 최적화)
// ================================================================
#pragma once

#include <Arduino.h>

// 명령어 버퍼 크기
constexpr size_t SERIAL_CMD_BUFFER_SIZE = 64;

// ================================================================
// 시리얼 명령어 처리 함수 (버퍼 기반)
// ================================================================
void processSerialCommands();

// ================================================================
// 개별 명령어 그룹 핸들러 (const char* 사용)
// ================================================================
void handleSystemCommands(const char* cmd);
void handleWatchdogCommands(const char* cmd);
void handleConfigCommands(const char* cmd);
void handleNetworkCommands(const char* cmd);
void handleSensorCommands(const char* cmd);
void handleControlCommands(const char* cmd);
void handleDebugCommands(const char* cmd);
void showHelp();
