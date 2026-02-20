#pragma once
// ================================================================
// SD_Logger.h  —  SD 카드 로깅, 일일 리포트, 시간 유틸리티
// v3.9.3: String → char[] 최적화
// ================================================================
#include <Arduino.h>

// ISO8601 타임스탬프 버퍼 크기
constexpr size_t ISO8601_BUFFER_SIZE = 32;

// ── SD 초기화 ──
void initSD();

// ── 사이클·에러·센서 로그 ──
void logCycle();
void logError(const ErrorInfo& error);
void logSensorTrend();

// ── 리포트 ──
void generateDailyReport();
void cleanupOldLogs();

// ── 시간 ──
void syncTime();
void getCurrentTimeISO8601(char* buffer, size_t bufferSize);
