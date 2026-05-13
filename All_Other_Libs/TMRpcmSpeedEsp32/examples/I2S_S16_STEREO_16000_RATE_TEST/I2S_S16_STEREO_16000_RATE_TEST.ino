/*
  TMRpcmSpeedEsp32 v4.2-XT - test rate moteur

  Fichier SD attendu:
    /SCAN-V12_16B.IDL

  Format:
    WAV PCM signed 16-bit stereo 16000 Hz
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

#define WAV_FILE "/SCAN-V12_16B.IDL"

TMRpcmSpeedEsp32 audio;

float rate = 0.50f;
bool up = true;

// Zones observées comme moins propres sur ton montage.
// On les saute dans l'exemple, sans modifier la lib.
static float avoidBadRateZones(float r, bool rising) {
  if (r >= 0.64f && r <= 0.68f) {
    return rising ? 0.69f : 0.63f;
  }

  // Tu as remarqué un comportement douteux après ~1.32.
  // Pour valider proprement, on limite d'abord à 1.30.
  if (r > 1.30f) {
    return 1.30f;
  }

  return r;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("TMRpcmSpeedEsp32 v4.2-XT - RATE test");

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
  audio.setFormat(TMRPCM_FMT_S16_STEREO_16000);

  audio.loopPlayback = true;
  audio.setVolume(100);        // niveau original, plus pur que 150/200
  audio.setPlaybackRate(rate);

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
      if (rate <= 0.5f) {
        rate = 0.5f;
        up = true;
      }
    }

    rate = avoidBadRateZones(rate, up);
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
