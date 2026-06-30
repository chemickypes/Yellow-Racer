#pragma once
#include <Arduino.h>
#include "renderer.hpp"
#include "input.hpp"

enum class GameState : uint8_t { Menu, Race, GameOver };

class Game {
public:
  void begin(Renderer* r, Input* in);
  void update(uint32_t nowMs);
  // Cattura 3 scene (menu/gara/gameover) e le scarica via seriale (RGB332 raw).
  void captureScenes();
  // Cattura N frame di gara (posizione avanzante) per generare una GIF.
  void captureRaceFrames();

private:
  Renderer* r_  = nullptr;
  Input*    in_ = nullptr;

  GameState state_ = GameState::Menu;
  uint8_t   trackId_ = 0;
  bool      won_ = false;

  float    position_ = 0;
  float    speed_    = 0;
  float    playerX_  = 0;
  uint8_t  lap_      = 0;
  int      timeLeft_ = 0;   // ms
  int      score_    = 0;
  uint32_t lapFlashMs_ = 0; // timestamp ultimo passaggio giro

  uint32_t lastMs_ = 0;
  bool     touchPressed_ = false;
  int16_t  touchX_ = 0, touchY_ = 0;
  bool     prevPressed_ = false;

  // layout bottoni menu
  struct Rect { int16_t x, y, w, h; };
  Rect buttonRect(uint8_t i) const;

  void startRace(uint8_t trackId);
  void finishRace(bool won);
  void drawHudBox(int16_t x, int16_t y, int16_t w, int16_t h, const char* s);

  void updateMenu(bool justPressed);
  void updateRace(float dt);
  void updateGameOver(bool justPressed);

  void drawMenu();
  void drawRace();
  void drawGameOver();
  void dumpFrame(const char* name);
  void dumpFrameRaw();
};
