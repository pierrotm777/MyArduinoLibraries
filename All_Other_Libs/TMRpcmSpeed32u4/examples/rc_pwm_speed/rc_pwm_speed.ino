#include <SPI.h>
#include <SD.h>
#define TMRPCM_32U4_AUDIO_TIMER 1  // 1=Timer1 (default), 3=Timer3
#include <TMRpcmSpeed32u4.h>

#define SD_CS     10
#define RC_PIN    2
#define AUDIO_PIN 6   // D6 / OC4D

TMRpcmSpeed32u4 audio;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Minimal engine test");

  pinMode(RC_PIN, INPUT);
  pinMode(AUDIO_PIN, OUTPUT);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAIL");
    while (1);
  }
  Serial.println("SD OK");

  if (!audio.begin()) {
    Serial.println("audio.begin FAIL");
    while (1);
  }
  Serial.println("audio.begin OK");

  audio.setVolume(8);          // 0..7 ou 0..10 selon ta lib
  audio.loopPlayback = true;

  audio.play("/ENGINE.IDL");   // nom simple pour test
}

void loop() {
  // Lecture RC (bloquante, mais OK pour test)
  unsigned long us = pulseIn(RC_PIN, HIGH, 25000); // timeout 25 ms

  if (us >= 1000 && us <= 2000) {
    // map 1000..2000 -> 0.7 .. 1.8
    float rate = 0.7f + (float)(us - 1000) * (1.8f - 0.7f) / 1000.0f;
    audio.setPlaybackRate(rate);

    Serial.print("us=");
    Serial.print(us);
    Serial.print(" rate=");
    Serial.println(rate, 2);
  }

  audio.update();  // TRÈS IMPORTANT
}
