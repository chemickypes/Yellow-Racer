// User_Setup.h - TFT_eSPI config for ESP32-2432S028R (Cheap Yellow Display / CYD)
// Source: github.com/witnessmenow/ESP32-Cheap-Yellow-Display /DisplayConfig/User_Setup.h
// Iniettato in ogni TU via build_flags "-include src/User_Setup.h"
// USER_SETUP_LOADED e' fornito via -D per bloccare il default di TFT_eSPI.

#define USER_SETUP_INFO "CYD Yellow Racer"

// ---------------- Driver ----------------
// Il CYD monta un ILI9341 (NON ST7789): usa il driver alternativo consigliato dai docs CYD.
#define ILI9341_2_DRIVER

// ---------------- Geometria (portrait nativo) ----------------
// Usiamo tft.setRotation(1) -> landscape 320x240
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ---------------- Pin display (bus HSPI) ----------------
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // RESET collegato al RST della board

// ---------------- Backlight ----------------
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// ---------------- Font ----------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ---------------- Frequenze SPI ----------------
#define SPI_FREQUENCY        55000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Il display usa la porta HSPI.
// (Il touch XPT2046 usa un bus SPI separato, gestito dalla lib XPT2046_Touchscreen)
#define USE_HSPI_PORT
