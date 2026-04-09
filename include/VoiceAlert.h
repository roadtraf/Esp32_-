#pragma once
#include <Arduino.h>
#include "Lang.h"   // Language enum (LANG_KO, LANG_EN)

// Voice ID
enum VoiceID : uint8_t {
    VOICE_NONE           = 0,
    VOICE_STARTUP        = 1,
    VOICE_PUMP_ON        = 2,
    VOICE_PUMP_OFF       = 3,
    VOICE_PRESSURE_HIGH  = 4,
    VOICE_PRESSURE_LOW   = 5,
    VOICE_TEMP_HIGH      = 6,
    VOICE_CURRENT_HIGH   = 7,
    VOICE_MAINTENANCE    = 8,
    VOICE_EMERGENCY_STOP = 9,
    VOICE_FILTER_CLEAN   = 10,
    VOICE_SYSTEM_OK      = 11,
    VOICE_WIFI_CONNECT   = 12,
    VOICE_WIFI_LOST      = 13,
    // 구버전 호환 alias
    VOICE_READY          = VOICE_SYSTEM_OK,
    VOICE_START          = VOICE_PUMP_ON,
    VOICE_STOP           = VOICE_PUMP_OFF,
    VOICE_WARNING        = VOICE_PRESSURE_HIGH,
    VOICE_MAX
};

// 구버전 호환 타입
typedef uint8_t VoiceSystemId;

enum VoiceState : uint8_t {
    VS_IDLE    = 0,
    VS_PLAYING = 1,
    VS_QUEUED  = 2,
    VS_ERROR   = 3,
};

class VoiceAlert {
public:
    VoiceAlert();
    bool begin();
    void end();

    // 재생
    bool play(VoiceID id);
    bool queue(VoiceID id);
    void stop();
    bool isPlaying() const;

    // 구버전 DFPlayer 호환 API
    bool enqueue(uint8_t /*player*/, uint8_t track) {
        if (track == 0 || track >= (uint8_t)VOICE_MAX) return false;
        return queue((VoiceID)track);
    }
    bool isOnline() const {
        return _initialized && _state != VS_ERROR;
    }
    bool playVoice(uint8_t track) {
        if (track == 0 || track >= (uint8_t)VOICE_MAX) return false;
        return play((VoiceID)track);
    }

    // 구버전 UI 호환 API
    bool playSystem(VoiceSystemId id) {
        return play((VoiceID)id);
    }
    int      getFileCount() const { return (int)VOICE_MAX - 1; }
    Language getLanguage()  const { return _language; }
    void     setLanguage(Language lang) { _language = lang; }

    // 볼륨
    void    setVolume(uint8_t vol);
    uint8_t getVolume() const;

    // 반복
    void setRepeat(bool en, uint32_t intervalMs = 5000);

    // loop에서 호출
    void handle();

    VoiceState getState() const;

private:
    void        _startPlay(VoiceID id);
    const char* _idToPath(VoiceID id) const;
    void        _codecInit();

    uint8_t    _volume      = 15;
    VoiceID    _queue[8]    = {};
    uint8_t    _qHead       = 0;
    uint8_t    _qTail       = 0;
    bool       _repeat      = false;
    uint32_t   _repeatMs    = 5000;
    uint32_t   _lastPlayMs  = 0;
    VoiceID    _lastID      = VOICE_NONE;
    VoiceState _state       = VS_IDLE;
    bool       _initialized = false;
    Language   _language    = LANG_KO;
};

extern VoiceAlert voiceAlert;