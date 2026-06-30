#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.hpp"
#include "renderer.hpp"
#include "input.hpp"
#include "game.hpp"

TFT_eSPI  tft;
Renderer  renderer;
Input     input;
Game      game;

void setup() {
  Serial.begin(115200);
  delay(100);
  tft.init();
  tft.setRotation(TFT_ROTATION);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

  input.begin();
  renderer.begin(&tft);
  game.begin(&renderer, &input);
  Serial.println("Yellow Racer M3 - game");
}

void loop() {
  if (Serial.available()) {
    int c = Serial.read();
    if (c == 's') game.captureScenes();
    else if (c == 'g') game.captureRaceFrames();
  }
  game.update(millis());
}
