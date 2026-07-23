//Edit TFT_eSPI/User_setup_select.h
//add
//#include <User_setups/Setup_E32R28T_1.h>

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
