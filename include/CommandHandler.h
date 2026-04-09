// ================================================================
// CommandHandler.h -     (v3.9.3 String )
// ================================================================
#pragma once

#include <Arduino.h>

//  
constexpr size_t CMD_BUFFER_SIZE = 64;
constexpr size_t PASSWORD_BUFFER_SIZE = 64;

// ================================================================
//     (String  )
// ================================================================
class CommandHandler {
public:
    // 
    void begin();
    
    //   
    void handleSerialCommands();
    
private:
    //    (const char* )
    void handleOperatorCommands(const char* cmd);
    bool handleManagerCommands(const char* cmd);
    bool handleDeveloperCommands(const char* cmd);
    
    //    (const char* )
    void handleModeSwitch(const char* cmd);
    
    //  ( )
    bool readCommand(char* buffer, size_t bufferSize);
    bool waitForPassword(char* buffer, size_t bufferSize, uint32_t timeout = 30000);
    
    //  
    char cmdBuffer[CMD_BUFFER_SIZE];
    char passwordBuffer[PASSWORD_BUFFER_SIZE];
};

//  
extern CommandHandler commandHandler;
