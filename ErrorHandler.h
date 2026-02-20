#pragma once
// ================================================================
// ErrorHandler.h  —  에러 설정, 복구, ring-buffer 로그
// ================================================================

void setError(ErrorCode code, ErrorSeverity severity, const char* message);
void clearError();
void handleError();
bool attemptErrorRecovery();
