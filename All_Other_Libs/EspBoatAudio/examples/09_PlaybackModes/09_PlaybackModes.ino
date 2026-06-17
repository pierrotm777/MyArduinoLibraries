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

EspBoatAudio::AudioHandle lastHandle;

void pumpAudio()
{
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.chainTick();
}

void printMemory(const char* label)
{
  Serial.printf("[%s] Heap=%lu KB | PSRAM=%lu KB\r\n",
                label,
                ESP.getFreeHeap() / 1024,
                ESP.getFreePsram() / 1024);
}

void printMenu()
{
  Serial.println();
  Serial.println(F("─────── EspBoatAudio Playback Modes ───────"));
  Serial.println(F("1 = RAM direct playVoice"));
  Serial.println(F("2 = STREAM direct playVoiceStream"));
  Serial.println(F("3 = AUTO playFxAuto"));
  Serial.println(F("4 = RAM repeat playVoiceRepeat x3"));
  Serial.println(F("5 = STREAM loop playVoiceStreamRepeat"));
  Serial.println(F("6 = PRELOADED FX RAM playVoice cached"));
  Serial.println(F("7 = PRELOADED ENGINE loop"));
  Serial.println(F("8 = QUEUE START -> PRELOADED ENGINE"));
  Serial.println(F("9 = AUDIO status"));
  Serial.println(F("0 = STOP all"));
  Serial.println(F("p = Print memory"));
  Serial.println(F("-------------------------------------------"));
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("EspBoatAudio - 09_PlaybackModes"));

  printMemory("BOOT");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(25), &sdSPI)))
  {
    Serial.println(F("SD init FAILED"));
    while (true) delay(500);
  }

  Serial.println(F("SD OK"));

  if (!audio.begin(I2S_BCLK, I2S_LRCK, I2S_DOUT, 44100, I2S_NUM_0, 0))
  {
    Serial.println(F("Audio begin FAILED"));
    while (true) delay(500);
  }

  Serial.println(F("Audio OK"));

  audio.setMasterVolume(1.0f);

  printMemory("BEFORE PRELOAD");

  Serial.println(F("Preload FX cache: /FIXEDSOUND/SHORT_1.wav"));
  bool fxOk = audio.preloadFxCache("/FIXEDSOUND/SHORT_1.wav");
  Serial.printf("FX preload: %s\r\n", fxOk ? "OK" : "FAIL");

  Serial.println(F("Preload engine loop: /ENGINES/DSL-TURB_IDL.wav"));
  bool engOk = audio.preloadEngineLoop(
                 "/ENGINES/DSL-TURB_IDL.wav",
                 0.35f,
                 1.0f,
                 255
               );
  Serial.printf("ENGINE preload: %s\r\n", engOk ? "OK" : "FAIL");

  printMemory("AFTER PRELOAD");

  printMenu();
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
        Serial.println(F("[1] RAM direct: CANNON"));
        lastHandle = audio.playVoice(
                       EspBoatAudio::VOICE_FX0,
                       "/FIXEDSOUND/CANNON.wav",
                       false,
                       1.0f,
                       1.0f,
                       10
                     );
        break;

      case '2':
        Serial.println(F("[2] STREAM direct: AMBIENT once"));
        lastHandle = audio.playVoiceStream(
                       EspBoatAudio::VOICE_FX0,
                       "/FIXEDSOUND/AMBIENT.wav",
                       false,
                       0.8f,
                       1.0f,
                       10
                     );
        break;

      case '3':
        Serial.println(F("[3] AUTO: LONG_1"));
        lastHandle = audio.playFxAuto(
                       "/FIXEDSOUND/LONG_1.wav",
                       1.0f,
                       10,
                       1.0f
                     );
        break;

      case '4':
        Serial.println(F("[4] RAM repeat x3: SHORT_1"));
        lastHandle = audio.playVoiceRepeat(
                       EspBoatAudio::VOICE_FX0,
                       "/FIXEDSOUND/SHORT_1.wav",
                       3,
                       1.0f,
                       1.0f,
                       10
                     );
        break;

      case '5':
        Serial.println(F("[5] STREAM repeat infinite: AMBIENT"));
        lastHandle = audio.playVoiceStreamRepeat(
                       EspBoatAudio::VOICE_FX0,
                       "/FIXEDSOUND/AMBIENT.wav",
                       0,
                       0.6f,
                       1.0f,
                       10
                     );
        break;

      case '6':
        Serial.println(F("[6] PRELOADED FX RAM: SHORT_1"));
        lastHandle = audio.playVoice(
                       EspBoatAudio::VOICE_FX0,
                       "/FIXEDSOUND/SHORT_1.wav",
                       false,
                       1.0f,
                       1.0f,
                       10
                     );
        break;

      case '7':
        Serial.println(F("[7] PRELOADED ENGINE loop"));
        lastHandle = audio.playPreloadedEngineLoop(false);
        audio.engineSetVolume(0.35f);
        audio.engineSetPitch(1.0f);
        break;

      case '8':
      {
        Serial.println(F("[8] QUEUE START -> PRELOADED ENGINE"));

        EspBoatAudio::QueueItem q[2];

        q[0].path = "/ENGINES/DSL-TURB_STA.wav";
        q[0].loop = false;
        q[0].repeatCount = 1;
        q[0].volume = 1.0f;
        q[0].pitch = 1.0f;
        q[0].priority = 255;

        q[1].path = "@PRELOADED_ENGINE";
        q[1].loop = true;
        q[1].repeatCount = 0;
        q[1].volume = 0.35f;
        q[1].pitch = 1.0f;
        q[1].priority = 255;

        lastHandle = audio.playVoiceQueue(
                       EspBoatAudio::VOICE_ENGINE,
                       q,
                       2
                     );
        break;
      }

      case '9':
        audio.printVoicesStatus();
        break;

      case '0':
        Serial.println(F("[0] STOP all"));
        audio.stopAllFx();
        audio.engineStop();
        audio.ambientStop();
        break;

      case 'p':
      case 'P':
        printMemory("NOW");
        break;

      case 'm':
      case 'M':
        printMenu();
        break;
    }
  }

  pumpAudio();
}