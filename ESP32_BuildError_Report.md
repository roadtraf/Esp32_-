# ESP32-S3 진공제어시스템 v3.9.5
## 가상 빌드 테스트 — 에러 분석 보고서

> **분석일:** 2026-02-20
> **분석 파일:** `platformio.ini` / `main.cpp` / `Tasks.cpp` / `Config.h` / `SystemController.h`

---

## 1. 빌드 결과 요약

| 🔴 빌드 불가 에러 | 🟡 링크/API 에러 | 🟠 런타임 위험 | ⚠️ 관리 이슈 |
|:-:|:-:|:-:|:-:|
| **11건** | **3건** | **1건** | **1건** |

> **현재 상태: 빌드 실패.** 아래 에러들을 순서대로 수정해야 합니다.

---

## 2. 에러 전체 목록

| # | 에러 유형 | 원인 및 에러 메시지 | 심각도 |
|---|-----------|---------------------|--------|
| 1 | 누락 헤더 — `VoiceAlert_Hardened.h` | `main.cpp` 36행: `#include "VoiceAlert_Hardened.h"` — 저장소에 해당 파일 없음 | 🔴 빌드 불가 |
| 2 | 누락 헤더 — `EnhancedWatchdog_Hardened.h` | `main.cpp` 37행: `#include "EnhancedWatchdog_Hardened.h"` — 저장소엔 `EnhancedWatchdog.h`만 존재 | 🔴 빌드 불가 |
| 3 | 누락 헤더 — `OTA_Hardened.h` | `main.cpp` 38행: `#include "OTA_Hardened.h"` — 저장소에 해당 파일 없음 | 🔴 빌드 불가 |
| 4 | 누락 함수 — `initStateMachine()` | `main.cpp` `setup()`에서 호출 — `StateMachine.h/.cpp`에 선언/정의 없음 | 🔴 빌드 불가 |
| 5 | 누락 라이브러리 — `NTPClient` | `main.cpp`: `#include <NTPClient.h>` — `platformio.ini` `lib_deps`에 미등록 | 🔴 빌드 불가 |
| 6 | 누락 라이브러리 — `DFRobotDFPlayerMini` | `main.cpp`: `#include <DFRobotDFPlayerMini.h>` — `platformio.ini` `lib_deps`에 미등록 | 🔴 빌드 불가 |
| 7 | `ErrorSeverity` 중복 정의 | `Config.h` 내 `enum ErrorSeverity`가 두 번 선언됨 — C++ ODR 위반 | 🔴 빌드 불가 |
| 8 | `Statistics` 중복 정의 | `Config.h` 내 `struct Statistics`가 두 번 정의됨 — ODR 위반 | 🔴 빌드 불가 |
| 9 | `ErrorInfo` 중복 정의 | `Config.h` 내 `struct ErrorInfo`가 두 번 정의됨 — ODR 위반 | 🔴 빌드 불가 |
| 10 | `Tasks.cpp` include 경로 오류 #1 | `#include "../include/Config.h"` — 파일이 루트에 있으므로 경로 불일치 | 🔴 빌드 불가 |
| 11 | `Tasks.cpp` include 경로 오류 #2 | `../include/EnhancedWatchdog.h` 등 5곳 모두 `../include/` 경로 잘못됨 | 🔴 빌드 불가 |
| 12 | ESP32-S3 LEDC API 오류 | `LEDC_HIGH_SPEED_MODE` 사용 — ESP32-S3는 Low Speed 모드만 지원 | 🟡 링크 에러 |
| 13 | `esp_brownout_init()` 미존재 | ESP-IDF 5.x(arduino-esp32 3.x)에서 해당 함수 제거됨 | 🟡 링크 에러 |
| 14 | `partitions_16mb.csv` 누락 | `platformio.ini`에서 참조하지만 저장소에 파일 없음 | 🟡 링크 에러 |
| 15 | ArduinoJson 버전 불일치 | `platformio.ini`에 `^6.21.3` 지정 — 코드에서 v7 스타일 API 혼용 가능성 | 🟠 런타임 위험 |
| 16 | 버전 표기 불일치 | `platformio.ini`=v3.9.4, `Config.h`=v3.9.5, `main.cpp`=v3.9.4 혼재 | ⚠️ 관리 이슈 |

---

## 3. 수정 방법 (우선순위 순)

---

### 3-1. 누락 헤더 파일 3종 → 파일명 수정

`main.cpp`가 존재하지 않는 `_Hardened` 버전 헤더를 참조하고 있습니다. 저장소에 있는 실제 파일명으로 교체하세요.

**❌ 현재 (`main.cpp` 36~38행):**
```cpp
#include "VoiceAlert_Hardened.h"
#include "EnhancedWatchdog_Hardened.h"
#include "OTA_Hardened.h"
```

**✅ 수정 후:**
```cpp
// VoiceAlert_Hardened.h → 저장소에 VoiceAlert.h 있으면 아래처럼, 없으면 삭제
// #include "VoiceAlert.h"

#include "EnhancedWatchdog.h"      // _Hardened 접미사 제거
// OTA_Hardened.h → 삭제 (ArduinoOTA는 main.cpp에서 이미 직접 포함됨)
```

---

### 3-2. `initStateMachine()` 미선언 → `StateMachine.h/.cpp`에 함수 추가

`main.cpp` `setup()`에서 `initStateMachine()`을 호출하지만 `StateMachine.h/.cpp`에 해당 전역 함수가 없습니다.

**`StateMachine.h`에 추가:**
```cpp
// StateMachine.h 하단에 추가
void initStateMachine();
```

**`StateMachine.cpp`에 추가:**
```cpp
void initStateMachine() {
    // 기존 StateMachine 객체 초기화 코드 삽입
    // 예: currentState = STATE_IDLE; stateStartTime = millis();
}
```

---

### 3-3. `platformio.ini` lib_deps 누락 라이브러리 2종 추가

**❌ 현재 (`platformio.ini` `[common]` 섹션):**
```ini
lib_deps =
    lovyan03/LovyanGFX @ ^1.1.9
    knolleary/PubSubClient @ ^2.8
    bblanchon/ArduinoJson @ ^6.21.3
    mathworks/ThingSpeak @ ^2.0.0
    milesburton/DallasTemperature @ ^3.11.0
    paulstoffregen/OneWire @ ^2.3.7
```

**✅ 수정 후 (2줄 추가):**
```ini
lib_deps =
    lovyan03/LovyanGFX @ ^1.1.9
    knolleary/PubSubClient @ ^2.8
    bblanchon/ArduinoJson @ ^6.21.3
    mathworks/ThingSpeak @ ^2.0.0
    milesburton/DallasTemperature @ ^3.11.0
    paulstoffregen/OneWire @ ^2.3.7
    arduino-libraries/NTPClient @ ^3.2.1        ; ← 추가
    dfrobot/DFRobotDFPlayerMini @ ^1.0.6        ; ← 추가
```

---

### 3-4. `Config.h` 구조체/열거형 중복 정의 제거

`Config.h` 안에서 아래 세 가지가 각각 두 번씩 선언되어 있습니다.

**삭제 대상 (상단에 위치한 필드 수가 적은 버전 3개):**

```cpp
// ← 아래 블록 삭제 (Config.h 상단, 필드 3개짜리)
enum ErrorSeverity {
    SEVERITY_TEMPORARY,
    SEVERITY_RECOVERABLE,
    SEVERITY_CRITICAL
};
```

```cpp
// ← 아래 블록 삭제 (Config.h 상단, 필드 5개짜리 Statistics)
struct Statistics {
    uint32_t totalCycles;
    uint32_t successfulCycles;
    uint32_t failedCycles;
    uint32_t totalRuntime;
    uint32_t lastResetTime;
};
```

```cpp
// ← 아래 블록 삭제 (Config.h 상단, 필드 4개짜리 ErrorInfo)
struct ErrorInfo {
    uint16_t code;
    char message[64];
    uint32_t timestamp;
    uint8_t severity;
};
```

**유지 대상 (하단에 위치한 완전한 버전 — 필드 수가 더 많은 것):**
```cpp
// ✅ 유지 — Config.h 하단의 완전한 버전들
enum ErrorSeverity { SEVERITY_INFO, SEVERITY_WARNING, SEVERITY_RECOVERABLE, SEVERITY_CRITICAL };
struct Statistics   { uint32_t totalCycles; ... float averageCycleTime; ... };
struct ErrorInfo    { ErrorCode code; ErrorSeverity severity; ... char message[128]; };
```

---

### 3-5. `Tasks.cpp` include 경로 수정

모든 파일이 루트에 있으므로 `../include/` 경로를 제거해야 합니다.

**❌ 현재 (`Tasks.cpp` 상단):**
```cpp
#include "../include/Config.h"
#include "../include/EnhancedWatchdog.h"
#include "../include/HardenedConfig.h"
#include "../include/SPIBusManager.h"
#include "../include/SafeSensor.h"
```

**✅ 수정 후:**
```cpp
#include "Config.h"
#include "EnhancedWatchdog.h"
#include "HardenedConfig.h"
#include "SPIBusManager.h"
#include "SafeSensor.h"
```

---

### 3-6. ESP32-S3 LEDC API 수정 (`HIGH_SPEED` → `LOW_SPEED`)

ESP32-S3는 LEDC High Speed 모드를 하드웨어적으로 지원하지 않습니다.  
`main.cpp` 전체에서 VS Code `Ctrl+H` 일괄 치환으로 처리하세요.

**❌ 현재 (`main.cpp` 다수):**
```cpp
ledc_timer_config_t timerCfg;
timerCfg.speed_mode = LEDC_HIGH_SPEED_MODE;

ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch, duty);
ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch);
```

**✅ 수정 후 (전부 LOW로 변경):**
```cpp
timerCfg.speed_mode = LEDC_LOW_SPEED_MODE;

ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, duty);
ledc_update_duty(LEDC_LOW_SPEED_MODE, ch);
```

> **일괄 치환:** `LEDC_HIGH_SPEED_MODE` → `LEDC_LOW_SPEED_MODE` (전체 파일 대상)

---

### 3-7. `esp_brownout_init()` → sdkconfig 방식으로 교체

`esp_brownout_init()`은 ESP-IDF 5.x / arduino-esp32 3.x에서 제거되었습니다.

**❌ `main.cpp` `setup()`에서 삭제:**
```cpp
esp_brownout_init();   // ← 이 줄 삭제
```

**✅ `platformio.ini` `build_flags_common`에 추가:**
```ini
build_flags_common =
    ; ... 기존 플래그들 ...
    -DCONFIG_ESP_BROWNOUT_DET=y          ; ← 추가
    -DCONFIG_ESP_BROWNOUT_DET_LVL_SEL_0=y ; ← 추가 (레벨 0 = 2.43V)
```

---

### 3-8. `partitions_16mb.csv` 파일 생성

프로젝트 루트에 아래 내용으로 파일을 새로 생성하세요.

**파일명:** `partitions_16mb.csv` (프로젝트 루트)

```csv
# Name,    Type, SubType,  Offset,   Size,    Flags
nvs,       data, nvs,      0x9000,   0x5000,
otadata,   data, ota,      0xe000,   0x2000,
app0,      app,  ota_0,    0x10000,  0x650000,
app1,      app,  ota_1,    0x660000, 0x650000,
spiffs,    data, spiffs,   0xCB0000, 0x340000,
coredump,  data, coredump, 0xFF0000, 0x10000,
```

---

## 4. 권장 수정 순서

| 순서 | 작업 | 예상 결과 |
|:----:|------|-----------|
| 1 | `Config.h` 중복 정의 3종 제거 | 컴파일 에러 다수 즉시 해소 |
| 2 | `Tasks.cpp` include 경로 5곳 수정 | 파일 탐색 에러 제거 |
| 3 | `main.cpp` 누락 헤더 3종 수정/삭제 | 헤더 탐색 에러 제거 |
| 4 | `platformio.ini` lib_deps 2종 추가 | 라이브러리 빌드 에러 제거 |
| 5 | `initStateMachine()` 선언·정의 추가 | 링커 에러 제거 |
| 6 | `partitions_16mb.csv` 생성 | 파티션 에러 제거 |
| 7 | `LEDC_HIGH_SPEED_MODE` 전체 치환 | ESP32-S3 호환성 확보 |
| 8 | `esp_brownout_init()` 제거 + sdkconfig 전환 | ESP-IDF 5.x 호환성 확보 |

---

## 5. 수정 후 최종 체크리스트

```
[ ] Config.h — ErrorSeverity, Statistics, ErrorInfo 중복 제거 확인
[ ] Tasks.cpp — ../include/ 경로 5곳 수정 확인
[ ] main.cpp — VoiceAlert_Hardened.h, EnhancedWatchdog_Hardened.h, OTA_Hardened.h 수정 확인
[ ] platformio.ini — NTPClient, DFRobotDFPlayerMini lib_deps 추가 확인
[ ] StateMachine.h/.cpp — initStateMachine() 선언·정의 추가 확인
[ ] partitions_16mb.csv — 프로젝트 루트에 생성 확인
[ ] main.cpp — LEDC_HIGH_SPEED_MODE 전체 치환 확인 (VS Code: Ctrl+H)
[ ] main.cpp setup() — esp_brownout_init() 1행 삭제 확인
[ ] pio run 실행 후 에러 0건 확인
```

---

*— 보고서 끝 —*
