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

float enginePitch = 1.0f;
float engineVolume = 0.35f;

void pumpAudio()
{
  for (uint8_t i = 0; i < 6; i++)
  {
    audio.streamTick();
  }

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
  Serial.println(F("─────── EspBoatAudio Stream Stress ───────"));
  Serial.println(F("1 = Preload + start ENGINE"));
  Serial.println(F("2 = Start AMBIENT stream loop"));
  Serial.println(F("3 = Start LONG_1 stream loop on FX0"));
  Serial.println(F("4 = Start LONG_2 stream loop on FX1"));
  Serial.println(F("5 = Play CANNON auto"));
  Serial.println(F("6 = Play HORN auto"));
  Serial.println(F("7 = Play FAILSAFE stream repeat"));
  Serial.println(F("8 = Pitch engine +"));
  Serial.println(F("9 = AUDIO status"));
  Serial.println(F("0 = Stop all"));
  Serial.println(F("p = Print memory"));
  Serial.println(F("m = Menu"));
  Serial.println(F("------------------------------------------"));
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("EspBoatAudio - 14_StreamStress"));

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

  printMemory("BOOT");
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
      {
        Serial.println(F("[1] Preload + start ENGINE"));
        printMemory("BEFORE ENGINE PRELOAD");

        bool ok = audio.preloadEngineLoop(
                    "/ENGINES/DSL-TURB_IDL.wav",
                    engineVolume,
                    enginePitch,
                    255
                  );

        Serial.printf("preloadEngineLoop: %s\r\n", ok ? "OK" : "FAIL");
        printMemory("AFTER ENGINE PRELOAD");

        audio.playPreloadedEngineLoop(false);
        audio.engineSetVolume(engineVolume);
        audio.engineSetPitch(enginePitch);
        break;
      }

      case '2':
        Serial.println(F("[2] Start AMBIENT stream loop"));
        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_AMBIENT,
          "/FIXEDSOUND/AMBIENT.wav",
          0,
          0.30f,
          1.0f,
          2
        );
        break;

      case '3':
        Serial.println(F("[3] Start LONG_1 stream loop on FX0"));
        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_FX0,
          "/FIXEDSOUND/LONG_1.wav",
          0,
          0.50f,
          1.0f,
          5
        );
        break;

      case '4':
        Serial.println(F("[4] Start LONG_2 stream loop on FX1"));
        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_FX1,
          "/FIXEDSOUND/LONG_2.wav",
          0,
          0.50f,
          1.0f,
          5
        );
        break;

      case '5':
        Serial.println(F("[5] Play CANNON auto priority 50"));
        audio.playFxAuto(
          "/FIXEDSOUND/CANNON.wav",
          1.0f,
          50,
          1.0f
        );
        break;

      case '6':
        Serial.println(F("[6] Play HORN auto priority 20"));
        audio.playFxAuto(
          "/FIXEDSOUND/HORN_START.wav",
          1.0f,
          20,
          1.0f
        );
        break;

      case '7':
        Serial.println(F("[7] Play FAILSAFE stream repeat priority 100"));
        audio.playFxRepeatStream(
          "/FIXEDSOUND/FAILSAFE.wav",
          0,
          1.0f,
          100,
          1.0f
        );
        break;

      case '8':
        enginePitch += 0.10f;

        if (enginePitch > 2.20f)
        {
          enginePitch = 1.0f;
        }

        audio.engineSetPitch(enginePitch);
        Serial.printf("Engine pitch = %.2f\r\n", enginePitch);
        break;

      case '9':
        audio.printVoicesStatus();
        break;

      case '0':
        Serial.println(F("[0] Stop all"));
        audio.stopAllFx();
        audio.ambientStop();
        audio.engineStop();
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