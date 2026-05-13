/*
  TMRpcmSpeedEsp32 v4.2-XT - test rate I2S S16 stereo 44100

  Fichier SD attendu:
    /SCAN-V12_IDL.wav

  Format:
    WAV PCM signed 16-bit stereo 44100 Hz
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcmSpeedEsp32.h>

#define I2S_BCLK  8
#define I2S_WS    12
#define I2S_DOUT  5

#define SD_MISO 9
#define SD_MOSI 10
#define SD_SCK  11
#define SD_CS   13

#define WAV_FILE "/SCAN-V12_IDL.wav"

TMRpcmSpeedEsp32 audio;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("TMRpcmSpeedEsp32 v4.2-XT - S16 stereo 44100 RATE test");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  delay(100);

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD FAIL");
    while (1) delay(100);
  }
  Serial.println("SD OK");

  if (!SD.exists(WAV_FILE)) {
    Serial.println("Fichier introuvable");
    while (1) delay(100);
  }
  Serial.println("Fichier OK");

  audio.useI2S();
  audio.setI2SPins(I2S_BCLK, I2S_WS, I2S_DOUT);

  // Option stricte 44100:
  audio.setFormat(TMRPCM_FMT_S16_STEREO_44100);

  // Alternative possible:
  // audio.setFormat(TMRPCM_FMT_AUTO);

  audio.loopPlayback = true;
  audio.setVolume(100);
  audio.setPlaybackRate(0.75f);

  if (!audio.begin()) {
    Serial.print("audio.begin FAIL: ");
    Serial.println(audio.getLastError());
    while (1) delay(100);
  }

  Serial.print("Lib version: ");
  Serial.println(audio.version());

  if (!audio.play(WAV_FILE)) {
    Serial.print("audio.play FAIL: ");
    Serial.println(audio.getLastError());
    while (1) delay(100);
  }

  Serial.print("WAV: ");
  Serial.print(audio.getWavSampleRate());
  Serial.print(" Hz, ");
  Serial.print(audio.getWavBitsPerSample());
  Serial.print(" bits, ch=");
  Serial.println(audio.getWavChannels());
}

float rate = 0.75f;
bool up = true;

void loop() {
  audio.update();

  static uint32_t lastRate = 0;
  if (millis() - lastRate >= 250) {
    lastRate = millis();

    if (up) {
      rate += 0.01f;
      if (rate >= 1.30f) {
        rate = 1.30f;
        up = false;
      }
    } else {
      rate -= 0.01f;
      if (rate <= 0.75f) {
        rate = 0.75f;
        up = true;
      }
    }

    audio.setPlaybackRate(rate);
  }

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.print("rate=");
    Serial.print(audio.getPlaybackRate(), 2);
    Serial.print(" volume=");
    Serial.println(audio.getVolume());
  }
}
