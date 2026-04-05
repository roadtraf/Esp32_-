// ================================================================
// AdditionalHardening.h - 가상 테스트 추가 발견 취약점 방어 설정
// v3.9.4 Hardened Edition - Additional Issues
// ================================================================
// 추가 발견 항목:
//  [A] 공유 전역변수 (sensorData/currentState/stats) 멀티태스크 동시접근
//  [B] ledcWrite 멀티태스크 동시호출 - PWM 채널 경쟁
//  [C] Preferences(NVS) 동시접근 - Flash write corruption
//  [D] Serial.print 멀티태스크 경쟁 - 출력 뒤섞임
//  [E] 스택 오버플로우 - SmartAlert char[1024] 로컬변수
//  [F] MQTT callback에서 직접 changeState() - ISR-like 재진입
//  [G] OTA 중 펌프/밸브 미정지 - 안전 위험
//  [H] ADC + WiFi 동시 사용 (ESP32 ADC2 제한)
//  [I] DFPlayer UART 큐 없이 직접 play - 동시 호출 충돌
//  [J] volatile 미선언 - 컴파일러 최적화로 최신값 미반영
//  [K] NTP 미동기화 시 SD 파일명 1970년 생성
//  [L] 비상정지 핀 디바운스 없음 - 채터링 오동작
// ================================================================
#pragma once

#include <Arduino.h>

// ─────────────────── [A] 공유 전역변수 보호 ──────────────────
// SensorData, Statistics는 SensorTask가 쓰고 UITask/ControlTask가 읽음
// volatile만으로는 부족 → 멀티바이트 구조체는 atomic 보장 안됨
// 해결: FreeRTOS Mutex로 읽기/쓰기 보호

// ─────────────────── [B] PWM 채널 보호 ───────────────────────
// Control.cpp (VacuumCtrl Task) + ControlManager.cpp + UIManager.cpp
// → 동일 채널 0 동시 write → 예측 불가 PWM 값
#define PWM_MUTEX_TIMEOUT_MS        50

// ─────────────────── [C] NVS(Preferences) 보호 ───────────────
// ESP32 NVS는 내부적으로 Flash를 에뮬레이션
// 동시 write → NVS wear-leveling 오류 → 설정 손실
#define NVS_MUTEX_TIMEOUT_MS        200

// ─────────────────── [D] Serial 출력 보호 ────────────────────
// 여러 태스크에서 Serial.printf 동시 호출 → 출력 뒤섞임
// 릴리즈 빌드: 비활성화로 오버헤드 제거
#ifdef RELEASE_BUILD
  #define SAFE_LOG(fmt, ...)   do {} while(0)
  #define SAFE_LOGLN(msg)      do {} while(0)
#else
  #define SAFE_LOG(fmt, ...)   SafeSerial::printf(fmt, ##__VA_ARGS__)
  #define SAFE_LOGLN(msg)      SafeSerial::println(msg)
#endif

// ─────────────────── [E] 스택 오버플로우 방지 ────────────────
// SmartAlert::sendEmail(): char emailBody[1024] → MQTTHandler Task 스택 4096
// 함수 호출 깊이 고려 시 여유 없음
// 해결: Task별 스택 크기 조정
#define STACK_VACUUM_CTRL   4096
#define STACK_SENSOR_READ   3072
#define STACK_UI_UPDATE     10240   // [증가] LovyanGFX 렌더링 + SPI Guard
#define STACK_WIFI_MGR      4096
#define STACK_MQTT_HANDLER  6144   // [증가] char buffer[512] × 다수
#define STACK_DATA_LOGGER   4096
#define STACK_HEALTH_MON    3072
#define STACK_PREDICTOR     4096
#define STACK_DS18B20       2048
#define STACK_VOICE_ALERT   3072   // [신규] VoiceAlert 전용

// 스택 여유 경고 임계값 (uxTaskGetStackHighWaterMark)
#define STACK_WARN_WORDS    256    // 256 words = 1KB 이하 경고

// ─────────────────── [F] MQTT callback 보호 ──────────────────
// mqttCallback()은 mqttLoop() 내에서 동기 호출
// → Core0 MQTTHandler Task 컨텍스트에서 실행
// → changeState()는 Core1 VacuumCtrl Task가 사용하는 전역변수 수정
// 해결: 직접 변경 금지 → FreeRTOS Queue로 명령 전달
#define MQTT_CMD_QUEUE_SIZE  8     // MQTT 명령 큐 크기
#define MQTT_CMD_TIMEOUT_MS  100   // 큐 전송 타임아웃

// ─────────────────── [G] OTA 안전 정지 ──────────────────────
// OTA 진행 중: 플래시 에라이즈/라이트 → 인터럽트 지연 → 제어 루프 지연
// 해결: OTA onStart에서 펌프/밸브 강제 OFF
#define OTA_SAFE_SHUTDOWN_DELAY_MS  500  // 정지 후 OTA 시작 대기

// ─────────────────── [H] ADC 안전 사용 ──────────────────────
// ESP32 (non-S3): ADC2는 WiFi 활성화 시 사용 불가
// ESP32-S3: ADC2 제한 없음 (다름) - 하지만 ADC calibration 필요
// GPIO4(압력), GPIO5(전류) → ESP32-S3에서 ADC1 (안전)
// 확인: GPIO4 = ADC1_CH3, GPIO5 = ADC1_CH4 → 문제없음
// 단, analogRead()는 재진입 불가 → Mutex 필요
#define ADC_MUTEX_TIMEOUT_MS  20
#define ADC_OVERSAMPLE_COUNT   4  // 노이즈 제거 오버샘플링
#define ADC_REJECT_THRESHOLD   0.15f  // 15% 이상 편차 샘플 제거

// ─────────────────── [I] DFPlayer 큐 보호 ────────────────────
// 여러 태스크/콜백에서 동시 play() 호출 가능
// DFPlayer UART 시리얼은 비재진입
#define VOICE_QUEUE_SIZE     8    // 음성 재생 큐 크기
#define VOICE_MUTEX_TIMEOUT_MS 50

// ─────────────────── [J] volatile 선언 ───────────────────────
// 멀티코어에서 컴파일러 캐싱 방지
// currentState, errorActive, sleepMode 등 핵심 플래그

// ─────────────────── [K] NTP 미동기화 보호 ───────────────────
// time(nullptr) < 100000 → 1970-01-01 00:01:xx
// → SD 파일명: /reports/daily_19700101.txt (덮어쓰기 위험)
#define NTP_VALID_THRESHOLD  1700000000UL  // 2023년 이후만 유효
#define NTP_FALLBACK_PREFIX  "BOOT"        // 미동기화 시 파일명 prefix

// ─────────────────── [L] 비상정지 디바운스 ───────────────────
// 기계적 스위치 채터링: 5~50ms 동안 ON/OFF 반복
// 현재: digitalRead() 직접 사용 → 채터링으로 오동작 가능
#define ESTOP_DEBOUNCE_MS    20   // 비상정지 디바운스 시간
#define ESTOP_CONFIRM_COUNT   3   // 연속 확인 횟수 (20ms × 3 = 60ms)
