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

uint32_t lastAction = 0;
uint8_t state = 0;

static void pumpAudio()
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

  Serial.println("=== REPEAT TEST ===");

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!sd.begin(SdSpiConfig(SD_CS,
                            DEDICATED_SPI,
                            SD_SCK_MHZ(20),
                            &sdSPI))) {
    Serial.println("SD ERROR");
    while (1) delay(100);
  }

  if (!audio.begin(I2S_BCLK,
                   I2S_LRCK,
                   I2S_DOUT,
                   44100,
                   I2S_NUM_0,
                   1)) {
    Serial.println("AUDIO ERROR");
    while (1) delay(100);
  }

  audio.setMasterVolume(0.8f);

  Serial.println("READY");
}

void loop()
{
  pumpAudio();

  uint32_t now = millis();

  if (now - lastAction < 8000) {
    return;
  }

  lastAction = now;

  switch (state)
  {
    // -------------------------------------------------------------------------
    // Repeat 3 fois
    // -------------------------------------------------------------------------
    case 0:
    {
      Serial.println("PLAY HORN x3");

      audio.playVoiceRepeat(
        EspBoatAudio::VOICE_FX0,
        "/HORN.wav",
        3,
        0.75f,
        1.0f,
        220
      );

      break;
    }

    // -------------------------------------------------------------------------
    // Repeat infini
    // -------------------------------------------------------------------------
    case 1:
    {
      Serial.println("PLAY MG LOOP");

      audio.playVoiceRepeat(
        EspBoatAudio::VOICE_FX1,
        "/MG.wav",
        0,
        0.45f,
        1.0f,
        180
      );

      break;
    }

    // -------------------------------------------------------------------------
    // Stop MG
    // -------------------------------------------------------------------------
    case 2:
    {
      Serial.println("STOP MG");

      audio.stopVoice(EspBoatAudio::VOICE_FX1);

      break;
    }

    // -------------------------------------------------------------------------
    // Repeat ambiance longue
    // -------------------------------------------------------------------------
    case 3:
    {
      Serial.println("AMBIENT LOOP");

      audio.playVoiceRepeat(
        EspBoatAudio::VOICE_AMBIENT,
        "/AMBIENT.wav",
        0,
        0.25f,
        1.0f,
        100
      );

      break;
    }

    // -------------------------------------------------------------------------
    // Stop ambiance
    // -------------------------------------------------------------------------
    case 4:
    {
      Serial.println("STOP AMBIENT");

      audio.stopVoice(EspBoatAudio::VOICE_AMBIENT);

      break;
    }
  }

  state++;

  if (state > 4) {
    state = 0;
  }

  yield();
}