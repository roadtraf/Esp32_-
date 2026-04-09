#pragma once
// ================================================================
// ErrorHandler.h     , , ring-buffer 
// ================================================================

void setError(ErrorCode code, ErrorSeverity severity, const char* message);
void clearError();
void handleError();
bool attemptErrorRecovery();
