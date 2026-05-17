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

float pitch = 1.0f;
float dir   = 0.03f;

uint32_t lastPitch = 0;

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

  Serial.println();
  Serial.println("=== AIRSTART -> START -> IDLE ===");

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

  if (!audio.preloadEngineLoop("/ENGINES/DSL-TURB_IDL.wav",
                               0.55f,
                               1.0f,
                               255)) {
    Serial.println("PRELOAD FAIL");
    while (1) delay(100);
  }

  Serial.println("PRELOAD OK");

  EspBoatAudio::QueueItem engineSeq[] =
  {
    {"/DEFAULT/AIRSTART.wav", false, 1, 0.60f, 1.0f, 255},
    {"/ENGINES/DSL-TURB_STA.wav",    false, 1, 0.60f, 1.0f, 255},
    {"@PRELOADED_ENGINE",            true,  0, 0.55f, 1.0f, 255}
  };

  engineHandle = audio.playVoiceQueue(
                   EspBoatAudio::VOICE_ENGINE,
                   engineSeq,
                   3
                 );

  Serial.print("QUEUE = ");
  Serial.println(engineHandle.valid() ? "OK" : "FAIL");

  if (!engineHandle.valid()) {
    while (1) delay(100);
  }

  Serial.println();
  Serial.println("Fallback system:");
  Serial.println("- If AIRSTART missing -> START");
  Serial.println("- If START missing -> IDLE directly");
}

void loop()
{
  pumpAudio();

  if (!audio.isVoiceQueueDone(EspBoatAudio::VOICE_ENGINE)) {
    return;
  }

  uint32_t now = millis();

  if (now - lastPitch < 300) {
    return;
  }

  lastPitch = now;

  pitch += dir;

  if (pitch >= 1.60f) {
    pitch = 1.60f;
    dir = -0.03f;
  }

  if (pitch <= 1.00f) {
    pitch = 1.00f;
    dir = 0.03f;
  }

  audio.engineSetPitch(pitch);

  float volume =
    0.40f +
    ((pitch - 1.0f) / (1.60f - 1.0f)) * 0.25f;

  audio.engineSetVolume(volume);

  Serial.print("Pitch=");
  Serial.print(pitch);
  Serial.print(" Vol=");
  Serial.println(volume);

  yield();
}