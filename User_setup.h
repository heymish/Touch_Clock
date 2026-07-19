TFT_eSPI/User_setup.h


#define ST7789_DRIVER

#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1
#define TFT_BL   21

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7

#define SPI_FREQUENCY 27000000


//Might need thise to set the rotation
//tft.setRotation(1);
//Try 0, 1, 2, or 3.
