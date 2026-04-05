
#ifndef LOVYANGFX_CONFIG_HPP
#define LOVYANGFX_CONFIG_HPP

#include <LovyanGFX.hpp>

// ESP32-S3용 LovyanGFX 설정
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX(void) {
    // ===== SPI 버스 설정 =====
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;     // ESP32-S3는 SPI2_HOST 사용
      cfg.spi_mode = 0;             // SPI 모드 0
      cfg.freq_write = 40000000;    // 전송 시 SPI 클럭 (40MHz)
      cfg.freq_read = 16000000;     // 수신 시 SPI 클럭 (16MHz)
      cfg.spi_3wire = false;        // 3-wire 모드 비활성화
      cfg.use_lock = true;          // 트랜잭션 락 사용
      cfg.dma_channel = SPI_DMA_CH_AUTO; // DMA 자동 할당
      cfg.pin_sclk = 12;            // SPI SCLK 핀
      cfg.pin_mosi = 11;            // SPI MOSI 핀
      cfg.pin_miso = 13;            // SPI MISO 핀
      cfg.pin_dc = 9;               // SPI DC 핀

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // ===== 디스플레이 패널 설정 =====
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 10;              // CS 핀
      cfg.pin_rst = 8;              // RST 핀
      cfg.pin_busy = -1;            // BUSY 핀 (미사용)

      cfg.memory_width = 320;       // 드라이버 IC가 지원하는 최대 폭
      cfg.memory_height = 480;      // 드라이버 IC가 지원하는 최대 높이
      cfg.panel_width = 320;        // 실제 표시 폭
      cfg.panel_height = 480;       // 실제 표시 높이
      cfg.offset_x = 0;             // 패널 오프셋 X
      cfg.offset_y = 0;             // 패널 오프셋 Y
      cfg.offset_rotation = 0;      // 회전 방향 오프셋 (0~7)
      cfg.dummy_read_pixel = 8;     // 픽셀 읽기 전 더미 리드 비트 수
      cfg.dummy_read_bits = 1;      // 픽셀 외 데이터 읽기 전 더미 리드 비트 수
      cfg.readable = true;          // 데이터 읽기 가능
      cfg.invert = false;           // 패널 밝기 반전 여부
      cfg.rgb_order = false;        // 패널 RGB 순서
      cfg.dlen_16bit = false;       // 16비트 병렬 전송 모드
      cfg.bus_shared = true;        // SD 카드와 버스 공유

      _panel_instance.config(cfg);
    }

    // ===== 백라이트 설정 =====
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 15;              // 백라이트 핀
      cfg.invert = false;           // 백라이트 신호 반전 여부
      cfg.freq = 44100;             // 백라이트 PWM 주파수
      cfg.pwm_channel = 7;          // PWM 채널 번호

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    // ===== 터치 스크린 설정 =====
    {
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;                // 터치 스크린 X 최소값 (생값)
      cfg.x_max = 319;              // 터치 스크린 X 최대값 (생값)
      cfg.y_min = 0;                // 터치 스크린 Y 최소값 (생값)
      cfg.y_max = 479;              // 터치 스크린 Y 최대값 (생값)
      cfg.pin_int = 21;             // INT 핀
      cfg.bus_shared = true;        // SPI 버스 공유
      cfg.offset_rotation = 0;      // 터치 회전 오프셋

      // SPI 설정
      cfg.spi_host = SPI2_HOST;     // SPI2 사용
      cfg.freq = 1000000;           // SPI 클럭 (1MHz)
      cfg.pin_sclk = 12;            // SCLK 핀
      cfg.pin_mosi = 11;            // MOSI 핀
      cfg.pin_miso = 13;            // MISO 핀
      cfg.pin_cs = 14;              // CS 핀

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

// 전역 LGFX 인스턴스
static LGFX tft;

#endif // LOVYANGFX_CONFIG_HPP


