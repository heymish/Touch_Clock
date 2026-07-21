//#include "lgfx/v1/touch/Touch_XPT2046.hpp"
#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

// LovyanGFX configuration for LCDWiki E32R28T-1 / ESP32E 240x320 board
// Official pin assignments from: https://www.lcdwiki.com/2.8inch_ESP32-32E_Display
// Display: ST7789 on SPI1 (HSPI)
// Touch: XPT2046 on SPI2 (VSPI)

#define TFT_BL_PIN 21

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX(void) {
    {
      auto cfg = _bus.config();

      // Display uses SPI1 (HSPI) - NOT VSPI
      cfg.spi_host = SPI1_HOST;  // Changed from VSPI_HOST to SPI1_HOST
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;

      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      // Official pins for LCD (LCDWIKI spec)
      cfg.pin_sclk = 14;   // Official: IO14
      cfg.pin_mosi = 13;   // Official: IO13
      cfg.pin_miso = 12;   // Official: IO12
      cfg.pin_dc   = 2;    // Official: IO2 (this is correct per datasheet)

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();

      cfg.pin_cs  = 15;    // Official: IO15
      cfg.pin_rst = -1;    // Official: shared with EN/reset
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
      
      // Touch uses SPI2 (VSPI) - separate from display
      cfg.spi_host = SPI2_HOST;  // Touch on VSPI
      
      // Official pins for resistive touch (LCDWIKI spec)
      cfg.pin_cs = 33;     // Official: IO33
      cfg.pin_int = 36;    // Official: IO36 (touch interrupt)
      cfg.pin_sclk = 25;   // Official: IO25
      cfg.pin_mosi = 32;   // Official: IO32
      cfg.pin_miso = 39;   // Official: IO39
      
      cfg.bus_shared = false;  // Separate SPI bus, so not shared
      cfg.freq = 2500000;

      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};
