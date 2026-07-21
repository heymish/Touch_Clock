#include <Arduino.h>
#include <LovyanGFX.hpp>

// Test sketch for E32R28T-1 ST7789 display + XPT2046 touch
// Purpose: Verify GPIO pins and SPI setup for display and touch

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
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
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
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.memory_width = 240;
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
      cfg.spi_host = SPI2_HOST;
      cfg.pin_cs = 33;
      cfg.pin_int = 36;
      cfg.pin_sclk = 25;
      cfg.pin_mosi = 32;
      cfg.pin_miso = 39;
      cfg.bus_shared = false;
      cfg.freq = 1000000;  // 1MHz - safe frequency
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};

LGFX tft;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== E32R28T-1 Display & Touch Test ===\n");
  
  // Initialize backlight
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);
  Serial.println("✓ Backlight initialized on GPIO21");
  
  // Initialize display
  Serial.println("Initializing display...");
  tft.init();
  tft.setColorDepth(16);
  tft.fillScreen(TFT_BLACK);
  Serial.println("✓ Display initialized successfully");
  
  // Test display by showing colors
  Serial.println("\nTesting display colors...");
  delay(500);
  tft.fillScreen(TFT_RED);
  Serial.println("  - Red screen");
  delay(500);
  
  tft.fillScreen(TFT_GREEN);
  Serial.println("  - Green screen");
  delay(500);
  
  tft.fillScreen(TFT_BLUE);
  Serial.println("  - Blue screen");
  delay(500);
  
  tft.fillScreen(TFT_BLACK);
  
  // Draw test UI
  tft.setTextDatum(middle_center);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("E32R28T-1 Test", 120, 50, &fonts::Font4);
  tft.drawString("Display: OK", 120, 100, &fonts::Font2);
  tft.drawString("Touch test:", 120, 140, &fonts::Font2);
  tft.drawString("Tap the screen", 120, 170, &fonts::Font2);
  tft.drawString("Coordinates will", 120, 200, &fonts::Font2);
  tft.drawString("show in Serial", 120, 220, &fonts::Font2);
  
  Serial.println("\n✓ Display color test passed");
  Serial.println("\nTouch test started. Tap the screen and check Serial output.");
  Serial.println("Expected: Touch coordinates (0-240, 0-320)\n");
}

void loop() {
  // Poll touch every 100ms
  static unsigned long lastPoll = 0;
  unsigned long now = millis();
  
  if (now - lastPoll >= 100) {
    lastPoll = now;
    
    // Get touch data
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      Serial.printf("Touch detected: X=%d, Y=%d\n", x, y);
      
      // Draw touch indicator on screen
      tft.fillCircle(x, y, 5, TFT_YELLOW);
      tft.drawCircle(x, y, 10, TFT_CYAN);
      
      // Update serial info
      char buf[64];
      sprintf(buf, "X:%d Y:%d  ", x, y);
      tft.fillRect(0, 280, 240, 40, TFT_BLACK);
      tft.setTextDatum(top_center);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString(buf, 120, 290, &fonts::Font2);
      
      delay(100);  // Brief hold to see the circle
      
      // Clear the circle
      tft.fillCircle(x, y, 15, TFT_BLACK);
      tft.fillRect(0, 280, 240, 40, TFT_BLACK);
    }
  }
}
