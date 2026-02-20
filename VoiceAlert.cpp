// ================================================================
// VoiceAlert.cpp  —  v3.9 DFPlayer Mini 음성 알림 구현
// ================================================================

#ifdef ENABLE_VOICE_ALERTS

#include "../include/VoiceAlert.h"

// FreeRTOS (delay 개선)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 전역 인스턴스
VoiceAlert voiceAlert;

// ═══════════════════════════════════════════════════════════════
//  생성자
// ═══════════════════════════════════════════════════════════════

VoiceAlert::VoiceAlert() {
    serial = nullptr;
    online = false;
    autoVoice = true;
    muted = false;
    currentVolume = DEFAULT_VOLUME;
    savedVolume = DEFAULT_VOLUME;
    
    currentLanguage = LANG_KO;  // 기본값
    
    repeatEnabled = false;
    repeatCount = 2;
    currentRepeat = 0;
    repeatFolder = 0;
    repeatTrack = 0;
    
    queueHead = 0;
    queueTail = 0;
    
    totalPlayed = 0;
    lastPlayTime = 0;
}

// ═══════════════════════════════════════════════════════════════
//  초기화
// ═══════════════════════════════════════════════════════════════

bool VoiceAlert::begin() {
    Serial.println("[VoiceAlert] 초기화 시작...");
    
    // UART2 초기화
    serial = new HardwareSerial(DFPLAYER_UART);
    serial->begin(DFPLAYER_BAUD, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
    
    vTaskDelay(pdMS_TO_TICKS(500));  // DFPlayer 부팅 대기
    
    // DFPlayer 초기화
    if (!dfPlayer.begin(*serial)) {
        Serial.println("[VoiceAlert] ✗ DFPlayer 연결 실패");
        Serial.println("  - 배선 확인 (TX ↔ RX)");
        Serial.println("  - MicroSD 카드 삽입 확인");
        Serial.println("  - 전원 확인 (3.3V 또는 5V)");
        online = false;
        return false;
    }
    
    online = true;
    Serial.println("[VoiceAlert] ✓ DFPlayer 연결 성공");
    
    // 기본 설정
    dfPlayer.volume(currentVolume);
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
    
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // SD 카드 파일 개수 확인
    int fileCount = dfPlayer.readFileCounts();
    Serial.printf("[VoiceAlert] SD 카드 파일: %d개\n", fileCount);
    
    if (fileCount <= 0) {
        Serial.println("[VoiceAlert] ⚠️  SD 카드에 파일 없음");
        Serial.println("  - SD 카드 포맷 확인 (FAT32)");
        Serial.println("  - 음성 파일 복사 확인");
    }
    
    Serial.printf("[VoiceAlert] 볼륨: %d/30\n", currentVolume);
    Serial.printf("[VoiceAlert] 언어: %s\n", 
                 currentLanguage == LANG_KO ? "한국어" : "English");
    Serial.println("[VoiceAlert] 준비 완료!");
    
    return true;
}

bool VoiceAlert::isOnline() {
    return online;
}

// ═══════════════════════════════════════════════════════════════
//  언어 설정
// ═══════════════════════════════════════════════════════════════

void VoiceAlert::setLanguage(Language lang) {
    currentLanguage = lang;
    
    const char* langName = (lang == LANG_KO) ? "한국어" : "English";
    Serial.printf("[VoiceAlert] 언어 변경: %s\n", langName);
}

Language VoiceAlert::getLanguage() const {
    return currentLanguage;
}

uint8_t VoiceAlert::getFolderNumber(uint8_t baseFolder) {
    uint8_t offset;
    
    switch (currentLanguage) {
        case LANG_KO:
            offset = FOLDER_OFFSET_KOREAN;
            break;
            
        case LANG_EN:
            offset = FOLDER_OFFSET_ENGLISH;
            break;
            
        default:
            offset = FOLDER_OFFSET_KOREAN;  // 폴백
            break;
    }
    
    return baseFolder + offset;
}

// ═══════════════════════════════════════════════════════════════
//  볼륨 제어
// ═══════════════════════════════════════════════════════════════

void VoiceAlert::setVolume(uint8_t volume) {
    if (!online) return;
    
    if (volume > MAX_VOLUME) volume = MAX_VOLUME;
    
    currentVolume = volume;
    
    if (!muted) {
        dfPlayer.volume(volume);
        Serial.printf("[VoiceAlert] 볼륨: %d/30\n", volume);
    } else {
        savedVolume = volume;
    }
}

uint8_t VoiceAlert::getVolume() {
    return currentVolume;
}

void VoiceAlert::volumeUp() {
    if (currentVolume < MAX_VOLUME) {
        setVolume(currentVolume + 1);
    }
}

void VoiceAlert::volumeDown() {
    if (currentVolume > 0) {
        setVolume(currentVolume - 1);
    }
}

void VoiceAlert::mute() {
    if (!online || muted) return;
    
    savedVolume = currentVolume;
    dfPlayer.volume(0);
    muted = true;
    
    Serial.println("[VoiceAlert] 음소거");
}

void VoiceAlert::unmute() {
    if (!online || !muted) return;
    
    dfPlayer.volume(savedVolume);
    currentVolume = savedVolume;
    muted = false;
    
    Serial.println("[VoiceAlert] 음소거 해제");
}

// ═══════════════════════════════════════════════════════════════
//  재생 제어
// ═══════════════════════════════════════════════════════════════

void VoiceAlert::play(uint8_t folder, uint8_t track) {
    if (!online || muted) return;
    
    dfPlayer.playFolder(folder, track);
    
    totalPlayed++;
    lastPlayTime = millis();
    
    Serial.printf("[VoiceAlert] 재생: 폴더 %02d / 트랙 %03d\n", folder, track);
    
    // 반복 설정
    if (repeatEnabled) {
        currentRepeat = 1;
        repeatFolder = folder;
        repeatTrack = track;
    }
}

void VoiceAlert::playSystem(SystemVoice voice) {
    uint8_t folder = getFolderNumber(FOLDER_BASE_SYSTEM);  // 01 또는 11
    play(folder, (uint8_t)voice);
}

void VoiceAlert::playState(StateVoice voice) {
    uint8_t folder = getFolderNumber(FOLDER_BASE_STATE);   // 02 또는 12
    play(folder, (uint8_t)voice);
}

void VoiceAlert::playError(ErrorVoice voice) {
    uint8_t folder = getFolderNumber(FOLDER_BASE_ERROR);   // 03 또는 13
    play(folder, (uint8_t)voice);
    
    // 에러는 자동 반복
    if (!repeatEnabled) {
        repeatEnabled = true;
        repeatCount = 2;
        currentRepeat = 1;
        repeatFolder = folder;
        repeatTrack = (uint8_t)voice;
    }
}

void VoiceAlert::playMaintenance(MaintenanceVoice voice) {
    uint8_t folder = getFolderNumber(FOLDER_BASE_MAINTENANCE);  // 04 또는 14
    play(folder, (uint8_t)voice);
}

void VoiceAlert::playGuide(GuideVoice voice) {
    uint8_t folder = getFolderNumber(FOLDER_BASE_GUIDE);   // 05 또는 15
    play(folder, (uint8_t)voice);
}

void VoiceAlert::pause() {
    if (!online) return;
    dfPlayer.pause();
    Serial.println("[VoiceAlert] 일시정지");
}

void VoiceAlert::resume() {
    if (!online) return;
    dfPlayer.start();
    Serial.println("[VoiceAlert] 재개");
}

void VoiceAlert::stop() {
    if (!online) return;
    dfPlayer.stop();
    currentRepeat = 0;
    Serial.println("[VoiceAlert] 정지");
}

// ═══════════════════════════════════════════════════════════════
//  자동 재생
// ═══════════════════════════════════════════════════════════════

void VoiceAlert::playStateMessage(SystemState state) {
    if (!online || !autoVoice) return;
    
    switch (state) {
        case STATE_IDLE:
            playState(VOICE_STATE_IDLE);
            break;
            
        case STATE_VACUUM_ON:
            playState(VOICE_STATE_VACUUM_ON);
            break;
            
        case STATE_VACUUM_HOLD:
            playState(VOICE_STATE_VACUUM_HOLD);
            break;
            
        case STATE_VACUUM_BREAK:
            playState(VOICE_STATE_VACUUM_BREAK);
            break;
            
        case STATE_COMPLETE:
            playState(VOICE_STATE_COMPLETE);
            break;
            
        /*
        case STATE_STANDBY:  // ← SystemState에 미정의
            playState(VOICE_STATE_STANDBY);
            break;
        */
            
        default:
            break;
    }
}

void VoiceAlert::playErrorMessage(ErrorCode error) {
    if (!online || !autoVoice) return;
    
    switch (error) {
        case ERROR_OVERHEAT:
            playError(VOICE_ERROR_OVERHEAT);
            break;
            
        case ERROR_OVERCURRENT:
            playError(VOICE_ERROR_OVERCURRENT);
            break;
            
        case ERROR_VACUUM_FAIL:
            playError(VOICE_ERROR_VACUUM_FAIL);
            break;            
               
        case ERROR_SENSOR_FAULT:
            playError(VOICE_ERROR_SENSOR);
            break;
            
        case ERROR_EMERGENCY_STOP:
            playError(VOICE_ERROR_EMERGENCY);
            break;
            
        case ERROR_PHOTO_TIMEOUT:
            playError(VOICE_ERROR_TIMEOUT);
            break;
            
        default:
            break;
    }
}

void VoiceAlert::playMaintenanceMessage(MaintenanceLevel level) {
    if (!online || !autoVoice) return;
    
    switch (level) {
        case MAINTENANCE_SOON:
            playMaintenance(VOICE_MAINT_SOON);
            break;
            
        case MAINTENANCE_REQUIRED:
            playMaintenance(VOICE_MAINT_REQUIRED);
            break;
            
        case MAINTENANCE_URGENT:
            playMaintenance(VOICE_MAINT_URGENT);
            break;
            
        default:
            break;
    }
}

// ═══════════════════════════════════════════════════════════════
//  재생 상태
// ═══════════════════════════════════════════════════════════════

bool VoiceAlert::isPlaying() {
    if (!online) return false;
    
    // DFPlayer busy 핀 체크 (핀 연결 필요)
    // 또는 타이머 기반 추정
    return (millis() - lastPlayTime < 5000);  // 5초 이내면 재생 중
}

bool VoiceAlert::isBusy() {
    return isPlaying();
}

// ═══════════════════════════════════════════════════════════════
//  설정
// ═══════════════════════════════════════════════════════════════

void VoiceAlert::enableAutoVoice(bool enable) {
    autoVoice = enable;
    
    if (enable) {
        Serial.println("[VoiceAlert] 자동 음성 ON");
    } else {
        Serial.println("[VoiceAlert] 자동 음성 OFF");
    }
}

bool VoiceAlert::isAutoVoiceEnabled() {
    return autoVoice;
}

void VoiceAlert::enableRepeat(bool enable) {
    repeatEnabled = enable;
    
    if (!enable) {
        currentRepeat = 0;
    }
}

void VoiceAlert::setRepeatCount(uint8_t count) {
    repeatCount = count;
}

// ═══════════════════════════════════════════════════════════════
//  큐 시스템
// ═══════════════════════════════════════════════════════════════

void VoiceAlert::queueVoice(uint8_t folder, uint8_t track) {
    if (enqueue(folder, track)) {
        Serial.printf("[VoiceAlert] 큐 추가: %02d/%03d (큐 크기: %d)\n", 
                     folder, track, getQueueSize());
    } else {
        Serial.println("[VoiceAlert] ⚠️  큐가 가득 찼습니다");
    }
}

void VoiceAlert::clearQueue() {
    queueHead = 0;
    queueTail = 0;
    Serial.println("[VoiceAlert] 큐 초기화");
}

uint8_t VoiceAlert::getQueueSize() {
    if (queueTail >= queueHead) {
        return queueTail - queueHead;
    } else {
        return 10 - queueHead + queueTail;
    }
}

bool VoiceAlert::enqueue(uint8_t folder, uint8_t track) {
    uint8_t next = (queueTail + 1) % 10;
    
    if (next == queueHead) {
        return false;  // 큐 가득 찼음
    }
    
    queue[queueTail].folder = folder;
    queue[queueTail].track = track;
    queueTail = next;
    
    return true;
}

bool VoiceAlert::dequeue(uint8_t& folder, uint8_t& track) {
    if (queueHead == queueTail) {
        return false;  // 큐 비었음
    }
    
    folder = queue[queueHead].folder;
    track = queue[queueHead].track;
    queueHead = (queueHead + 1) % 10;
    
    return true;
}

void VoiceAlert::processQueue() {
    if (!online || isPlaying()) return;
    
    // 반복 재생 중
    if (currentRepeat > 0 && currentRepeat < repeatCount) {
        currentRepeat++;
        play(repeatFolder, repeatTrack);
        return;
    }
    
    // 반복 완료
    if (currentRepeat >= repeatCount) {
        currentRepeat = 0;
        repeatEnabled = false;
    }
    
    // 큐에서 다음 재생
    uint8_t folder, track;
    if (dequeue(folder, track)) {
        play(folder, track);
    }
}

void VoiceAlert::handleRepeat() {
    processQueue();
}

// ═══════════════════════════════════════════════════════════════
//  통계
// ═══════════════════════════════════════════════════════════════

uint32_t VoiceAlert::getTotalPlayed() {
    return totalPlayed;
}

uint32_t VoiceAlert::getLastPlayTime() {
    return lastPlayTime;
}

#endif // ENABLE_VOICE_ALERTS
