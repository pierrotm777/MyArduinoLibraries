#include <SPI.h>
#include <SD.h>
#include <TMRpcmSpeed32u4.h>

#define SD_CS 10   // adapte si besoin

TMRpcmSpeed32u4 audio;

float rate = 1.0;
bool up = true;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Init SD...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAIL");
    while (1);
  }

  Serial.println("SD OK");

  // Vérifie présence fichier
  if (!SD.exists("SCAN-V12.IDL")) {
    Serial.println("Fichier introuvable !");
    while (1);
  }

  Serial.println("Fichier OK");

  // Lancer lecture
  if (!audio.play("SCAN-V12.IDL")) {
    Serial.print("PLAY FAIL: ");
    Serial.println(audio.getLastError());
    while (1);
  }

  Serial.println("Lecture OK");
}

void loop() {
  // ⚠️ OBLIGATOIRE : maintient le flux audio
  audio.update();

  static unsigned long last = 0;

  if (millis() - last > 50) {
    last = millis();

    // variation du pitch
    if (up) {
      rate += 0.02;
      if (rate >= 2.0) {
        rate = 2.0;
        up = false;
      }
    } else {
      rate -= 0.02;
      if (rate <= 0.5) {
        rate = 0.5;
        up = true;
      }
    }

    audio.setPlaybackRate(rate);

    Serial.print("Rate: ");
    Serial.println(rate);
  }
}