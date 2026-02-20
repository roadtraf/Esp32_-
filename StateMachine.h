#pragma once
// ================================================================
// StateMachine.h  —  상태 머신 (전이, 진입/종료 처리)
// ================================================================
void        updateStateMachine();
void        changeState(SystemState newState);
const char* getStateName(SystemState state);
void        initStateMachine();  // Mutex 초기화


