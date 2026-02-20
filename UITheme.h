// ================================================================
// UITheme.h - 통일된 UI 디자인 시스템
// ================================================================
#pragma once

#include <Arduino.h>

// ================================================================
// 컬러 팔레트 - Material Design 기반
// ================================================================
namespace UITheme {
    // ── 기본 색상 ──
    constexpr uint16_t COLOR_PRIMARY       = 0x0E7F;  // 청록색 (주요)
    constexpr uint16_t COLOR_PRIMARY_DARK  = 0x0A5F;  // 어두운 청록
    constexpr uint16_t COLOR_SECONDARY     = 0xFD20;  // 주황색 (강조)
    constexpr uint16_t COLOR_ACCENT        = 0x07FF;  // 밝은 파랑 (액센트)
    
    // ── 상태 색상 ──
    constexpr uint16_t COLOR_SUCCESS       = 0x07E0;  // 녹색 (성공/정상)
    constexpr uint16_t COLOR_WARNING       = 0xFFE0;  // 노란색 (경고)
    constexpr uint16_t COLOR_DANGER        = 0xF800;  // 빨간색 (위험)
    constexpr uint16_t COLOR_INFO          = 0x04BF;  // 파란색 (정보)
    
    // ── 배경/텍스트 ──
    constexpr uint16_t COLOR_BG_DARK       = 0x0000;  // 검정 (배경)
    constexpr uint16_t COLOR_BG_CARD       = 0x2124;  // 어두운 회색 (카드)
    constexpr uint16_t COLOR_BG_ELEVATED   = 0x31A6;  // 밝은 회색 (선택)
    
    constexpr uint16_t COLOR_TEXT_PRIMARY  = 0xFFFF;  // 흰색 (주요 텍스트)
    constexpr uint16_t COLOR_TEXT_SECONDARY= 0xBDF7;  // 회색 (보조 텍스트)
    constexpr uint16_t COLOR_TEXT_DISABLED = 0x7BEF;  // 어두운 회색 (비활성)
    
    // ── 테두리 ──
    constexpr uint16_t COLOR_BORDER        = 0x4208;  // 어두운 회색
    constexpr uint16_t COLOR_DIVIDER       = 0x2945;  // 더 어두운 회색
    
    // ── 관리자 모드 ──
    constexpr uint16_t COLOR_MANAGER       = 0xFBE0;  // 주황색
    constexpr uint16_t COLOR_DEVELOPER     = 0xF81F;  // 마젠타
    
    // ================================================================
    // 타이포그래피
    // ================================================================
    constexpr uint8_t TEXT_SIZE_LARGE      = 3;       // 제목
    constexpr uint8_t TEXT_SIZE_MEDIUM     = 2;       // 본문
    constexpr uint8_t TEXT_SIZE_SMALL      = 1;       // 라벨
    
    // ================================================================
    // 간격 시스템 (8px 기준)
    // ================================================================
    constexpr uint8_t SPACING_XS           = 4;
    constexpr uint8_t SPACING_SM           = 8;
    constexpr uint8_t SPACING_MD           = 16;
    constexpr uint8_t SPACING_LG           = 24;
    constexpr uint8_t SPACING_XL           = 32;
    
    // ================================================================
    // 레이아웃 상수
    // ================================================================
    constexpr uint16_t SCREEN_WIDTH        = 480;
    constexpr uint16_t SCREEN_HEIGHT       = 320;
    
    constexpr uint8_t  HEADER_HEIGHT       = 30;
    constexpr uint8_t  FOOTER_HEIGHT       = 40;
    constexpr uint16_t CONTENT_HEIGHT      = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;
    
    constexpr uint8_t  CARD_RADIUS         = 8;
    constexpr uint8_t  BUTTON_RADIUS       = 6;
    constexpr uint8_t  BADGE_RADIUS        = 12;
    
    // ================================================================
    // 컴포넌트 크기
    // ================================================================
    constexpr uint16_t BUTTON_WIDTH_FULL   = 140;
    constexpr uint16_t BUTTON_WIDTH_HALF   = 68;
    constexpr uint16_t BUTTON_HEIGHT       = 36;
    
    constexpr uint16_t CARD_PADDING        = 12;
    
    // ================================================================
    // 터치 영역 상수 (매직 넘버 제거)
    // ================================================================
    // 헤더 건강도 아이콘
    constexpr int16_t  HEALTH_ICON_X       = SCREEN_WIDTH - 60;
    constexpr int16_t  HEALTH_ICON_Y       = 5;
    constexpr int16_t  HEALTH_ICON_W       = 30;
    constexpr int16_t  HEALTH_ICON_H       = 25;
    
    // 관리자 배지 오프셋
    constexpr int16_t  MANAGER_BADGE_OFFSET = 100;
    
    // 헤더 배지 위치
    constexpr int16_t  BADGE_X_OFFSET      = 6;   // 타이틀 오른쪽 간격
    constexpr int16_t  BADGE_Y_OFFSET      = 8;   // 상단 간격
    constexpr int16_t  BADGE_WIDTH         = 35;
    constexpr int16_t  BADGE_HEIGHT        = 14;
}
