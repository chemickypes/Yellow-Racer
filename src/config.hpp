#pragma once
#include <Arduino.h>

// ---------------- Display ----------------
constexpr uint16_t SCREEN_W     = 320;
constexpr uint16_t SCREEN_H     = 240;
constexpr uint8_t  TFT_ROTATION = 1;   // landscape 320x240

// ---------------- Touch (XPT2046) - bus SPI dedicato ----------------
// Pin diversi dal display (v. PINS.md CYD). IO36/IO39 sono input-only.
constexpr uint8_t XPT2046_IRQ  = 36;
constexpr uint8_t XPT2046_MISO = 39;
constexpr uint8_t XPT2046_MOSI = 32;
constexpr uint8_t XPT2046_CLK  = 25;
constexpr uint8_t XPT2046_CS   = 33;

// Calibrazione grezza ADC (0..4095) -> pixel. DA AFFINARE con sketch di calibrazione in M3.
// Nota: con setRotation(1) del display potrebbero servire swap/invert degli assi.
constexpr uint16_t TS_MIN_X = 200;
constexpr uint16_t TS_MAX_X = 3700;
constexpr uint16_t TS_MIN_Y = 240;
constexpr uint16_t TS_MAX_Y = 3800;

// ---------------- Palette RGB565 (usata da M2) ----------------
namespace col {
constexpr uint16_t SKY      = 0x6BD7;   // azzurro cielo
constexpr uint16_t GRASS_D  = 0x0360;   // prato scuro
constexpr uint16_t GRASS_L  = 0x07C0;   // prato chiaro
constexpr uint16_t ROAD_D   = 0x4208;   // asfalto scuro
constexpr uint16_t ROAD_L   = 0x5288;   // asfalto chiaro
constexpr uint16_t RUMBLE_D = 0x8400;   // cordolo rosso
constexpr uint16_t RUMBLE_L = 0xFFFF;   // cordolo bianco
constexpr uint16_t LANE     = 0xFFFF;   // linea tratteggiata centrale
constexpr uint16_t MOUNTAIN = 0x6B4D;   // marrone montagne
constexpr uint16_t SNOW     = 0xFFFF;
constexpr uint16_t HUD_FG   = 0xFFFF;
constexpr uint16_t HUD_BG   = 0x0000;
constexpr uint16_t MENU_BTN = 0x4A9F;
} // namespace col

// ---------------- Auto (RGB565, disegnata con primitive) ----------------
namespace car {
constexpr uint16_t BODY  = 0xD800;  // rosso
constexpr uint16_t WING  = 0xA800;  // rosso scuro (alettone)
constexpr uint16_t GLASS = 0x2104;  // blu scuro (abitacolo)
constexpr uint16_t TIRE  = 0x0000;  // nero (pneumatici)
constexpr uint16_t LIGHT = 0xFFE0;  // giallo (fanali)
} // namespace car

// ---------------- Game ----------------
constexpr uint8_t  NUM_TRACKS  = 3;
constexpr uint8_t  TOTAL_LAPS  = 3;
constexpr float    MAX_SPEED   = 7000.0f;   // world units/s (giro ~23s su N=800)
constexpr float    STEER_SPEED = 2.4f;      // velocita' lerp playerX
constexpr float    PLAYER_X_MAX = 1.4f;     // limite laterale (oltre 1 = fuori strada)
constexpr float    OFFROAD_MIN_SPEED = 800.0f;
constexpr float    OFFROAD_DRAG = 2500.0f;  // decelerazione sull'erba (units/s^2)
constexpr float    CENTRIFUGAL  = 0.4f;     // spinta verso l'esterno in curva (x speedRatio)
constexpr uint16_t LAP_FLASH_MS = 1500;     // durata flash "GIRO n"
