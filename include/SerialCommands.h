// ================================================================
// SerialCommands.h -    (v3.9.3 String )
// ================================================================
#pragma once

#include <Arduino.h>

//   
constexpr size_t SERIAL_CMD_BUFFER_SIZE = 64;

// ================================================================
//     ( )
// ================================================================
void processSerialCommands();

// ================================================================
//     (const char* )
// ================================================================
void handleSystemCommands(const char* cmd);
void handleWatchdogCommands(const char* cmd);
void handleConfigCommands(const char* cmd);
void handleNetworkCommands(const char* cmd);
void handleSensorCommands(const char* cmd);
void handleControlCommands(const char* cmd);
void handleDebugCommands(const char* cmd);
void showHelp();
