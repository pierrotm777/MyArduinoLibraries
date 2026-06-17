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
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.chainTick();
}

void printMenu()
{
  Serial.println();
  Serial.println(F("─────── Engine Generic Stop Demo ───────"));
  Serial.println(F("1 = START -> PRELOADED IDLE queue"));
  Serial.println(F("2 = Play PRELOADED IDLE directly"));
  Serial.println(F("3 = Pitch +"));
  Serial.println(F("4 = Pitch -"));
  Serial.println(F("5 = Volume +"));
  Serial.println(F("6 = Volume -"));
  Serial.println(F("7 = Generic stop 1800 ms"));
  Serial.println(F("8 = Generic stop 3000 ms"));
  Serial.println(F("9 = AUDIO status"));
  Serial.println(F("0 = Hard stop engine"));
  Serial.println(F("m = Menu"));
  Serial.println(F("---------------------------------------"));
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("EspBoatAudio - 11_EngineGenericStop"));

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

  Serial.printf("PSRAM before engine preload: %lu KB\r\n", ESP.getFreePsram() / 1024);

  bool ok = audio.preloadEngineLoop(
              "/ENGINES/DSL-TURB_IDL.wav",
              engineVolume,
              enginePitch,
              255
            );

  Serial.printf("preloadEngineLoop: %s\r\n", ok ? "OK" : "FAIL");
  Serial.printf("PSRAM after engine preload : %lu KB\r\n", ESP.getFreePsram() / 1024);

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
        Serial.println(F("[1] START -> PRELOADED IDLE queue"));

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
        q[1].volume = engineVolume;
        q[1].pitch = enginePitch;
        q[1].priority = 255;

        audio.playVoiceQueue(
          EspBoatAudio::VOICE_ENGINE,
          q,
          2
        );
        break;
      }

      case '2':
        Serial.println(F("[2] Play PRELOADED IDLE directly"));
        audio.playPreloadedEngineLoop(false);
        audio.engineSetVolume(engineVolume);
        audio.engineSetPitch(enginePitch);
        break;

      case '3':
        enginePitch += 0.10f;
        if (enginePitch > 2.50f) enginePitch = 2.50f;
        audio.engineSetPitch(enginePitch);
        Serial.printf("Engine pitch = %.2f\r\n", enginePitch);
        break;

      case '4':
        enginePitch -= 0.10f;
        if (enginePitch < 0.50f) enginePitch = 0.50f;
        audio.engineSetPitch(enginePitch);
        Serial.printf("Engine pitch = %.2f\r\n", enginePitch);
        break;

      case '5':
        engineVolume += 0.05f;
        if (engineVolume > 1.00f) engineVolume = 1.00f;
        audio.engineSetVolume(engineVolume);
        Serial.printf("Engine volume = %.2f\r\n", engineVolume);
        break;

      case '6':
        engineVolume -= 0.05f;
        if (engineVolume < 0.00f) engineVolume = 0.00f;
        audio.engineSetVolume(engineVolume);
        Serial.printf("Engine volume = %.2f\r\n", engineVolume);
        break;

      case '7':
        Serial.println(F("[7] Generic stop 1800 ms, target pitch 0.70"));
        audio.engineGenericStop(1800, 0.70f);
        break;

      case '8':
        Serial.println(F("[8] Generic stop 3000 ms, target pitch 0.50"));
        audio.engineGenericStop(3000, 0.50f);
        break;

      case '9':
        audio.printVoicesStatus();
        break;

      case '0':
        Serial.println(F("[0] Hard stop engine"));
        audio.engineStop();
        break;

      case 'm':
      case 'M':
        printMenu();
        break;
    }
  }

  pumpAudio();
}