#include <Arduino.h>
#include <SdFat.h>
#include "EspBoatAudio.h"

// -------------------------
// PINS À ADAPTER SI BESOIN
// -------------------------
#define SD_CS    13
#define SD_SCK   11
#define SD_MISO  10
#define SD_MOSI  12

#define I2S_BCLK 8
#define I2S_LRCK 9
#define I2S_DOUT 6

SdFat sd;
EspBoatAudio audio;

// Fichiers de test
const char* TEST_FX      = "/FIXEDSOUND/FAILSAFE.wav";
const char* TEST_CANNON  = "/FIXEDSOUND/CANNON.wav";
const char* TEST_ENGINE  = "/ENGINES/DSL-TURB_IDL.wav";
const char* TEST_AMBIENT = "/FIXEDSOUND/AMBIENT.wav";

void pumpAudio()
{
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.chainTick();
}

void printMemory()
{
  Serial.println();
  Serial.println(F("─────── MEMORY ───────"));
  Serial.printf("PSRAM totale : %lu KB\n", ESP.getPsramSize() / 1024);
  Serial.printf("PSRAM libre  : %lu KB\n", ESP.getFreePsram() / 1024);
  Serial.printf("HEAP libre   : %lu KB\n", ESP.getFreeHeap() / 1024);
  Serial.println(F("──────────────────────"));
}

void stopAll()
{
  audio.engineStop();
  audio.ambientStop();
  audio.stopAllFx();

  audio.stopVoice(EspBoatAudio::VOICE_AMBIENT_A);
  audio.stopVoice(EspBoatAudio::VOICE_AMBIENT_B);
  audio.stopVoice(EspBoatAudio::VOICE_BRIDGE);
  audio.stopVoice(EspBoatAudio::VOICE_RAIN);
  audio.stopVoice(EspBoatAudio::VOICE_THUNDER);
  audio.stopVoice(EspBoatAudio::VOICE_RANDOM_A);
  audio.stopVoice(EspBoatAudio::VOICE_RANDOM_B);
  audio.stopVoice(EspBoatAudio::VOICE_ANCHOR);
  audio.stopVoice(EspBoatAudio::VOICE_ALARM);

  Serial.println(F("[STOP] Toutes les voix stoppées"));
}

void test1_streamDirect()
{
  stopAll();

  Serial.println();
  Serial.println(F("[TEST 1] FX STREAM direct"));
  Serial.println(TEST_FX);

  auto h = audio.playFxStream(TEST_FX, 1.0f, 200, 1.0f);

  Serial.println(h.valid() ? F("Résultat : OK") : F("Résultat : FAIL"));
  audio.printVoicesStatus();
}

void test2_fxAuto()
{
  stopAll();

  Serial.println();
  Serial.println(F("[TEST 2] FX AUTO RAM ou STREAM"));
  Serial.println(TEST_FX);

  auto h = audio.playFxAuto(TEST_FX, 1.0f, 200, 1.0f);

  Serial.println(h.valid() ? F("Résultat : OK") : F("Résultat : FAIL"));
  audio.printVoicesStatus();
}

void test3_cachePsram()
{
  stopAll();

  Serial.println();
  Serial.println(F("[TEST 3] PRELOAD CACHE puis PLAY"));
  Serial.println(TEST_CANNON);

  bool ok = audio.preloadFxCache(TEST_CANNON, true);

  Serial.println(ok ? F("Preload : OK") : F("Preload : FAIL"));

  auto h = audio.playFxAuto(TEST_CANNON, 1.0f, 220, 1.0f);

  Serial.println(h.valid() ? F("Play : OK") : F("Play : FAIL"));

  audio.printFxCacheStatus();
  audio.printVoicesStatus();
}

void test4_enginePreload()
{
  stopAll();

  Serial.println();
  Serial.println(F("[TEST 4] ENGINE PRELOAD + PLAY"));
  Serial.println(TEST_ENGINE);

  bool ok = audio.preloadEngineLoop(TEST_ENGINE, 1.0f, 1.0f, 255);

  Serial.println(ok ? F("Preload moteur : OK") : F("Preload moteur : FAIL"));

  if (ok)
  {
    auto h = audio.playPreloadedEngineLoop();
    Serial.println(h.valid() ? F("Play moteur : OK") : F("Play moteur : FAIL"));
  }

  audio.printVoicesStatus();
}

void test5_ambientStream()
{
  stopAll();

  Serial.println();
  Serial.println(F("[TEST 5] AMBIENT STREAM LOOP"));
  Serial.println(TEST_AMBIENT);

  auto h = audio.playVoiceStream(
    EspBoatAudio::VOICE_AMBIENT_A,
    TEST_AMBIENT,
    true,
    0.6f,
    1.0f,
    100
  );

  Serial.println(h.valid() ? F("Résultat : OK") : F("Résultat : FAIL"));
  audio.printVoicesStatus();
}

void test6_repeatStream()
{
  stopAll();

  Serial.println();
  Serial.println(F("[TEST 6] REPEAT STREAM infini"));
  Serial.println(TEST_FX);

  auto h = audio.playFxRepeatStream(TEST_FX, 0, 1.0f, 200, 1.0f);

  Serial.println(h.valid() ? F("Résultat : OK") : F("Résultat : FAIL"));
  audio.printVoicesStatus();
}

void printHelp()
{
  Serial.println();
  Serial.println(F("════════ AUDIO DIAG TEST ════════"));
  Serial.println(F("1 : FX stream direct"));
  Serial.println(F("2 : FX auto RAM/STREAM"));
  Serial.println(F("3 : preload cache PSRAM + play"));
  Serial.println(F("4 : moteur preload + play"));
  Serial.println(F("5 : ambient stream loop"));
  Serial.println(F("6 : repeat stream infini"));
  Serial.println(F("7 : print voices"));
  Serial.println(F("8 : print cache"));
  Serial.println(F("9 : print mémoire"));
  Serial.println(F("0 : stop tout"));
  Serial.println(F("h : aide"));
  Serial.println(F("═════════════════════════════════"));
}

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println(F("BOOT AUDIO DIAG"));

  printMemory();

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(10))))
  {
    Serial.println(F("[SD] FAIL"));
    while (true)
    {
      delay(1000);
    }
  }

  Serial.println(F("[SD] OK"));

  bool audioOk = audio.begin(
    I2S_BCLK,
    I2S_LRCK,
    I2S_DOUT,
    44100,
    I2S_NUM_0,
    0
  );

  Serial.println(audioOk ? F("[AUDIO] begin OK") : F("[AUDIO] begin FAIL"));

  audio.setMasterVolume(1.0f);

  if (ESP.getPsramSize() >= 6UL * 1024UL * 1024UL)
    audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_8MB);
  else
    audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_2MB);

  printHelp();
}

void loop()
{
  pumpAudio();

  if (Serial.available())
  {
    char c = Serial.read();

    switch (c)
    {
      case '1':
        test1_streamDirect();
        break;

      case '2':
        test2_fxAuto();
        break;

      case '3':
        test3_cachePsram();
        break;

      case '4':
        test4_enginePreload();
        break;

      case '5':
        test5_ambientStream();
        break;

      case '6':
        test6_repeatStream();
        break;

      case '7':
        audio.printVoicesStatus();
        break;

      case '8':
        audio.printFxCacheStatus();
        audio.printEngineFxCacheStatus();
        break;

      case '9':
        printMemory();
        break;

      case '0':
        stopAll();
        break;

      case 'h':
      case 'H':
        printHelp();
        break;
    }
  }
}
