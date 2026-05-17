#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <EspBoatAudio.h>

#define SD_CS    10
#define SD_SCK   12
#define SD_MISO  13
#define SD_MOSI  11

#define I2S_BCLK 4
#define I2S_LRCK 5
#define I2S_DOUT 6

SPIClass sdSPI(FSPI);
SdFat sd;
EspBoatAudio audio;

float pitch = 0.75f;
float dir   = 0.01f;

uint32_t lastUpdate = 0;

void pumpAudio()
{
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.chainTick();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(25), &sdSPI))) {
    Serial.println("SD ERROR");
    return;
  }

  if (!audio.begin(I2S_BCLK, I2S_LRCK, I2S_DOUT, 44100, I2S_NUM_0, 0)) {
    Serial.println("AUDIO ERROR");
    return;
  }

  audio.setMasterVolume(0.8f);

  if (!audio.preloadEngineLoop("/ENGINES/VAPEUR_IDL.wav", 0.7f, 1.0f, 255)) {
    Serial.println("ENGINE PRELOAD ERROR");
    return;
  }

  Serial.println("ENGINE PRELOADED");

  audio.playPreloadedEngineLoop();
  Serial.println("ENGINE PLAY");
}

void loop()
{
  pumpAudio();

  if (millis() - lastUpdate >= 40) {
    lastUpdate = millis();

    pitch += dir;

    if (pitch >= 1.30f) {
      pitch = 1.30f;
      dir = -0.01f;
    }

    if (pitch <= 0.75f) {
      pitch = 0.75f;
      dir = 0.01f;
    }

    float volume = 0.35f + ((pitch - 0.75f) / (1.30f - 0.75f)) * 0.35f;

    audio.engineSetPitch(pitch);
    audio.engineSetVolume(volume);

    Serial.print("Pitch=");
    Serial.print(pitch);
    Serial.print(" Volume=");
    Serial.println(volume);
  }
}