// User_Setup.h for ESP32-2432S028 (Cheap Yellow Display 2.8")
// Specific pin definitions for ILI9341 display and XPT2046 touch controller
#define USER_SETUP_INFO "ESP32-2432S028_CYD_Setup"

#define ILI9341_2_DRIVER     // Alternative ILI9341 driver with corrected color inversion

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Display SPI Pins
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1          // Connect to ESP32 RST or -1
#define TFT_BL   21          // Backlight control pin (HIGH = ON)
#define TFT_BACKLIGHT_ON HIGH

// XPT2046 Resistive Touch Controller
#define TOUCH_CS 33

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// SPI Speed
#define SPI_FREQUENCY       40000000 // 40MHz for fast rendering
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
