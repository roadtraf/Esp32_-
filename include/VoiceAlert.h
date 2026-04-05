// ================================================================
// VoiceAlert_Hardened.h - DFPlayer 안전 재생 큐 시스템
// v3.9.4 Hardened Edition
// ================================================================
// [I] DFPlayer UART 동시 접근 보호
//
// 기존 문제:
//   - SmartAlert, MQTT callback, 상태머신이 각각 voiceAlert.play() 호출
//   - DFPlayer UART(Serial2)는 재진입 불가
//   - 동시 play()→ 명령 중첩 → DFPlayer 응답 불가 → 이후 모든 음성 무음
//   - dfPlayer.readFileCounts()가 최대 2초 블로킹 (초기화 시)
//
// 해결:
//   - 모든 play 요청을 FreeRTOS Queue에 넣음
//   - 전용 VoiceTask가 큐에서 꺼내 순차 재생
//   - Mutex로 UART 직렬화
//   - 우선순위: EMERGENCY > ERROR > WARNING > INFO
// ================================================================
#pragma once

#ifdef ENABLE_VOICE_ALERTS

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "AdditionalHardening.h"
#include "EnhancedWatchdog.h"

// ================================================================
// 음성 명령 구조체
// ================================================================
struct VoiceCmd {
    uint8_t  folder;       // DFPlayer 폴더 번호
    uint8_t  track;        // 트랙 번호
    uint8_t  volume;       // 볼륨 (0-30), 0=현재 볼륨 사용
    uint8_t  priority;     // 우선순위 (3=긴급, 2=오류, 1=경고, 0=정보)
    bool     interrupt;    // true=현재 재생 중단 후 즉시 재생
};

// 우선순위 상수
#define VOICE_PRI_INFO      0
#define VOICE_PRI_WARNING   1
#define VOICE_PRI_ERROR     2
#define VOICE_PRI_EMERGENCY 3

// ================================================================
// 안전한 DFPlayer 관리자 (큐 기반)
// ================================================================
class SafeVoiceAlert {
public:
    SafeVoiceAlert() : _online(false), _muted(false), _volume(20),
                       _cmdQueue(nullptr), _uart(nullptr),
                       _taskHandle(nullptr) {}

    // ── 초기화 ──
    bool begin(uint8_t rxPin, uint8_t txPin, uint8_t uartNum = 2) {
        Serial.println("[VoiceAlert] 초기화 시작...");

        // UART 초기화
        _uart = new HardwareSerial(uartNum);
        _uart->begin(9600, SERIAL_8N1, rxPin, txPin);

        // 큐 생성
        _cmdQueue = xQueueCreate(VOICE_QUEUE_SIZE, sizeof(VoiceCmd));
        _mutex    = xSemaphoreCreateMutex();

        if (!_cmdQueue || !_mutex) {
            Serial.println("[VoiceAlert] ❌ 큐/뮤텍스 생성 실패");
            return false;
        }

        // DFPlayer 초기화 (블로킹 - 초기화 시에만)
        vTaskDelay(pdMS_TO_TICKS(1000));  // DFPlayer 부팅 대기

        if (!_dfPlayer.begin(*_uart, false, true)) {
            Serial.println("[VoiceAlert] ❌ DFPlayer 연결 실패");
            Serial.println("  - 배선: ESP32 TX → DFPlayer RX, ESP32 RX → DFPlayer TX");
            Serial.println("  - 저항: 1kΩ (TX 라인 직렬)");
            Serial.println("  - SD카드 FAT32 포맷 확인");
            _online = false;
            return false;
        }

        _dfPlayer.volume(_volume);
        _dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
        _online = true;

        // 전용 VoiceTask 생성
        xTaskCreatePinnedToCore(
            _voiceTaskFunc, "VoiceAlert",
            STACK_VOICE_ALERT, this,
            1, &_taskHandle, 0  // Core0 (WiFi와 같은 코어, 낮은 우선순위)
        );
        enhancedWatchdog.registerTask("VoiceAlert", 10000);

        Serial.println("[VoiceAlert] ✅ 초기화 완료 (큐 기반)");
        return true;
    }

    // ── 재생 요청 (큐에 추가) ──
    bool enqueue(uint8_t folder, uint8_t track,
                 uint8_t priority = VOICE_PRI_INFO,
                 bool interrupt = false,
                 uint8_t volume = 0) {
        if (!_online || _muted) return false;

        VoiceCmd cmd = {
            .folder    = folder,
            .track     = track,
            .volume    = volume ? volume : _volume,
            .priority  = priority,
            .interrupt = interrupt
        };

        // 긴급 명령은 큐 앞에 (interrupt=true이면 현재 재생 중단)
        // FreeRTOS Queue는 FIFO만 지원 → 우선순위 큐는 별도 구현 필요
        // 간소화: interrupt=true이면 큐 비우고 즉시 앞에 삽입
        if (interrupt && priority >= VOICE_PRI_ERROR) {
            xQueueReset(_cmdQueue);  // 대기 큐 비우기
        }

        BaseType_t result = xQueueSend(_cmdQueue, &cmd,
                                        pdMS_TO_TICKS(VOICE_MUTEX_TIMEOUT_MS));
        if (result != pdTRUE) {
            Serial.println("[VoiceAlert] ⚠️  큐 가득 참 (재생 스킵)");
            return false;
        }
        return true;
    }

    // ── 긴급 재생 (큐 비우고 즉시) ──
    bool playEmergency(uint8_t folder, uint8_t track) {
        return enqueue(folder, track, VOICE_PRI_EMERGENCY, true, 30);
    }

    // ── 음소거 ──
    void mute(bool enable) {
        _muted = enable;
        if (enable && _online) {
            if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                _dfPlayer.stop();
                xSemaphoreGive(_mutex);
            }
            xQueueReset(_cmdQueue);
        }
    }

    // ── 볼륨 설정 ──
    void setVolume(uint8_t vol) {
        _volume = min(vol, (uint8_t)30);
        if (_online && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            _dfPlayer.volume(_volume);
            xSemaphoreGive(_mutex);
        }
    }

    bool isOnline() const { return _online; }
    bool isMuted()  const { return _muted;  }
    uint32_t getQueueLength() const {
        return _cmdQueue ? uxQueueMessagesWaiting(_cmdQueue) : 0;
    }

private:
    // ── VoiceTask 메인 루프 ──
    static void _voiceTaskFunc(void* param) {
        SafeVoiceAlert* self = (SafeVoiceAlert*)param;
        VoiceCmd cmd;

        for (;;) {
            // 큐에서 명령 대기 (최대 100ms)
            if (xQueueReceive(self->_cmdQueue, &cmd,
                              pdMS_TO_TICKS(100)) == pdTRUE) {
                if (!self->_muted && self->_online) {
                    self->_playDirect(cmd);
                }
            }

            WDT_CHECKIN("VoiceAlert");
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    // ── 실제 재생 (Mutex 보호) ──
    void _playDirect(const VoiceCmd& cmd) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(VOICE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
            Serial.println("[VoiceAlert] 뮤텍스 타임아웃 (재생 스킵)");
            return;
        }

        // 볼륨 설정
        if (cmd.volume != _volume) {
            _dfPlayer.volume(cmd.volume);
        }

        // 재생
        _dfPlayer.playFolder(cmd.folder, cmd.track);

        xSemaphoreGive(_mutex);
    }

    DFRobotDFPlayerMini _dfPlayer;
    bool                _online;
    bool                _muted;
    uint8_t             _volume;
    QueueHandle_t       _cmdQueue;
    SemaphoreHandle_t   _mutex;
    HardwareSerial*     _uart;
    TaskHandle_t        _taskHandle;
};

extern SafeVoiceAlert safeVoiceAlert;

#endif // ENABLE_VOICE_ALERTS
