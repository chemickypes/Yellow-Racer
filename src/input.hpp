#pragma once
#include <Arduino.h>

struct TouchPoint {
  uint16_t x = 0;       // coordinata schermo (mappata)
  uint16_t y = 0;
  uint16_t rx = 0;      // raw ADC (per calibrazione)
  uint16_t ry = 0;
  bool pressed = false;
};

// Wrapper minimo attorno a XPT2046_Touchscreen su bus SPI dedicato.
class Input {
public:
  void begin();
  // Restituisce punto touch mappato sui pixel dello schermo (con raw per debug).
  TouchPoint read();
  // Sterzo: -1 sinistra, +1 destra, 0 nessuno (usato da M3).
  int8_t steer();

private:
  bool ready_ = false;
};
