#pragma once
// ================================================================
// SD_Logger.h    SD  ,  ,  
// v3.9.3: String  char[] 
// ================================================================
#include <Arduino.h>

// ISO8601   
constexpr size_t ISO8601_BUFFER_SIZE = 32;

//  SD  
void initSD();

//    
void logCycle();
void logError(const ErrorInfo& error);
void logSensorTrend();

//   
void generateDailyReport();
void cleanupOldLogs();

//   
void syncTime();
void getCurrentTimeISO8601(char* buffer, size_t bufferSize);
