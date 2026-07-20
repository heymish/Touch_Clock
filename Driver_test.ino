
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX(void) {

    {
      auto cfg = _bus.config();

      cfg.spi_host = VSPI_HOST;

      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;

      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;

      cfg.pin_dc = 2;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();

      cfg.pin_cs = 15;
      cfg.pin_rst = -1;

      cfg.panel_width = 240;
      cfg.panel_height = 320;

      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};

LGFX tft;

void setup() {

  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();

  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);

  tft.drawString("Hello", 50, 50, 4);
}

void loop() {
}
