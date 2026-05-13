/*
  TMRpcmSpeedEsp32 v4.2-XT - Mixer test

  Format conseillé pour tous les fichiers:
    WAV PCM signed 16-bit stereo 44100 Hz

  Fichiers attendus sur SD:
    /SCAN-V12_IDL.wav  moteur
    /AUX1.wav
    /AUX2.wav
    /AUX3.wav
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

#define MOTOR_FILE "/SCAN-V12_IDL.wav"

TMRpcmSpeedEsp32 audio;

float rate = 0.85f;
bool up = true;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("TMRpcmSpeedEsp32 v4.2-XT - MIXER TEST");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  delay(100);

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD FAIL");
    while (1) delay(100);
  }
  Serial.println("SD OK");

  audio.useI2S();
  audio.setAudioTaskCore(0);
  audio.setAudioTaskPriority(5);
  audio.setXTChunkFrames(128);
  audio.setI2SPins(I2S_BCLK, I2S_WS, I2S_DOUT);
  audio.setFormat(TMRPCM_FMT_S16_STEREO_44100);

  audio.loopPlayback = true;
  audio.setVolume(100);       // volume global
  audio.setAutoMixNormalize(true);
  audio.setCompressor(22000, 4.0f, 100);  // limiteur doux anti-saturation
  audio.setMotorVolume(80);
  audio.setAuxVolume(0, 60);
  audio.setAuxVolume(1, 60);
  audio.setAuxVolume(2, 60);

  audio.setMotorInterpolation(true);
  audio.setMotorRate(rate);  // agit seulement sur le moteur, pas sur les AUX

  if (!audio.begin()) {
    Serial.print("audio.begin FAIL: ");
    Serial.println(audio.getLastError());
    while (1) delay(100);
  }

  Serial.print("Lib version: ");
  Serial.println(audio.version());

  if (!audio.playMotor(MOTOR_FILE)) {
    Serial.print("playMotor FAIL: ");
    Serial.println(audio.getLastError());
    while (1) delay(100);
  }

  Serial.print("Motor WAV: ");
  Serial.print(audio.getWavSampleRate());
  Serial.print(" Hz, ");
  Serial.print(audio.getWavBitsPerSample());
  Serial.print(" bits, ch=");
  Serial.println(audio.getWavChannels());

  Serial.println("Preload AUX in RAM...");
  if (!audio.preloadAux(0, "/AUX1.wav")) Serial.println(audio.getLastError());
  if (!audio.preloadAux(1, "/AUX2.wav")) Serial.println(audio.getLastError());
  if (!audio.preloadAux(2, "/AUX3.wav")) Serial.println(audio.getLastError());
  Serial.println("Preload done.");
  // Pour une musique longue, utiliser par exemple :
  // audio.playAuxFromSD(2, "/MUSIC4MIN.wav");
}

void loop() {
  audio.update();

  // Test auxiliaires automatiques.
  static uint32_t lastAux = 0;
  static uint8_t aux = 0;

  if (millis() - lastAux >= 4000) {
    lastAux = millis();

    const char *file = nullptr;
    if (aux == 0) file = "/AUX1.wav";
    if (aux == 1) file = "/AUX2.wav";
    if (aux == 2) file = "/AUX3.wav";

    Serial.print("playAux ");
    Serial.print(aux);
    Serial.print(" ");
    Serial.println(file);

    if (!audio.playAux(aux)) {
      Serial.print("playAux FAIL: ");
      Serial.println(audio.getLastError());
    }

    aux++;
    if (aux >= 3) aux = 0;
  }

  // Variation lente du moteur.
  static uint32_t lastRate = 0;
  if (millis() - lastRate >= 300) {
    lastRate = millis();

    if (up) {
      rate += 0.005f;
      if (rate >= 1.25f) {
        rate = 1.25f;
        up = false;
      }
    } else {
      rate -= 0.005f;
      if (rate <= 0.75f) {
        rate = 0.75f;
        up = true;
      }
    }

    audio.setMotorInterpolation(true);
  audio.setMotorRate(rate);  // agit seulement sur le moteur, pas sur les AUX
  }
}
