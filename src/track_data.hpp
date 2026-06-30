#pragma once
#include <Arduino.h>

// Segment di pista (modello Jake Gordon).
//  - curve: delta curvatura accumulato su dx. Costante -> curva uniforme.
//  - y    : altezza (world Y assoluta) del bordo near. Differenze -> salite/discese.
struct Segment {
  float curve;
  float y;
};

// Definizione pista. K* = numero di onde complete sul totale N (loop seamless,
// perche' freq = 2*pi*K/N). A* = ampiezza. timeBudget = secondi per 3 giri.
struct TrackDef {
  const char* name;
  int   cK1; float cA1;
  int   cK2; float cA2;
  int   hK1; float hA1;
  int   hK2; float hA2;
  int   timeBudget;
};

const TrackDef TRACKS[NUM_TRACKS] = {
  {"PISTA 1 - FACILE",    3, 1.6f,  7, 0.5f,  2, 700.0f,  5, 180.0f, 85},
  {"PISTA 2 - MEDIA",     4, 2.4f,  9, 0.9f,  3, 1100.0f, 6, 280.0f, 80},
  {"PISTA 3 - DIFFICILE", 5, 3.2f, 11, 1.3f,  2, 1400.0f, 7, 380.0f, 75},
};

// Genera la pista trackId nell'array segs. Ritorna il numero di segmenti.
inline uint16_t buildTrack(uint8_t trackId, Segment* segs, uint16_t capacity) {
  const uint16_t N = 800;   // trackLength = N*200 = 160000 -> 3 giri ~69s a MAX_SPEED
  const uint16_t count = (N < capacity) ? N : capacity;
  const TrackDef& t = TRACKS[trackId % NUM_TRACKS];
  const float w1 = 2.0f * PI * t.cK1 / (float)N;
  const float w2 = 2.0f * PI * t.cK2 / (float)N;
  const float w3 = 2.0f * PI * t.hK1 / (float)N;
  const float w4 = 2.0f * PI * t.hK2 / (float)N;
  for (uint16_t i = 0; i < count; i++) {
    segs[i].curve = sinf(w1 * i) * t.cA1 + sinf(w2 * i) * t.cA2;
    segs[i].y     = sinf(w3 * i) * t.hA1 + sinf(w4 * i) * t.hA2;
  }
  return count;
}
