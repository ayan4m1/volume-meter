#include <Arduino.h>
#include <TFT_eSPI.h>

#include <Audio.hpp>

TFT_eSPI lcd = TFT_eSPI();
Audio audio = Audio();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  delay(100);
  Serial.println("Awake");

  audio.begin();

  lcd.init();
  lcd.setRotation(1);
  lcd.fillScreen(TFT_BLACK);
}

void loop() {
  double db = audio.getDecibels();

  if (db > 0) {
    Serial.printf("%.1f\n", db);
  }
}