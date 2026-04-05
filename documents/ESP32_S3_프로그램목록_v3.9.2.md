# ESP32-S3 진공 제어 시스템 v3.9.2 Phase 3-1
## 전체 프로그램 목록

---

## 📋 프로젝트 개요

### 기본 정보
- **프로젝트명**: ESP32-S3 진공 제어 시스템
- **버전**: v3.9.2 Phase 3-1
- **빌드**: 2024.12 (Refactored)
- **MCU**: ESP32-S3
- **아키텍처**: Manager Pattern

### 주요 특징
- ✅ 예측 유지보수 (Predictive Maintenance)
- ✅ 머신러닝 기반 고장 예측
- ✅ 스마트 알림 시스템
- ✅ 음성 안내 기능
- ✅ 작업자/관리자/개발자 3단계 권한 시스템
- ✅ ThingSpeak 클라우드 연동
- ✅ FreeRTOS 멀티태스킹
- ✅ 향상된 워치독 시스템
- ✅ 안전 모드 (Safe Mode)

### 센서
- 압력 센서
- 온도 센서 (DS18B20)
- 전류 센서

### 통신
- WiFi
- MQTT
- USB Serial
- USB HID Keyboard

### 파일 구성
- **총 파일 수**: 72개
- **헤더 파일**: ~40개
- **소스 파일**: ~32개
- **카테고리**: 13개

---

## 📂 파일 목록

### 1️⃣ Core System Files

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `main.cpp` | 메인 프로그램 | 시스템 초기화, 메인 루프, 시리얼 명령 처리 | v3.9.1 |
| `main_cpp_refactoring.cpp` | 리팩토링 메인 | Manager Pattern 적용, 모듈화된 구조 | v3.9.2 |
| `Config.h` | 시스템 설정 | 핀 정의, 시스템 상수, 설정 구조체 | v3.9 |

---

### 2️⃣ Manager Modules (Phase 3-1)

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `SystemController.h/.cpp` | 시스템 제어 | 작업자/관리자/개발자 모드 관리, 권한 제어 | v3.9.2 |
| `SensorManager.h/.cpp` | 센서 관리 | 센서 데이터 읽기, 버퍼 관리, 캘리브레이션 | v3.9.2 |
| `ControlManager.h/.cpp` | 제어 관리 | 펌프/밸브 제어, PID 제어, 상태 관리 | v3.9.2 |
| `NetworkManager.h/.cpp` | 네트워크 관리 | WiFi/MQTT 연결, 데이터 전송 | v3.9.2 |
| `UIManager.h/.cpp` | UI 관리 | 화면 관리, 터치 처리, 팝업 관리 | v3.9.2 |
| `ConfigManager.h/.cpp` | 설정 관리 | 설정 저장/로드, 백업/복원 | v3.9.2 |

---

### 3️⃣ Core Control Modules

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `Sensor.h/.cpp` | 센서 모듈 | 압력/온도/전류 센서 읽기, 건강 체크 | v3.9 |
| `Control.h/.cpp` | 제어 모듈 | 펌프/밸브 제어, 안전 인터락, 비상 셧다운 | v3.9 |
| `PID_Control.h/.cpp` | PID 제어 | PID 제어 알고리즘, 자동 튜닝 | v3.9 |
| `StateMachine.h/.cpp` | 상태 머신 | 시스템 상태 전환, 타이밍 제어 | v3.9 |
| `ErrorHandler.h/.cpp` | 에러 처리 | 에러 감지, 복구, 링버퍼 로깅 | v3.9 |

---

### 4️⃣ Advanced Features

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `HealthMonitor.h/.cpp` | 건강도 모니터 | 시스템 건강도 계산, 예측 유지보수 | v3.6 |
| `MLPredictor.h/.cpp` | ML 예측기 | 머신러닝 기반 고장 예측 | v3.6 |
| `AdvancedAnalyzer.h/.cpp` | 고급 분석 | AI 기반 데이터 분석 및 이상 탐지 | v3.8.3 |
| `SmartAlert.h/.cpp` | 스마트 알림 | 지능형 유지보수 알림 시스템 | v3.8 |
| `VoiceAlert.h/.cpp` | 음성 알림 | 음성 안내 시스템 | v3.9 |
| `CloudManager.h/.cpp` | 클라우드 관리 | ThingSpeak 연동, 데이터 업로드 | v3.6 |

---

### 5️⃣ Data & Logging

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `DataLogger.h/.cpp` | 데이터 로거 | 센서 데이터 로깅, 건강도 이력 | v3.9.1 |
| `SD_Logger.h/.cpp` | SD 로거 | SD 카드 로깅, 일일 리포트 | v3.9 |
| `SensorBuffer.h/.cpp` | 센서 버퍼 | 순환 버퍼, 통계 계산 | v3.9 |
| `MemoryPool.h/.cpp` | 메모리 풀 | 메모리 관리 최적화 | v3.9 |
| `Memory_Management.cpp` | 메모리 관리 | 동적 메모리 관리 유틸리티 | v3.9 |

---

### 6️⃣ Network & Communication

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `Network.h/.cpp` | 네트워크 | WiFi, MQTT, 시리얼 통신 | v4.0 |
| `WiFiResilience.h/.cpp` | WiFi 복원력 | WiFi 재연결, 안정성 관리 | v3.9 |
| `WiFiPowerManager.h/.cpp` | WiFi 전력 관리 | WiFi 절전 모드 관리 | v3.9 |
| `RemoteManager.h/.cpp` | 원격 관리 | MQTT 원격 제어 | v3.9 |
| `OTA_Update.cpp` | OTA 업데이트 | 무선 펌웨어 업데이트 | v3.9 |

---

### 7️⃣ UI Components

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `UI_Screens.h/.cpp` | UI 화면 | 화면 그리기 및 터치 핸들러 | v3.9 |
| `UIComponents.h/.cpp` | UI 컴포넌트 | 버튼, 슬라이더 등 UI 요소 | v3.9 |
| `UITheme.h` | UI 테마 | 색상, 폰트 정의 | v3.9 |
| `UI_Popup.cpp` | 팝업 | 숫자 입력 팝업 | v3.9 |
| `ManagerUI.h/.cpp` | 관리자 UI | 관리자 전용 UI 컴포넌트 | v3.9 |

---

### 8️⃣ UI Screens (18개 화면)

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `UI_Screen_Main.cpp` | 메인 화면 | 시스템 상태 및 제어 | v3.9 |
| `UI_Screen_Settings.cpp` | 설정 화면 | 시스템 설정 | v3.9 |
| `UI_Screen_PID.cpp` | PID 화면 | PID 파라미터 설정 | v3.9 |
| `UI_Screen_Timing.cpp` | 타이밍 화면 | 사이클 타이밍 설정 | v3.9 |
| `UI_Screen_Statistics.cpp` | 통계 화면 | 통계 정보 표시 | v3.9 |
| `UI_Screen_Calibration.cpp` | 캘리브레이션 | 센서 캘리브레이션 | v3.9 |
| `UI_Screen_Alarm.cpp` | 알람 화면 | 알람 설정 및 이력 | v3.9 |
| `UI_Screen_About.cpp` | 정보 화면 | 시스템 정보 | v3.9 |
| `UI_Screen_Help.cpp` | 도움말 화면 | 사용 설명서 | v3.9 |
| `UI_Screen_TrendGraph.cpp` | 트렌드 그래프 | 데이터 추세 시각화 | v3.9 |
| `UI_Screen_StateDiagram.h/.cpp` | 상태 다이어그램 | 시스템 상태 시각화 | v3.9 |
| `UI_Screen_Health.cpp` | 건강도 화면 | 시스템 건강도 표시 | v3.9 |
| `UI_Screen_HealthTrend.cpp` | 건강도 추세 | 건강도 이력 그래프 | v3.9 |
| `UI_Screen_SmartAlertConfig.cpp` | 알림 설정 | 스마트 알림 구성 | v3.9 |
| `UI_Screen_VoiceSettings.cpp` | 음성 설정 | 음성 알림 설정 | v3.9 |
| `UI_Screen_Watchdog.cpp` | 워치독 화면 | 워치독 상태 표시 | v3.9 |
| `UI_Screen_WatchdogStatus.cpp` | 워치독 상태 | 상세 워치독 정보 | v3.9 |
| `UI_Screen_AdvancedAnalysis.cpp` | 고급 분석 | AI 분석 결과 | v3.9 |
| `Trend_Graph.cpp` | 그래프 모듈 | 트렌드 그래프 렌더링 | v3.9 |

---

### 9️⃣ System Safety & Reliability

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `EnhancedWatchdog.h/.cpp` | 향상된 워치독 | 태스크별 워치독, 자동 복구 | v3.9 |
| `SafeMode.h/.cpp` | 안전 모드 | 시스템 실패 시 최소 기능 모드 | v3.9 |
| `PowerSchedule.h` | 전력 스케줄 | 전력 관리 스케줄링 | v3.9 |

---

### 🔟 Task Management

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `Tasks.h/.cpp` | FreeRTOS 태스크 | 멀티태스킹 구현 | v3.9 |
| `TaskConfig.h` | 태스크 설정 | 태스크 우선순위 및 스택 크기 | v3.9 |

---

### 1️⃣1️⃣ Testing & Debug

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `UnitTest_Framework.h/.cpp` | 테스트 프레임워크 | 단위 테스트 구조 | v3.6 |
| `SystemTest.h/.cpp` | 시스템 테스트 | 통합 테스트, 스트레스 테스트 | v3.9 |
| `Unit_Tests.cpp` | 단위 테스트 | 개별 모듈 테스트 | v3.9 |
| `Test_All_Modules.cpp` | 전체 모듈 테스트 | 모든 모듈 통합 테스트 | v3.9 |
| `Test_DataLogger.cpp` | 로거 테스트 | 데이터 로거 테스트 | v3.9 |
| `AdvancedAnalyzer_Test.cpp` | 분석기 테스트 | 고급 분석기 테스트 | v3.9 |

---

### 1️⃣2️⃣ Utilities & Helpers

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `SerialCommands.h/.cpp` | 시리얼 명령 | 시리얼 명령 처리기 | v3.9 |
| `Utils.h/.cpp` | 유틸리티 | 범용 헬퍼 함수 | v3.9 |
| `Lang.h/.cpp` | 다국어 | 영어/한국어 지원 | v3.9 |
| `Korean_Font.cpp` | 한글 폰트 | 한글 폰트 데이터 | v3.9 |
| `USB_Keyboard.cpp` | USB 키보드 | USB HID 키보드 기능 | v3.9 |

---

### 1️⃣3️⃣ Configuration

| 파일명 | 모듈명 | 주요 기능 | 버전 |
|--------|--------|-----------|------|
| `LovyanGFX_Config.hpp` | 디스플레이 설정 | LovyanGFX 라이브러리 설정 | v3.9 |
| `syntax_test.cpp` | 문법 테스트 | 코드 문법 검증 | v3.9 |

---

## 📊 통계

### 카테고리별 파일 분포

| 카테고리 | 파일 수 | 비율 |
|----------|---------|------|
| Core System Files | 3 | 4.2% |
| Manager Modules (Phase 3-1) | 6 | 8.3% |
| Core Control Modules | 5 | 6.9% |
| Advanced Features | 6 | 8.3% |
| Data & Logging | 5 | 6.9% |
| Network & Communication | 5 | 6.9% |
| UI Components | 5 | 6.9% |
| UI Screens | 18 | 25.0% |
| System Safety & Reliability | 3 | 4.2% |
| Task Management | 2 | 2.8% |
| Testing & Debug | 6 | 8.3% |
| Utilities & Helpers | 5 | 6.9% |
| Configuration | 2 | 2.8% |
| **전체** | **72** | **100%** |

---

## 🎯 주요 모듈 설명

### Manager Pattern (v3.9.2 Phase 3-1)
리팩토링의 핵심으로, 6개의 Manager 클래스가 시스템을 관리합니다:

1. **SystemController**: 권한 및 모드 관리
2. **SensorManager**: 센서 데이터 통합 관리
3. **ControlManager**: 제어 로직 중앙화
4. **NetworkManager**: 네트워크 통신 관리
5. **UIManager**: UI/UX 통합 관리
6. **ConfigManager**: 설정 영속성 관리

### 예측 유지보수 시스템 (v3.6+)
- **HealthMonitor**: 실시간 시스템 건강도 모니터링
- **MLPredictor**: 머신러닝 기반 고장 예측
- **AdvancedAnalyzer**: AI 기반 이상 패턴 탐지

### 스마트 알림 시스템 (v3.8+)
- **SmartAlert**: 상황 인식형 알림
- **VoiceAlert**: 음성 안내 기능

### 안정성 및 복구 시스템
- **EnhancedWatchdog**: 태스크별 감시 및 자동 복구
- **SafeMode**: 시스템 실패 시 안전 모드
- **WiFiResilience**: 네트워크 복원력

---

## 🔄 리팩토링 주요 개선사항

### Before (v3.9.1)
- 전역 변수 남발 (50+ 개)
- 긴 main.cpp (1400+ 줄)
- 모듈 간 강한 결합
- 코드 중복

### After (v3.9.2)
- Manager Pattern 적용
- 전역 변수 최소화
- 모듈화 및 캡슐화
- 명확한 책임 분리
- 유지보수성 향상

---

## 📝 버전 히스토리

| 버전 | 주요 기능 |
|------|-----------|
| v3.6 | 예측 유지보수, ML 예측, 클라우드 연동 |
| v3.7 | 유지보수 알림 고도화 |
| v3.8 | 스마트 알림 시스템, AI 분석 |
| v3.8.3 | 고급 분석 강화 |
| v3.9 | 음성 알림, 워치독 강화 |
| v3.9.1 | 데이터 로거 최적화 |
| **v3.9.2** | **Manager Pattern 리팩토링** |

---

*문서 생성일: 2024년 12월*  
*ESP32-S3 진공 제어 시스템 개발팀*
