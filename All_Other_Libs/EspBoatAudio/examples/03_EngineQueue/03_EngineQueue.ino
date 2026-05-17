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

EspBoatAudio::AudioHandle engineHandle;

bool started = false;
uint32_t lastPitch = 0;
float pitch = 1.0f;

void pumpAudio()
{
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.chainTick();
}

void startEngineQueue()
{
  EspBoatAudio::QueueItem q[2];

  q[0] = {
    "/ENGINES/VAPEUR_STA.wav",
    false,
    1,
    0.7f,
    1.0f,
    255
  };

  q[1] = {
    "@PRELOADED_ENGINE",
    true,
    0,
    0.7f,
    1.0f,
    255
  };

  engineHandle = audio.playVoiceQueue(EspBoatAudio::VOICE_ENGINE, q, 2);

  Serial.println("QUEUE START -> PRELOADED ENGINE");
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

  startEngineQueue();
  started = true;
}

void loop()
{
  pumpAudio();

  if (started && millis() - lastPitch >= 100) {
    lastPitch = millis();

    pitch += 0.01f;

    if (pitch > 1.30f) {
      pitch = 0.75f;
    }

    audio.engineSetPitch(pitch);
  }
}