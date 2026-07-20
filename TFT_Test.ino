
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  // could be 0,1,2,3
  tft.setRotation(1);
  //If you have strange colours
  //tft.invertDisplay(true);
  //tft.invertDisplay(false);
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Hello E32R28T-1", 20, 20, 4);
}

void loop() {
}
