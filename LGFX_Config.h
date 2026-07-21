//#include "lgfx/v1/touch/Touch_XPT2046.hpp"
#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

// LovyanGFX configuration for LCDWiki E32R28T-1 / ESP32E 240x320 board
// Verified working pins from your blue-screen / white "Hello" test.
// Display controller: ST7789
// Resolution: 240 x 320
// Backlight: GPIO21

#define TFT_BL_PIN 21

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX(void) {
    {
      auto cfg = _bus.config();

      cfg.spi_host = VSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;

      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc   = 2;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();

      cfg.pin_cs  = 15;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;

      cfg.panel_width  = 240;
      cfg.panel_height = 320;
      cfg.memory_width  = 240;
      cfg.memory_height = 320;

      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;

      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel.config(cfg);
    }

    {
      auto cfg = _touch.config();

      cfg.x_min = 0;
      cfg.x_max = 240;
      cfg.y_min = 0;
      cfg.y_max = 320;
      cfg.pin_cs = 4;
      cfg.pin_int = -1;
      cfg.bus_shared = true;
      cfg.freq = 2500000;

      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};
