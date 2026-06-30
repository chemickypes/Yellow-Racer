#include "renderer.hpp"

bool Renderer::begin(TFT_eSPI* tft) {
  tft_ = tft;
  spr_ = new TFT_eSprite(tft_);
  spr_->setColorDepth(8);  // RGB332, 77KB -> entra nel blocco contiguo (~108KB)
  if (spr_->createSprite(SCREEN_W, SCREEN_H) == nullptr) {
    Serial.printf("SPRITE 8-bit OOM free=%u max=%u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    delete spr_;
    spr_ = nullptr;
    return false;
  }
  Serial.printf("SPRITE 8-bit OK free=%u\n", ESP.getFreeHeap());
  loadTrack(0);
  return true;
}

void Renderer::loadTrack(uint8_t trackId) {
  trackCount_ = buildTrack(trackId, track_, maxSegments);
}

void Renderer::push() {
  if (spr_) spr_->pushSprite(0, 0);
}

void Renderer::wipe(uint16_t color) {
  if (spr_) spr_->fillRect(0, 0, SCREEN_W, SCREEN_H, color);
}

void Renderer::project(Proj& p, float worldX, float worldY, float relZ) {
  if (relZ < 1.0f) relZ = 1.0f;                 // clamp dietro-camera
  float scale = cameraDepth / relZ;
  p.x = (SCREEN_W * 0.5f) + scale * (worldX - camX_) * (SCREEN_W * 0.5f);
  p.y = (SCREEN_H * 0.5f) - scale * (worldY - camY_) * (SCREEN_H * 0.5f);
  p.w = scale * roadWidth * (SCREEN_W * 0.5f);
}

void Renderer::drawMountains() {
  const int16_t hz = SCREEN_H / 2;
  spr_->fillTriangle(SCREEN_W / 2, hz - 46, SCREEN_W / 2 - 58, hz, SCREEN_W / 2 + 58, hz, col::MOUNTAIN);
  spr_->fillTriangle(SCREEN_W / 2, hz - 46, SCREEN_W / 2 - 16, hz - 28, SCREEN_W / 2 + 16, hz - 28, col::SNOW);
  spr_->fillTriangle(60, hz - 28, 22, hz, 100, hz, col::MOUNTAIN);
  spr_->fillTriangle(SCREEN_W - 52, hz - 24, SCREEN_W - 96, hz, SCREEN_W - 12, hz, col::MOUNTAIN);
}

void Renderer::drawSegment(uint16_t i, const Proj& p1, const Proj& p2) {
  float yA = p2.y;   // far (alto)
  float yB = p1.y;   // near (basso)
  // Tiling ESATTO (lroundf su edge bit-identical): nessun gap, nessun overlap.
  int16_t ys = (int16_t)lroundf(yA);
  int16_t ye = (int16_t)lroundf(yB);
  if (ys < 0) ys = 0;
  if (ye > SCREEN_H) ye = SCREEN_H;
  if (ys >= ye) return;

  uint16_t group  = (i / rumbleLength) % 2;
  uint16_t grass  = group ? col::GRASS_L  : col::GRASS_D;
  uint16_t rumble = group ? col::RUMBLE_L : col::RUMBLE_D;
  uint16_t road   = group ? col::ROAD_L   : col::ROAD_D;
  bool     lane   = (group == 0);

  spr_->fillRect(0, ys, SCREEN_W, ye - ys, grass);

  float span = yB - yA;
  if (span < 0.001f) span = 0.001f;
  for (int16_t y = ys; y < ye; y++) {
    float t  = (y - yA) / span;            // 0 = far, 1 = near
    float cx = p2.x + (p1.x - p2.x) * t;
    float cw = p2.w + (p1.w - p2.w) * t;
    if (cw < 0.0f) cw = 0.0f;
    float r  = cw * 0.12f;
    int16_t lx = (int16_t)(cx - cw);
    int16_t rx = (int16_t)(cx + cw);
    int16_t rw = (int16_t)r;  if (rw < 1) rw = 1;
    spr_->drawFastHLine(lx - rw, y, rw, rumble);
    spr_->drawFastHLine(rx, y, rw, rumble);
    int16_t roadW = rx - lx;
    if (roadW >= 1) {
      spr_->drawFastHLine(lx, y, roadW, road);
      if (lane && roadW > 6) {
        int16_t dw = roadW / 20;  if (dw < 1) dw = 1;
        spr_->drawFastHLine((int16_t)cx - dw, y, dw * 2, col::LANE);
      }
    } else {
      spr_->drawFastHLine((int16_t)cx, y, 1, road);
    }
  }
}

void Renderer::drawCar(float playerX) {
  if (!spr_) return;
  const int16_t cx = SCREEN_W / 2 + (int16_t)(playerX * (SCREEN_W * 0.30f));
  const int16_t cy = SCREEN_H - 36;
  // ombra
  spr_->fillRect(cx - 30, cy + 17, 60, 4, col::ROAD_D);
  // pneumatici (sporgenti ai lati)
  spr_->fillRect(cx - 30, cy - 8, 8, 22, car::TIRE);
  spr_->fillRect(cx + 22, cy - 8, 8, 22, car::TIRE);
  // corpo
  spr_->fillRect(cx - 26, cy - 12, 52, 28, car::BODY);
  // alettone posteriore
  spr_->fillRect(cx - 32, cy - 16, 64, 6, car::WING);
  // abitacolo (vetro)
  spr_->fillRect(cx - 14, cy - 8, 28, 12, car::GLASS);
  // fanali posteriori
  spr_->fillRect(cx - 22, cy + 6, 10, 4, car::LIGHT);
  spr_->fillRect(cx + 12, cy + 6, 10, 4, car::LIGHT);
  // striscia centrale
  spr_->fillRect(cx - 2, cy - 12, 4, 28, car::LIGHT);
}

void Renderer::renderRace(float position, float playerX) {
  if (!spr_) return;
  const float len = trackLength();
  position = fmodf(position, len);
  if (position < 0.0f) position += len;
  const uint16_t baseSeg = (uint16_t)(position / segmentLength) % trackCount_;
  const float basePercent = (position / segmentLength) - floorf(position / segmentLength);
  const uint16_t nextSeg  = (baseSeg + 1) % trackCount_;
  float playerY = track_[baseSeg].y + (track_[nextSeg].y - track_[baseSeg].y) * basePercent;

  camX_ = playerX * roadWidth;
  camY_ = playerY + cameraHeight;

  float x  = 0.0f;
  float dx = -track_[baseSeg].curve * basePercent;
  float topRoadY = (float)SCREEN_H;
  for (uint16_t n = 0; n < drawDistance; n++) {
    uint16_t i = (baseSeg + n) % trackCount_;
    uint16_t j = (baseSeg + n + 1) % trackCount_;
    float d0 = n * segmentLength - basePercent * segmentLength;
    float d1 = (n + 1) * segmentLength - basePercent * segmentLength;
    project(p1_[n], x,      track_[i].y, d0);
    project(p2_[n], x + dx, track_[j].y, d1);
    if (p2_[n].y < topRoadY) topRoadY = p2_[n].y;
    x  += dx;
    dx += track_[i].curve;
  }

  int16_t skyH = (int16_t)lroundf(topRoadY);
  if (skyH < 0) skyH = 0;
  if (skyH > SCREEN_H) skyH = SCREEN_H;
  spr_->fillRect(0, 0, SCREEN_W, skyH, col::SKY);
  drawMountains();
  for (int16_t n = drawDistance - 1; n >= 0; n--) {
    drawSegment((baseSeg + n) % trackCount_, p1_[n], p2_[n]);
  }
}
