#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <EspBoatAudio.h>

#define SD_CS    13
#define SD_SCK   11
#define SD_MISO  10
#define SD_MOSI  12

#define I2S_BCLK 8
#define I2S_LRCK 9
#define I2S_DOUT 6

SPIClass sdSPI(FSPI);
SdFat sd;
EspBoatAudio audio;

uint32_t lastAction = 0;
uint8_t stepDemo = 0;

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

  // Premier moteur
  audio.enginePlayLoop("/ENGINES/VAPEUR_IDL.wav");
  audio.engineSetVolume(0.7f);

  // Première ambiance
  audio.ambientPlayLoop("/FIXEDSOUND/AMBIENT.wav");
  audio.ambientSetVolume(0.5f);

  Serial.println("START ENGINE + AMBIENT");
}

void loop()
{
  pumpAudio();

  if (millis() - lastAction < 8000) {
    return;
  }

  lastAction = millis();

  switch (stepDemo)
  {
    case 0:
      Serial.println("ENGINE CROSSFADE -> MED");
      audio.engineCrossfade(
        "/ENGINES/VAPEUR_MED.wav",
        2000,
        0.7f,
        1.0f,
        255,
        false   // false = RAM/preload style
      );
      break;

    case 1:
      Serial.println("AMBIENT CROSSFADE -> STORM");
      audio.ambientCrossfade(
        "/FIXEDSOUND/AMBIENT_STORM.wav",
        3000,
        0.5f,
        1.0f,
        200,
        true    // true = streaming SD
      );
      break;

    case 2:
      Serial.println("ENGINE CROSSFADE -> HIGH");
      audio.engineCrossfade(
        "/ENGINES/VAPEUR_HIGH.wav",
        2000,
        0.7f,
        1.0f,
        255,
        false
      );
      break;

    case 3:
      Serial.println("AMBIENT CROSSFADE -> NORMAL");
      audio.ambientCrossfade(
        "/FIXEDSOUND/AMBIENT.wav",
        3000,
        0.5f,
        1.0f,
        200,
        true
      );
      break;

    case 4:
      Serial.println("DEBUG VOICES");
      audio.printVoicesStatus();
      break;
  }

  stepDemo++;
  if (stepDemo > 4) {
    stepDemo = 0;
  }
}