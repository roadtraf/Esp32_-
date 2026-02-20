// ================================================================
// HardenedConfig.h - 가상 하드웨어 테스트 기반 방어 설정
// v3.9.4 Hardened Edition
// ================================================================
// 테스트 시나리오:
//   [1] Brownout false reset 방지
//   [2] Task Watchdog 안정 세팅
//   [3] PSRAM 안전 사용 모드
//   [4] SD 쓰기 타임아웃
//   [5] I2C 버스 복구 (SDA LOW 잠금, 노이즈, 전원 변동)
//   [6] WiFi reconnect non-blocking
//   [7] Heap fragmentation 방어
//   [8] SPI 충돌 방지 (ILI9488 + XPT2046 + SD)
//   [9] DS18B20 완전 비동기 + WDT 안전
// ================================================================
#pragma once

#include <Arduino.h>
#include <esp_system.h>
#include <soc/rtc.h>

// ─────────────────── [1] BROWNOUT 방지 ───────────────────────
// ADC 기반 brownout detector는 노이즈에 취약 → 레벨 완화
// esp_brownout_disable()은 완전히 끄는 것이므로 사용 금지
// 대신: BOD 레벨을 가장 낮은 단계로 설정 (약 2.43V)
#define BROWNOUT_DET_LEVEL      0       // 0=2.43V (최저, 가장 관대)
                                        // 7=3.00V (기본, 가장 엄격)

// Brownout 발생 시 즉시 재시작이 아닌 카운터 기반 지연 재시작
#define BROWNOUT_RETRY_DELAY_MS  500    // 500ms 후 재시도
#define BROWNOUT_MAX_RETRIES     3      // 최대 3회 후 재시작 허용

// ─────────────────── [2] TASK WATCHDOG 안정 세팅 ─────────────
// 기존: WDT_TIMEOUT=10s → WiFi connectWiFi()가 최대 10s 블로킹 → WDT reset
// 해결: WDT timeout 확대 + 각 Task별 독립 WDT 등록

#define WDT_TIMEOUT_HW          15      // 하드웨어 WDT (초) - 여유 확보
#define WDT_TIMEOUT_TASK_VACUUM  3000   // VacuumCtrl 태스크 체크인 간격 (ms)
#define WDT_TIMEOUT_TASK_SENSOR  3000   // SensorRead 태스크 체크인 간격 (ms)
#define WDT_TIMEOUT_TASK_UI      5000   // UIUpdate 태스크 체크인 간격 (ms)
#define WDT_TIMEOUT_TASK_WIFI    30000  // WiFiMgr 태스크 (ms) - 재연결 허용
#define WDT_TIMEOUT_TASK_MQTT    10000  // MQTTHandler 태스크 (ms)
#define WDT_TIMEOUT_TASK_LOGGER  5000   // DataLogger 태스크 (ms)

// WDT feed 최대 간격 - 이 시간 내 반드시 feed
#define WDT_FEED_MAX_INTERVAL_MS 8000

// ─────────────────── [3] PSRAM 안전 사용 모드 ────────────────
// ESP32-S3: 8MB PSRAM (OPI PSRAM)
// 문제: 일반 malloc은 내부 SRAM 우선 → 고갈 시 crash
// 해결: 큰 버퍼는 명시적으로 PSRAM 할당
#define PSRAM_BUFFER_THRESHOLD   1024   // 1KB 이상 → PSRAM 할당
#define PSRAM_SAFE_ALLOC(size)  \
    ((size >= PSRAM_BUFFER_THRESHOLD && psramFound()) ? \
     heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : \
     malloc(size))
#define PSRAM_SAFE_FREE(ptr)    free(ptr)

// 내부 SRAM 최소 여유 공간 (이하면 PSRAM으로 전환)
#define INTERNAL_HEAP_MIN_FREE  32768   // 32KB
// 경고 임계값
#define HEAP_WARN_THRESHOLD     16384   // 16KB 이하 경고

// SensorBuffer: PSRAM 고정 할당 크기
#define SENSOR_BUFFER_SIZE_PSRAM 500    // PSRAM에 500개 샘플 저장
#define SENSOR_BUFFER_SIZE_SRAM  50     // SRAM fallback

// ─────────────────── [4] SD 쓰기 타임아웃 ────────────────────
// SD.open()이 카드 없거나 SPI 충돌 시 무한 대기 가능
// 해결: FreeRTOS 뮤텍스 + 타임아웃 래퍼
#define SD_WRITE_TIMEOUT_MS     2000    // SD 쓰기 타임아웃 (ms)
#define SD_OPEN_TIMEOUT_MS      1000    // SD.open() 타임아웃 (ms)
#define SD_MAX_RETRY_COUNT      3       // SD 쓰기 재시도 횟수
#define SD_RETRY_DELAY_MS       200     // 재시도 간격 (ms)

// SD SPI CS 핀 (Config.h와 연동)
#define SD_CS_PIN               46      // SD 카드 CS 핀

// ─────────────────── [5] I2C 버스 복구 ───────────────────────
// 문제: SDA가 LOW로 잠기면 Wire.begin()이 먹히지 않음
// 해결: GPIO 직접 제어로 클럭 9번 토글 → SDA 강제 해제
#define I2C_SDA_PIN             19      // I2C SDA 핀
#define I2C_SCL_PIN             20      // I2C SCL 핀
#define I2C_FREQ_HZ             100000  // 100kHz (노이즈 환경에서 400kHz 불안)
#define I2C_TIMEOUT_MS          50      // I2C 작업 타임아웃 (ms)
#define I2C_RECOVER_CLOCK_COUNT 9       // SDA 해제를 위한 SCL 클럭 수
#define I2C_RECOVER_DELAY_US    5       // 클럭 사이 지연 (us)
#define I2C_MAX_RETRY           3       // I2C 작업 재시도 횟수
#define I2C_SENSOR_WARMUP_MS    200     // 센서 전원 인가 후 안정화 대기 (ms)

// ─────────────────── [6] WiFi 비블로킹 재연결 ────────────────
// 문제: connectWiFi()가 while(WL_CONNECTED)으로 최대 10s 블로킹
//       → Core0 WiFi Task WDT reset 발생
// 해결: 상태 머신 기반 비동기 재연결

#define WIFI_CONNECT_STEP_MS    500     // 연결 상태 체크 주기 (ms)
#define WIFI_MAX_CONNECT_STEPS  20      // 최대 20 * 500ms = 10s
#define WIFI_BACKOFF_BASE_MS    1000    // 첫 재시도 대기
#define WIFI_BACKOFF_MAX_MS     30000   // 최대 지수 백오프 (30s)
#define WIFI_BACKOFF_MULTIPLIER 2       // 지수 백오프 배수

// ─────────────────── [7] HEAP FRAGMENTATION 방지 ─────────────
// 문제: std::vector::push_back 반복 시 realloc → 단편화
// 해결: capacity 사전 예약 + 순환 고정 크기 버퍼

// SensorBuffer: std::vector 대신 순환 배열 사용
#define CIRCULAR_BUFFER_SIZE    100     // 순환 버퍼 크기 (고정)

// ArduinoJson 재사용 패턴
#define JSON_DOC_SIZE_SENSOR    256     // 센서 데이터 JSON
#define JSON_DOC_SIZE_MQTT      512     // MQTT 발행 JSON
#define JSON_DOC_SIZE_CONFIG    1024    // 설정 JSON

// ─────────────────── [8] SPI 버스 충돌 방지 ──────────────────
// 구성: ILI9488 (TFT) + XPT2046 (터치) + SD 카드 → 동일 SPI 버스 공유
// 문제: UITask에서 TFT 업데이트 중 TouchTask가 XPT2046 접근 시 충돌
//       beginTransaction 누락 시 데이터 오염
// 해결: SPI 버스 전역 Mutex + 각 접근 시 반드시 뮤텍스 획득

// SPI CS 핀 정의
#define TFT_CS_PIN              10      // ILI9488 CS (LovyanGFX에서도 사용)
#define TOUCH_CS_PIN            14      // XPT2046 CS
#define SD_CS_PIN_SPI           46      // SD CS (위 [4]와 동일)

// SPI 뮤텍스 대기 타임아웃
#define SPI_MUTEX_TIMEOUT_MS    100     // 100ms 내 뮤텍스 획득 못하면 skip

// ─────────────────── [9] DS18B20 완전 비동기 ─────────────────
// 문제: requestTemperatures()가 기본 750ms 블로킹 (setWaitForConversion=true시)
//       OneWire 통신 중 인터럽트 비활성화 → UI/제어 지연
// 해결: FreeRTOS Task 분리 + 상태 머신 + 비동기 변환

#define DS18B20_CONVERSION_TIME_MS  800 // 12bit 변환: 750ms + 50ms 여유
#define DS18B20_TASK_STACK         2048 // DS18B20 전용 태스크 스택
#define DS18B20_TASK_PRIORITY       1   // 낮은 우선순위 (제어에 영향 없도록)
#define DS18B20_FALLBACK_TEMP      25.0f // 센서 오류 시 기본값

// ─────────────────── 시스템 상태 모니터링 ────────────────────
#define HEALTH_CHECK_INTERVAL_MS    5000  // 시스템 헬스 체크 주기
#define LOG_HEAP_INTERVAL_MS        10000 // 힙 상태 로그 주기

// ─────────────────── 버전 정보 ───────────────────────────────
#define HW_HARDENED_VERSION "v3.9.4"
#define HW_HARDENED_DATE    "2026-02-17"
