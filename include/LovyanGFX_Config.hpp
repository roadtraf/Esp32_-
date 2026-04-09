
#ifndef LOVYANGFX_CONFIG_HPP
#define LOVYANGFX_CONFIG_HPP

#include <LovyanGFX.hpp>

// ESP32-S3 LovyanGFX 
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX(void) {
    // ===== SPI   =====
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;     // ESP32-S3 SPI2_HOST 
      cfg.spi_mode = 0;             // SPI  0
      cfg.freq_write = 40000000;    //   SPI  (40MHz)
      cfg.freq_read = 16000000;     //   SPI  (16MHz)
      cfg.spi_3wire = false;        // 3-wire  
      cfg.use_lock = true;          //   
      cfg.dma_channel = SPI_DMA_CH_AUTO; // DMA  
      cfg.pin_sclk = 12;            // SPI SCLK 
      cfg.pin_mosi = 11;            // SPI MOSI 
      cfg.pin_miso = 13;            // SPI MISO 
      cfg.pin_dc = 9;               // SPI DC 

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // =====    =====
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 10;              // CS 
      cfg.pin_rst = 8;              // RST 
      cfg.pin_busy = -1;            // BUSY  ()

      cfg.memory_width = 320;       //  IC   
      cfg.memory_height = 480;      //  IC   
      cfg.panel_width = 320;        //   
      cfg.panel_height = 480;       //   
      cfg.offset_x = 0;             //   X
      cfg.offset_y = 0;             //   Y
      cfg.offset_rotation = 0;      //    (0~7)
      cfg.dummy_read_pixel = 8;     //       
      cfg.dummy_read_bits = 1;      //         
      cfg.readable = true;          //   
      cfg.invert = false;           //    
      cfg.rgb_order = false;        //  RGB 
      cfg.dlen_16bit = false;       // 16   
      cfg.bus_shared = true;        // SD   

      _panel_instance.config(cfg);
    }

    // =====   =====
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 15;              //  
      cfg.invert = false;           //    
      cfg.freq = 44100;             //  PWM 
      cfg.pwm_channel = 7;          // PWM  

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    // =====    =====
    {
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;                //   X  ()
      cfg.x_max = 319;              //   X  ()
      cfg.y_min = 0;                //   Y  ()
      cfg.y_max = 479;              //   Y  ()
      cfg.pin_int = 21;             // INT 
      cfg.bus_shared = true;        // SPI  
      cfg.offset_rotation = 0;      //   

      // SPI 
      cfg.spi_host = SPI2_HOST;     // SPI2 
      cfg.freq = 1000000;           // SPI  (1MHz)
      cfg.pin_sclk = 12;            // SCLK 
      cfg.pin_mosi = 11;            // MOSI 
      cfg.pin_miso = 13;            // MISO 
      cfg.pin_cs = 14;              // CS 

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

//  LGFX 
static LGFX tft;

#endif // LOVYANGFX_CONFIG_HPP


