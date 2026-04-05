// ================================================================
// Config.h  —  ESP32-S3 진공 제어 시스템 v3.9.5 (UI 개선 + 임계값 정리)
// ================================================================
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Sensor.h"        // 센서 관련 함수
#include "SensorBuffer.h"  // 센서 버퍼링 시스템
#include "SystemController.h"   // SystemController 
#include "RemoteManager.h"    // Phase 2: 원격 관리
#include "UITheme.h"
#include "UIComponents.h"
#include "EnhancedWatchdog.h"
#include "ConfigManager.h"
#include "SafeMode.h"       
#include "WiFiResilience.h"    

#ifndef ENABLE_MQTT   // MQTT 활성화 확인
#define ENABLE_MQTT
#endif

// ─────────────────── 버전 정보 ──────────────────────────
#define FIRMWARE_VERSION "v3.9.5"
#define BUILD_DATE "2026-02-18"


// ──────────── v3.8.0  단위 테스트 기능 활성화 ───────────────
// #define UNIT_TEST_MODE

// ──────────── v3.8.0 예측 유지보수 기능 활성화 ────────────────
#define ENABLE_PREDICTIVE_MAINTENANCE

// ──────────── v3.8.1: 데이터 로깅, 추세분석 기능 활성화 ──────────────────
#define ENABLE_DATA_LOGGING           // 데이터 로깅
#define ENABLE_TREND_ANALYSIS         // 추세 분석

// ──────────── v3.8.2: 스마트 알림 활성화 ──────────────────
#define ENABLE_SMART_ALERTS

// ──────────── v3.8.3: 고급 분석 활성화 ───────────────────
#define ENABLE_ADVANCED_ANALYSIS

// ──────────── v3.8.3: ThingSpeak 활성화 ─────────────────────
#define ENABLE_THINGSPEAK
#define THINGSPEAK_CHANNEL_ID     123456      //기본 채널
#define THINGSPEAK_WRITE_KEY      "YOUR_WRITE_KEY"

// v3.8 추가 채널 (선택)
#define THINGSPEAK_CHANNEL_TREND    234567
#define THINGSPEAK_WRITE_KEY_TREND  "YOUR_KEY"
#define THINGSPEAK_CHANNEL_ALERT    345678
#define THINGSPEAK_WRITE_KEY_ALERT  "YOUR_KEY"

// v3.9 한 영 음성지원 (선택)
#define DEFAULT_LANGUAGE    LANGUAGE_KOREAN
#define ENABLE_VOICE_ALERTS

// 음성 알림 볼륨 (0-30)
#define VOICE_VOLUME_DEFAULT    20    // 기본 볼륨
#define VOICE_VOLUME_ERROR      25    // 에러 시 볼륨
#define VOICE_VOLUME_EMERGENCY  30    // 비상 시 볼륨

// 시스템 모드 설정
// 비밀번호 (실제 사용 시 변경 필수!)
#define MANAGER_PASSWORD    "admin1234"
#define DEVELOPER_PASSWORD  "dev5678"

// 자동 로그아웃 시간 (밀리초)
#define AUTO_LOGOUT_TIME    300000      // 5분 (관리자 모드)

// ─────────────────── 핀 정의 ──────────────────
// 제어 출력 (IRF540 4CH MOSFET)
#define PIN_PUMP_PWM        1
#define PIN_VALVE           2
#define PIN_12V_MAIN        42
#define PIN_12V_EMERGENCY   43

// DFPlayer Mini 설정
#define DFPLAYER_RX_PIN     17    // ESP32 RX ← DFPlayer TX
#define DFPLAYER_TX_PIN     18    // ESP32 TX → DFPlayer RX
#define DFPLAYER_UART       2     // UART2 사용
#define DFPLAYER_BAUD       9600

// 센서 입력
#define PIN_PRESSURE_SENSOR 4
#define PIN_CURRENT_SENSOR  5
#define PIN_TEMP_SENSOR     14    // DS18B20 온도 센서
#define PIN_LIMIT_SWITCH    40
#define PIN_PHOTO_SENSOR    41
#define PIN_EMERGENCY_STOP  39
#define PIN_ESTOP           0     // GPIO0 (E-Stop 화면용, main_hardened 호환)

// 알람 출력
#define PIN_BUZZER          38
#define PIN_LED_GREEN       37
#define PIN_LED_RED         36

// ─────────────────── 상수 정의 ──────────────────────────────

// ================================================================
// 압력 제어 설정 (v3.9.5 실무 권장값)
// ================================================================
// 펌프 스펙: DC12V 42W, 최대 진공도 -85kPa, 유량 40L/min
// 실무 권장: 스펙의 90% = -76.5kPa (누출 여유 고려)
// ================================================================

// PID 제어 목표값
#define TARGET_PRESSURE        -60.0f   // kPa — 일반 박스(5~10kg) 권장
#define PRESSURE_HYSTERESIS      2.0f   // kPa — ±2kPa 허용 범위

// 압력 경보 단계
#define PRESSURE_ALARM_KPA     -80.0f   // kPa — 경고 (목표값 미달)
#define PRESSURE_TRIP_KPA      -85.0f   // kPa — 위험 (펌프 한계, 강제정지)

// UI 표시 범위
#define PRESSURE_MIN_KPA      -100.0f   // kPa — 프로그레스바/그래프 최소
#define PRESSURE_MAX_KPA         0.0f   // kPa — 프로그레스바/그래프 최대 (대기압)

// PID 파라미터
#define PID_KP                  2.0f
#define PID_KI                  0.5f
#define PID_KD                  1.0f
#define PID_OUTPUT_MIN          0.0f
#define PID_OUTPUT_MAX        100.0f
#define INTEGRAL_LIMIT         50.0f

// ================================================================
// 전류 임계값 (v3.9.5 통합 정리)
// ================================================================
// 펌프 스펙: DC12V 42W → 정격 전류 3.5A
// 실무 권장: 정격의 1.4배(5A) 경고, 1.7배(6A) 위험
// ================================================================

// 전류 경보 단계
#define CURRENT_THRESHOLD_WARNING   5.0f    // A — 경고 (과부하 초기)
#define CURRENT_ALARM_A             5.0f    // A — UI 색상 변경 (동일)

#define CURRENT_THRESHOLD_CRITICAL  6.0f    // A — 위험 (강제정지)
#define CURRENT_TRIP_A              6.0f    // A — 강제정지 (동일)

// UI 표시 범위
#define CURRENT_MIN_A               0.0f    // A — 프로그레스바/그래프 최소
#define CURRENT_MAX_A               8.0f    // A — 프로그레스바/그래프 최대

// ================================================================
// 온도 임계값 (v3.9.5 통합 정리)
// ================================================================
// 펌프 모터 권장 동작 온도: 0~50°C
// 실무 권장: 50°C 경고, 60°C 위험, 70°C 강제 차단
// ================================================================

// 온도 경보 단계
#define TEMP_THRESHOLD_WARNING   50.0f   // °C — 경고 시작
#define TEMP_ALARM_C             50.0f   // °C — UI 색상 변경 (동일)

#define TEMP_THRESHOLD_CRITICAL  60.0f   // °C — 위험
#define TEMP_TRIP_C              60.0f   // °C — 강제 정지 (동일)

#define TEMP_THRESHOLD_SHUTDOWN  70.0f   // °C — 비상 전원 차단

// UI 표시 범위
#define TEMP_MIN_C              -10.0f   // °C — 그래프 최소
#define TEMP_MAX_C               80.0f   // °C — 그래프 최대

// ================================================================
// 타이밍
// ================================================================
#define UPDATE_INTERVAL       100      // ms — 센서 읽기 주기
#define PID_UPDATE_INTERVAL    50      // ms — PID 루프 주기
#define DEBOUNCE_TIME          50      // ms
#define WDT_TIMEOUT            10      // 초
#define IDLE_TIMEOUT  (2UL * 60 * 1000) // 2분

// ================================================================
// PWM
// ================================================================
#define PWM_FREQUENCY   1000
#define PWM_RESOLUTION     8
#define PWM_CHANNEL_PUMP   0
#define PWM_MIN           50
#define PWM_MAX          255

// ================================================================
// 화면
// ================================================================
#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  320

// ─────────────────── 열거형 ─────────────────────────────────
enum SystemState {
  STATE_IDLE,
  STATE_VACUUM_ON,
  STATE_VACUUM_HOLD,
  STATE_VACUUM_BREAK,
  STATE_WAIT_REMOVAL,
  STATE_COMPLETE,
  STATE_ERROR,
  STATE_EMERGENCY_STOP,
  STATE_STANDBY 
};

enum ControlMode {
  MODE_MANUAL,
  MODE_AUTO,
  MODE_PID
};

enum ErrorSeverity {
  SEVERITY_TEMPORARY,
  SEVERITY_RECOVERABLE,
  SEVERITY_CRITICAL
};

enum ErrorCode {
  ERROR_NONE,
  ERROR_OVERCURRENT,
  ERROR_SENSOR_FAULT,
  ERROR_MOTOR_FAILURE,
  ERROR_PHOTO_TIMEOUT,
  ERROR_EMERGENCY_STOP,
  ERROR_WATCHDOG,
  ERROR_MEMORY,
  ERROR_OVERHEAT,           // 과열
  ERROR_TEMP_SENSOR_FAULT,  // 온도센서 고장
  ERROR_VACUUM_FAIL
};

enum ScreenType {
  SCREEN_MAIN,
  SCREEN_SETTINGS,
  SCREEN_TIMING_SETUP,
  SCREEN_PID_SETUP,
  SCREEN_STATISTICS,
  SCREEN_ALARM,
  SCREEN_ABOUT,
  SCREEN_HELP,
  SCREEN_CALIBRATION,
  SCREEN_STATE_DIAGRAM,
  SCREEN_TREND_GRAPH,
  SCREEN_WATCHDOG_STATUS,
  SCREEN_ESTOP,             // [v3.9.5] 비상정지 전용 화면 추가
  
  #ifdef ENABLE_PREDICTIVE_MAINTENANCE
  SCREEN_HEALTH,
  SCREEN_HEALTH_TREND,
  #endif
      
  #ifdef ENABLE_SMART_ALERTS
  SCREEN_SMART_ALERT_CONFIG,
  #endif  
      
  #ifdef ENABLE_VOICE_ALERTS
  SCREEN_VOICE_SETTINGS,
  #endif
    
  #ifdef ENABLE_ADVANCED_ANALYSIS
  SCREEN_ADVANCED_ANALYSIS,
  SCREEN_FAILURE_PREDICTION,
  SCREEN_COMPONENT_LIFE,
  SCREEN_OPTIMIZATION,
  SCREEN_COMPREHENSIVE_REPORT,
  SCREEN_COST_ANALYSIS,
  #endif
};

// ================================================================
// 전역 객체 extern 선언
// ================================================================

// 디스플레이
#include "LovyanGFX_Config.hpp"
extern LGFX tft;

// 시스템 컨트롤러
#include "SystemController.h"
extern SystemController systemController;

// ================================================================
// 화면 관련
// ================================================================
extern ScreenType currentScreen;
extern bool screenNeedsRedraw;

// ================================================================
// 센서 버퍼
// ================================================================
extern std::vector<float> temperatureBuffer;
extern std::vector<float> pressureBuffer;
extern std::vector<float> currentBuffer;

// ================================================================
// 통계
// ================================================================
struct Statistics {
    uint32_t totalCycles;
    uint32_t successfulCycles;
    uint32_t failedCycles;
    uint32_t totalRuntime;
    uint32_t lastResetTime;
};
extern Statistics stats;

// ================================================================
// 에러 처리
// ================================================================
#define ERROR_HIST_MAX 10

enum ErrorSeverity {
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_RECOVERABLE,
    SEVERITY_CRITICAL
};

struct ErrorInfo {
    uint16_t code;
    char message[64];
    uint32_t timestamp;
    uint8_t severity;
};

extern ErrorInfo errorHistory[ERROR_HIST_MAX];
extern uint8_t errorHistIdx;
extern uint8_t errorHistCnt;
extern bool errorActive;
extern ErrorInfo currentError;

// ================================================================
// 조건부 기능
// ================================================================

#ifdef ENABLE_PREDICTIVE_MAINTENANCE
#include "HealthMonitor.h"
extern HealthMonitor healthMonitor;
#endif

#ifdef ENABLE_SMART_ALERTS
#include "SmartAlert.h"
extern SmartAlert smartAlert;
#endif

#ifdef ENABLE_VOICE_ALERTS
#include "VoiceAlert.h"
extern VoiceAlert voiceAlert;
#endif

// ─────────────────── 구조체 ─────────────────────────────────
struct SystemConfig {
  float      targetPressure;
  float      pressureHysteresis;
  float      pidKp, pidKi, pidKd;
  uint32_t   vacuumOnTime;
  uint32_t   vacuumHoldTime;
  uint32_t   vacuumHoldExtension;   // 자동 연장 시간 (ms)
  uint8_t    maxHoldExtensions;     // 최대 연장 횟수
  uint32_t   vacuumBreakTime;
  uint32_t   waitRemovalTime;
  float      tempWarning;           // 경고 온도
  float      tempCritical;          // 위험 온도
  float      tempShutdown;          // 정지 온도
  ControlMode controlMode;
  bool       buzzerEnabled;
  bool       holdExtensionEnabled;  // 자동 연장 활성화
  bool       tempSensorEnabled;     // 온도센서 활성화
  uint8_t  backlightLevel;  
  char      wifiSSID[32];
  char      wifiPassword[64];
  char       mqttBroker[64];    // MQTT 설정 
  uint16_t mqttPort;
  char       mqttUser[32];       // MQTT 사용자명
  char       mqttPassword[64];   // MQTT 비밀번호
  uint8_t    language;       // 0=EN, 1=KO
};

struct SensorData {
  float    pressure;
  float    current;
  float    temperature;     // 온도 (°C)
  bool     limitSwitch;
  bool     photoSensor;
  bool     emergencyStop;
  uint32_t timestamp;
};

struct ErrorInfo {
  ErrorCode      code;
  ErrorSeverity  severity;
  uint32_t       timestamp;
  uint8_t        retryCount;
  char           message[128];
};

struct Statistics {
  uint32_t totalCycles;
  uint32_t successfulCycles;
  uint32_t failedCycles;
  uint32_t totalErrors;
  uint32_t uptime;
  float    averageCycleTime;
  float    minPressure;
  float    maxPressure;
  float    averageCurrent;
};

// ─────────────────── 전역 변수 (extern) ─────────────────────
// 정의(실체)는 main.cpp에 있음 — 여기서는 선언만

// 시스템 상태
extern SystemState  currentState;
extern SystemState  previousState;
extern ControlMode  currentMode;
extern ScreenType   currentScreen;

// 설정
extern SystemConfig  config;
extern Preferences   preferences;

// 에러 정보
extern ErrorInfo     errorInfo;

// 통계
extern Statistics    stats;

// 상태 머신
extern uint32_t      stateStartTime;

// UI 관련
extern bool          screenNeedsRedraw;
extern uint8_t       helpPageIndex;

// 네트워크
extern bool          wifiConnected;
extern bool          mqttConnected;

// SD 카드
extern bool          sdCardAvailable;

// 현재 사용언어 음성지원(v3.9)
extern Language currentLanguage;

extern SafeMode safeMode;
extern WiFiResilience wifiResilience;

// ================================================================
// v3.9.5 변경 이력
// ================================================================
// - 버전: v3.9.4 → v3.9.5
// - 날짜: 2026-02-18
// - 변경사항:
//   1. 압력/온도/전류 임계값 통합 정리 (중복 제거)
//   2. 실무 권장값 적용:
//      - TARGET_PRESSURE = -60kPa (펌프 스펙 90%)
//      - CURRENT_WARNING = 5A (정격 1.4배)
//      - CURRENT_CRITICAL = 6A (정격 1.7배)
//   3. UI 표시용 상수 추가:
//      - PRESSURE_MIN/MAX_KPA
//      - CURRENT_MIN/MAX_A
//      - TEMP_MIN/MAX_C
//   4. SCREEN_ESTOP 추가 (비상정지 전용 화면)
//   5. PIN_ESTOP 추가 (GPIO0, main_hardened 호환)
//   6. 주석 명확화 (스펙 및 실무 근거 명시)
// ================================================================
