#pragma once

// StateMachine.h      (, / )
void        updateStateMachine();
void        changeState(SystemState newState);
const char* getStateName(SystemState state);
void        initStateMachine();  // Mutex 