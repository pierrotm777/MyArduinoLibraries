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

EspBoatAudio::AudioHandle cannon;
uint32_t lastFire = 0;

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
}

void loop()
{
  pumpAudio();

  if (millis() - lastFire > 6000) {
    lastFire = millis();

    cannon = audio.playFxAuto("/FIXEDSOUND/CANNON.wav", 0.9f, 220, 1.0f);
    Serial.println("CANNON FIRE");
  }

  if (audio.isPlaying(cannon)) {
    uint32_t pos = audio.positionMillis(cannon);
    uint32_t rem = audio.remainingMillis(cannon);

    if (audio.inWindow(cannon, 100, 250)) {
      Serial.println("LED FLASH WINDOW");
    }

    static uint32_t lastPrint = 0;

    if (millis() - lastPrint > 500) {
      lastPrint = millis();

      Serial.print("Cannon pos=");
      Serial.print(pos);
      Serial.print("ms remaining=");
      Serial.println(rem);
    }
  }
}