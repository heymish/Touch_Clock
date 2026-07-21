//#include "lgfx/v1/touch/Touch_XPT2046.hpp"
#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

// LovyanGFX configuration for LCDWiki E32R28T-1 / ESP32E 240x320 board
// Display: ST7789 on VSPI (verified working)
// Touch: XPT2046 will use independent SPI on GPIO25/32/39/33

#define TFT_BL_PIN 21

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX(void) {
    {
      auto cfg = _bus.config();

      // Display MUST use VSPI_HOST - this is what works on this board
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
      
      // Touch uses independent pins per LCDWIKI spec
      cfg.spi_host = SPI2_HOST;  // Use SPI2_HOST for touch - separate from display VSPI
      cfg.pin_cs = 33;     // GPIO33 - XPT2046 CS
      cfg.pin_int = 36;    // GPIO36 - XPT2046 interrupt
      cfg.pin_sclk = 25;   // GPIO25 - Touch SCLK
      cfg.pin_mosi = 32;   // GPIO32 - Touch MOSI
      cfg.pin_miso = 39;   // GPIO39 - Touch MISO
      
      cfg.bus_shared = false;  // Separate SPI bus entirely
      cfg.freq = 2500000;

      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};
