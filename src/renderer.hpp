#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.hpp"
#include "track_data.hpp"

// Renderer pseudo-3D a segmenti (curve + hill) con DOUBLE-BUFFER via sprite 8-bit.
// Comporre in RAM elimina ghost/flicker; un solo pushSprite riduce il tearing.
// Il game compone il frame (renderRace + drawCar + HUD su canvas) poi chiama push().
class Renderer {
public:
  bool begin(TFT_eSPI* tft);
  void loadTrack(uint8_t trackId);

  // Disegna cielo+montagne+strada nello sprite (NO auto, NO HUD, NO push).
  void renderRace(float position, float playerX);
  // Disegna l'auto del giocatore in basso, spostata da playerX.
  void drawCar(float playerX);
  // Riempe tutto lo sprite (per menu/gameover).
  void wipe(uint16_t color);
  // Accesso al canvas per disegnare HUD/testo/bottoni.
  TFT_eSprite* canvas() { return spr_; }
  // Buffer 8-bit grezzo dello sprite (per dump screenshot). Stride == SCREEN_W.
  uint8_t* framebuffer8() { return spr_ ? (uint8_t*)spr_->getPointer() : nullptr; }
  // Trasferisce lo sprite al TFT.
  void push();

  uint16_t trackCount() const { return trackCount_; }
  float    trackLength() const { return trackCount_ * segmentLength; }
  // Curvatura del segmento alla posizione data (per forza centrifuga).
  float curveAt(float position) const {
    if (trackCount_ == 0) return 0.0f;
    float p = fmodf(position, trackLength());
    if (p < 0.0f) p += trackLength();
    return track_[(uint16_t)(p / segmentLength) % trackCount_].curve;
  }

  static constexpr float    segmentLength = 200.0f;
  static constexpr float    roadWidth     = 2000.0f;
  static constexpr float    cameraHeight  = 1000.0f;
  static constexpr float    cameraDepth   = 0.8390996f; // 1/tan(50deg), fov=100
  static constexpr uint16_t drawDistance  = 150;
  static constexpr uint16_t rumbleLength  = 3;
  static constexpr uint16_t maxSegments   = 2000;

private:
  struct Proj { float x, y, w; };  // x=center, y=screenY, w=halfWidth (px)

  TFT_eSPI*    tft_ = nullptr;
  TFT_eSprite* spr_ = nullptr;
  Segment      track_[maxSegments];
  uint16_t     trackCount_ = 0;
  Proj         p1_[drawDistance];
  Proj         p2_[drawDistance];

  float camX_ = 0, camY_ = 0;

  void project(Proj& p, float worldX, float worldY, float relZ);
  void drawMountains();
  void drawSegment(uint16_t i, const Proj& p1, const Proj& p2);
};
