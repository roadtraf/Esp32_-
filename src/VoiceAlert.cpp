// ============================================================
// VoiceAlert.cpp  — ES8311 + audioI2S 버전
// Waveshare ESP32-S3-Touch-LCD-3.5B
// 2026-04-08
// ============================================================
// 의존 라이브러리:
//   schreibfaul1/ESP32-audioI2S  @ ^3.3.0   (lib_deps 에 포함)
//   es8311 드라이버              → lib/es8311/ (오프라인 설치)
//
// 음성 파일 위치: SPIFFS  /voice/001.mp3 ~ /voice/013.mp3
//   pio run --target uploadfs  로 업로드
// ============================================================

#include "VoiceAlert.h"
#include "Config.h"        // PIN_I2C_SDA, PIN_I2C_SCL 등
#include <Arduino.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Audio.h>         // schreibfaul1/ESP32-audioI2S

// ── ES8311 I2C 레지스터 직접 제어용 ─────────────────────────
// Waveshare 공식 es8311 드라이버가 lib/es8311/ 에 있으면 아래를 활성화
// #include "es8311.h"
// 없으면 Wire 로 직접 초기화 (아래 _codecInit() 구현)

// ── 핀 (Config.h 또는 platformio.ini -D 플래그에서 옴) ──────
#ifndef I2S_BCK_PIN
#  define I2S_BCK_PIN   13
#endif
#ifndef I2S_LRCK_PIN
#  define I2S_LRCK_PIN  15
#endif
#ifndef I2S_DOUT_PIN
#  define I2S_DOUT_PIN  16
#endif
#ifndef I2S_DIN_PIN
#  define I2S_DIN_PIN   14
#endif
#ifndef PIN_I2C_SDA
#  define PIN_I2C_SDA   7
#endif
#ifndef PIN_I2C_SCL
#  define PIN_I2C_SCL   8
#endif

// ── ES8311 I2C 주소 ──────────────────────────────────────────
#define ES8311_ADDR      0x18

// ── Audio 객체 (ESP32-audioI2S) ──────────────────────────────
static Audio _audio;

// ── 볼륨 변환: DFPlayer 0~21 → audioI2S 0~100 ───────────────
static inline uint8_t _map_volume(uint8_t vol21) {
    return (uint8_t)((uint32_t)vol21 * 100 / 21);
}

// ── 전역 싱글턴 ──────────────────────────────────────────────
VoiceAlert voiceAlert;

// ============================================================
// 생성자
// ============================================================
VoiceAlert::VoiceAlert() {}

// ============================================================
// begin() — ES8311 코덱 + I2S + SPIFFS 초기화
// ============================================================
bool VoiceAlert::begin() {
    if (_initialized) return true;

    // 1. SPIFFS 마운트
    if (!SPIFFS.begin(true)) {
        Serial.println("[VoiceAlert] SPIFFS 마운트 실패");
        _state = VS_ERROR;
        return false;
    }

    // 2. Wire(I2C) 초기화 (온보드 기기와 공유)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // 3. ES8311 코덱 초기화
    _codecInit();

    // 4. audioI2S I2S 핀 설정
    _audio.setPinout(I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DOUT_PIN);
    _audio.setVolume(_map_volume(_volume));  // 0~21 → 0~100

    _initialized = true;
    _state       = VS_IDLE;

    Serial.println("[VoiceAlert] ES8311 초기화 완료");
    return true;
}

// ============================================================
// end()
// ============================================================
void VoiceAlert::end() {
    if (!_initialized) return;
    _audio.stopSong();
    _initialized = false;
    _state       = VS_IDLE;
}

// ============================================================
// play() — 즉시 재생 (진행 중이면 중단)
// ============================================================
bool VoiceAlert::play(VoiceID id) {
    if (!_initialized || id == VOICE_NONE || id >= VOICE_MAX) return false;
    stop();
    _startPlay(id);
    return true;
}

// ============================================================
// queue() — 큐에 추가
// ============================================================
bool VoiceAlert::queue(VoiceID id) {
    if (!_initialized || id == VOICE_NONE || id >= VOICE_MAX) return false;
    uint8_t next = (_qTail + 1) % 8;
    if (next == _qHead) return false;  // 큐 가득 참
    _queue[_qTail] = id;
    _qTail = next;
    if (_state == VS_IDLE) {
        _startPlay(_queue[_qHead]);
        _qHead = (_qHead + 1) % 8;
    }
    return true;
}

// ============================================================
// stop()
// ============================================================
void VoiceAlert::stop() {
    if (!_initialized) return;
    _audio.stopSong();
    _qHead = _qTail = 0;  // 큐 비우기
    _state = VS_IDLE;
}

// ============================================================
// isPlaying()
// ============================================================
bool VoiceAlert::isPlaying() const {
    return _state == VS_PLAYING;
}

// ============================================================
// setVolume()  0~21 (DFPlayer 스케일 호환)
// ============================================================
void VoiceAlert::setVolume(uint8_t vol) {
    _volume = (vol > 21) ? 21 : vol;
    if (_initialized) {
        _audio.setVolume(_map_volume(_volume));
    }
}

uint8_t VoiceAlert::getVolume() const { return _volume; }

// ============================================================
// setRepeat()
// ============================================================
void VoiceAlert::setRepeat(bool en, uint32_t intervalMs) {
    _repeat   = en;
    _repeatMs = intervalMs;
}

// ============================================================
// handle() — loop() 에서 호출
// ============================================================
void VoiceAlert::handle() {
    if (!_initialized) return;

    // audioI2S 루프 처리 (내부 스트림 관리)
    _audio.loop();

    // 재생 완료 감지
    if (_state == VS_PLAYING && !_audio.isRunning()) {
        _state = VS_IDLE;

        // 큐에 다음 항목이 있으면 재생
        if (_qHead != _qTail) {
            _startPlay(_queue[_qHead]);
            _qHead = (_qHead + 1) % 8;
            return;
        }

        // 반복 재생
        if (_repeat && _lastID != VOICE_NONE) {
            uint32_t now = millis();
            if (now - _lastPlayMs >= _repeatMs) {
                _startPlay(_lastID);
            }
        }
    }
}

// ============================================================
// getState()
// ============================================================
VoiceState VoiceAlert::getState() const { return _state; }

// ============================================================
// _startPlay() — 내부 재생 실행
// ============================================================
void VoiceAlert::_startPlay(VoiceID id) {
    const char* path = _idToPath(id);
    if (!SPIFFS.exists(path)) {
        Serial.printf("[VoiceAlert] 파일 없음: %s\n", path);
        _state = VS_ERROR;
        return;
    }
    if (_audio.connecttoFS(SPIFFS, path)) {
        _state      = VS_PLAYING;
        _lastID     = id;
        _lastPlayMs = millis();
        Serial.printf("[VoiceAlert] 재생: %s\n", path);
    } else {
        Serial.printf("[VoiceAlert] 재생 실패: %s\n", path);
        _state = VS_ERROR;
    }
}

// ============================================================
// _idToPath() — VoiceID → SPIFFS 경로
// ============================================================
const char* VoiceAlert::_idToPath(VoiceID id) const {
    // SPIFFS 에 /voice/001.mp3 ~ /voice/013.mp3 형태로 저장
    static char buf[24];
    snprintf(buf, sizeof(buf), "/voice/%03d.mp3", (int)id);
    return buf;
}

// ============================================================
// _codecInit() — ES8311 레지스터 직접 초기화
// (공식 es8311.h 드라이버가 없는 경우 사용)
// ============================================================
void VoiceAlert::_codecInit() {
    // ES8311 기본 초기화 시퀀스 (Waveshare 공식 예제 기반)
    // I2C 레지스터 쓰기 헬퍼
    auto wr = [](uint8_t reg, uint8_t val) {
        Wire.beginTransmission(ES8311_ADDR);
        Wire.write(reg);
        Wire.write(val);
        uint8_t err = Wire.endTransmission();
        if (err) {
            Serial.printf("[ES8311] I2C 오류: reg=0x%02X err=%d\n", reg, err);
        }
    };

    // 소프트 리셋
    wr(0x00, 0x1F);
    delay(20);
    wr(0x00, 0x00);

    // MCLK 내부 생성 (I2S BCLK 기반)
    wr(0x01, 0x30);   // MCLK from BCLK
    wr(0x02, 0x10);   // ADC/DAC 클럭 분주
    wr(0x03, 0x10);

    // I2S 포맷: I2S 표준, 16비트
    wr(0x09, 0x00);   // DAC I2S standard
    wr(0x0A, 0x00);   // ADC I2S standard

    // DAC 볼륨 기본값
    wr(0x32, 0x00);   // DAC 볼륨 0dB
    wr(0x37, 0x08);   // 스피커 출력 믹서 활성화

    // 전원 ON
    wr(0x0D, 0x01);   // DAC 전원 ON
    wr(0x15, 0x00);   // 아날로그 전원 ON

    delay(50);
    Serial.println("[ES8311] 코덱 초기화 완료");
}

// ============================================================
// audioI2S v3.x 콜백 (라이브러리 필수 - 없으면 링크 오류)
// ============================================================
void audio_info(const char* info)           { /* Serial.printf("[audio] %s\n", info); */ }
void audio_id3data(const char* info)        { /* ID3 태그 정보 */ }
void audio_eof_mp3(const char* info)        { /* 재생 완료 — handle() 에서 isPlaying()으로 감지 */ }
void audio_showstation(const char* info)    { }
void audio_showstreamtitle(const char* info){ }
void audio_bitrate(const char* info)        { }
void audio_commercial(const char* info)     { }
void audio_icyurl(const char* info)         { }
void audio_lasthost(const char* info)       { }
void audio_eof_speech(const char* info)     { }