#include "input.hpp"
#include "config.hpp"
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

namespace {
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
} // namespace

void Input::begin() {
  // Questa versione della lib non accetta SPIClass: usa il bus SPI globale.
  // ATTENZO all'ordine: SPIClass::begin() e' no-op se il bus e' gia' avviato
  // (framework: "if(_spi) return;"). Quindi configuriamo i pin touch PRIMA di
  // ts.begin(), altrimenti il suo SPI.begin() interno blocca i pin sui default
  // VSPI (18/19/23/5) e SCK/MOSI non arrivano al chip (MISO float -> 0xFFFF).
  // Il display usa HSPI (USE_HSPI_PORT), quindi la SPI globale (VSPI) e' libera.
  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ready_ = true;
}

TouchPoint Input::read() {
  TouchPoint p;
  if (!ready_) return p;
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point tp = ts.getPoint();
    p.rx = tp.x;
    p.ry = tp.y;
    // Mappa raw -> pixel. Calibrazione grezza: DA AFFINARE (eventuale swap/invert).
    int16_t mx = map(tp.x, TS_MIN_X, TS_MAX_X, 0, SCREEN_W);
    int16_t my = map(tp.y, TS_MIN_Y, TS_MAX_Y, 0, SCREEN_H);
    p.x = static_cast<uint16_t>(constrain(mx, 0, SCREEN_W - 1));
    p.y = static_cast<uint16_t>(constrain(my, 0, SCREEN_H - 1));
    p.pressed = true;
  }
  return p;
}

int8_t Input::steer() {
  TouchPoint p = read();
  if (!p.pressed) return 0;
  // Meta' schermo: due zone di sterzo. (Uso effettivo da M3.)
  return (p.x < SCREEN_W / 2) ? -1 : 1;
}
