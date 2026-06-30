#include "game.hpp"

void Game::begin(Renderer* r, Input* in) {
  r_  = r;
  in_ = in;
  lastMs_ = millis();
}

Game::Rect Game::buttonRect(uint8_t i) const {
  // 3 bottoni verticali centrati, 200x46, partenza y=78, spacing 56
  return Rect{ (int16_t)((SCREEN_W - 200) / 2), (int16_t)(78 + i * 56), 200, 46 };
}

void Game::startRace(uint8_t trackId) {
  trackId_ = trackId;
  r_->loadTrack(trackId);
  position_ = 0;
  speed_    = 0;
  playerX_  = 0;
  lap_      = 0;
  score_    = 0;
  lapFlashMs_ = 0;
  timeLeft_ = TRACKS[trackId].timeBudget * 1000;
  state_    = GameState::Race;
}

void Game::finishRace(bool won) {
  won_   = won;
  score_ = won ? (timeLeft_ / 10)           // ms/10 = (secondi * 100)
               : (int)lap_ * 1000;
  state_ = GameState::GameOver;
}

void Game::update(uint32_t nowMs) {
  float dt = (nowMs - lastMs_) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f;
  if (dt < 0.0f) dt = 0.0f;
  lastMs_ = nowMs;

  TouchPoint tp = in_->read();
  touchX_ = tp.x;
  touchY_ = tp.y;
  touchPressed_ = tp.pressed;
  bool justPressed = tp.pressed && !prevPressed_;
  prevPressed_ = tp.pressed;

  switch (state_) {
    case GameState::Menu:     updateMenu(justPressed);     drawMenu();     break;
    case GameState::Race:     updateRace(dt);              drawRace();     break;
    case GameState::GameOver: updateGameOver(justPressed); drawGameOver(); break;
  }
}

// ---------------- Menu ----------------
void Game::updateMenu(bool justPressed) {
  if (!justPressed) return;
  for (uint8_t i = 0; i < NUM_TRACKS; i++) {
    Rect b = buttonRect(i);
    if (touchX_ >= b.x && touchX_ <= b.x + b.w &&
        touchY_ >= b.y && touchY_ <= b.y + b.h) {
      startRace(i);
      return;
    }
  }
}

void Game::drawMenu() {
  r_->wipe(col::SKY);
  TFT_eSprite* c = r_->canvas();
  if (!c) return;
  c->setTextColor(col::HUD_FG);
  c->setTextDatum(TC_DATUM);
  c->drawString("YELLOW RACER", SCREEN_W / 2, 22, 4);
  c->setTextColor(col::LANE);
  c->drawString("tocca una pista", SCREEN_W / 2, 52, 2);
  for (uint8_t i = 0; i < NUM_TRACKS; i++) {
    Rect b = buttonRect(i);
    c->fillRect(b.x, b.y, b.w, b.h, col::MENU_BTN);
    c->drawRect(b.x, b.y, b.w, b.h, col::HUD_FG);
    c->setTextColor(col::HUD_FG);
    c->setTextDatum(MC_DATUM);
    c->drawString(TRACKS[i].name, b.x + b.w / 2, b.y + b.h / 2, 2);
  }
  r_->push();
}

// ---------------- Race ----------------
void Game::updateRace(float dt) {
  // accelerazione: in strada rampa verso MAX, sull'erba decelera
  bool offRoad = (playerX_ < -1.0f || playerX_ > 1.0f);
  if (offRoad) {
    speed_ -= OFFROAD_DRAG * dt;
    if (speed_ < OFFROAD_MIN_SPEED) speed_ = OFFROAD_MIN_SPEED;
  } else {
    speed_ += (MAX_SPEED - speed_) * 0.9f * dt;
    if (speed_ > MAX_SPEED) speed_ = MAX_SPEED;
  }

  // sterzo touch: meta' sx = -1, meta' dx = +1. NESSUN recentraggio al rilascio:
  // l'auto mantiene la posizione laterale; la forza centrifuga la spinge in curva.
  if (touchPressed_) {
    float target = (touchX_ < SCREEN_W / 2) ? -1.0f : 1.0f;
    playerX_ += (target - playerX_) * STEER_SPEED * dt;
  }
  // forza centrifuga: sposta l'auto verso l'esterno della curva (deve sterzare per restare in strada)
  float curve = r_->curveAt(position_);
  playerX_ -= curve * (speed_ / MAX_SPEED) * CENTRIFUGAL * dt;
  if (playerX_ < -PLAYER_X_MAX) playerX_ = -PLAYER_X_MAX;
  if (playerX_ >  PLAYER_X_MAX) playerX_ =  PLAYER_X_MAX;

  // avanzamento
  position_ += speed_ * dt;
  while (position_ >= r_->trackLength()) {
    position_ -= r_->trackLength();
    lap_++;
    lapFlashMs_ = millis();
    if (lap_ >= TOTAL_LAPS) { finishRace(true); return; }
  }

  // tempo
  timeLeft_ -= (int)(dt * 1000);
  if (timeLeft_ <= 0) { timeLeft_ = 0; finishRace(false); return; }
}

void Game::drawHudBox(int16_t x, int16_t y, int16_t w, int16_t h, const char* s) {
  TFT_eSprite* c = r_->canvas();
  c->fillRect(x, y, w, h, col::HUD_BG);
  c->drawRect(x, y, w, h, col::HUD_FG);
  c->setTextColor(col::HUD_FG);
  c->setTextDatum(MC_DATUM);
  c->drawString(s, x + w / 2, y + h / 2, 2);
}

void Game::drawRace() {
  r_->renderRace(position_, playerX_);
  r_->drawCar(playerX_);

  char buf[20];
  uint8_t lapShown = (lap_ < TOTAL_LAPS) ? (uint8_t)(lap_ + 1) : TOTAL_LAPS;
  // Box in alto a sx: durante il flash mostra "GIRO n!" evidenziato, altrimenti "LAP x/3".
  bool flashing = lapFlashMs_ && (millis() - lapFlashMs_ < LAP_FLASH_MS);
  if (flashing) {
    uint8_t ln = (lap_ < TOTAL_LAPS) ? (uint8_t)(lap_ + 1) : TOTAL_LAPS;
    snprintf(buf, sizeof(buf), "GIRO %d!", ln);
    TFT_eSprite* c = r_->canvas();
    c->fillRect(2, 2, 78, 18, col::LANE);   // sfondo bianco -> evidenziazione
    c->drawRect(2, 2, 78, 18, col::HUD_FG);
    c->setTextColor(col::HUD_BG);            // testo nero su bianco
    c->setTextDatum(MC_DATUM);
    c->drawString(buf, 2 + 39, 2 + 9, 2);
  } else {
    snprintf(buf, sizeof(buf), "LAP %d/%d", lapShown, TOTAL_LAPS);
    drawHudBox(2, 2, 78, 18, buf);
  }

  snprintf(buf, sizeof(buf), "T %ds", timeLeft_ / 1000);
  drawHudBox(SCREEN_W / 2 - 39, 2, 78, 18, buf);

  snprintf(buf, sizeof(buf), "SPD %d", (int)(speed_ / 10));
  drawHudBox(SCREEN_W - 80, 2, 78, 18, buf);

  r_->push();
}

// ---------------- GameOver ----------------
void Game::updateGameOver(bool justPressed) {
  if (justPressed) state_ = GameState::Menu;
}

void Game::drawGameOver() {
  r_->wipe(col::HUD_BG);
  TFT_eSprite* c = r_->canvas();
  if (!c) return;
  c->setTextColor(col::HUD_FG);
  c->setTextDatum(TC_DATUM);
  c->drawString(won_ ? "HAI VINTO!" : "TEMPO SCADUTO", SCREEN_W / 2, 70, 4);
  char buf[24];
  snprintf(buf, sizeof(buf), "PUNTI: %d", score_);
  c->drawString(buf, SCREEN_W / 2, 120, 2);
  c->setTextColor(col::LANE);
  c->drawString("tocca per il menu", SCREEN_W / 2, 168, 2);
  r_->push();
}

// ---------------- Capture (screenshot via seriale) ----------------
void Game::dumpFrame(const char* name) {
  const uint8_t* fb = r_->framebuffer8();
  if (!fb) { Serial.println("SHOT:none"); return; }
  Serial.write((const uint8_t*)"SHOT:", 5);
  Serial.write((const uint8_t*)name, strlen(name));
  Serial.write('\n');
  Serial.write(fb, (size_t)SCREEN_W * SCREEN_H);  // RGB332 raw, stride 320
  Serial.write('\n');
  Serial.flush();
}

void Game::captureScenes() {
  Serial.println("\nCAPTURE START");
  // 1) Menu
  state_ = GameState::Menu;
  drawMenu();
  dumpFrame("menu");
  delay(60);

  // 2) Gara: pista 1 (index 0), curva a sinistra + hill lieve, scena bilanciata
  r_->loadTrack(0);
  position_   = 40000.0f;
  playerX_    = -0.3f;
  lap_        = 1;
  timeLeft_   = 60000;
  speed_      = 6500.0f;
  lapFlashMs_ = 0;
  drawRace();
  dumpFrame("race");
  delay(60);

  // 3) GameOver (vittoria)
  won_   = true;
  score_ = 4200;
  drawGameOver();
  dumpFrame("gameover");

  Serial.println("CAPTURE DONE");
  state_ = GameState::Menu;  // riprende normale
}

void Game::dumpFrameRaw() {
  const uint8_t* fb = r_->framebuffer8();
  if (!fb) return;
  Serial.write((const uint8_t*)"FRAME\n", 6);
  Serial.write(fb, (size_t)SCREEN_W * SCREEN_H);  // RGB332 raw, stride 320
  Serial.flush();
}

void Game::captureRaceFrames() {
  Serial.println("\nGIF START");
  r_->loadTrack(0);            // pista 1
  position_   = 30000.0f;
  lap_        = 1;
  timeLeft_   = 60000;
  speed_      = 6500.0f;
  lapFlashMs_ = 0;
  const int   NFRAMES = 60;
  const float delta   = 300.0f;   // ~1 frame a 24fps a MAX_SPEED
  for (int i = 0; i < NFRAMES; i++) {
    // lieve controsterzo per seguire la curva (sembra guida reale)
    playerX_ = -r_->curveAt(position_) * 0.15f;
    if (playerX_ >  0.8f) playerX_ =  0.8f;
    if (playerX_ < -0.8f) playerX_ = -0.8f;
    drawRace();
    dumpFrameRaw();
    position_ += delta;
  }
  Serial.println("GIF DONE");
  state_ = GameState::Menu;
}
