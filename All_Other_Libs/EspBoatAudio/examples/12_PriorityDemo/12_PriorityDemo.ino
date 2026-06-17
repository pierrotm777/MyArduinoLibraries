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

void pumpAudio()
{
  for (uint8_t i = 0; i < 4; i++)
  {
    audio.streamTick();
  }

  audio.chainTick();
}

void printMenu()
{
  Serial.println();
  Serial.println(F("─────── EspBoatAudio Priority Demo ───────"));
  Serial.println(F("1 = Fill FX slots with LOW priority sounds"));
  Serial.println(F("2 = Play MEDIUM priority FX"));
  Serial.println(F("3 = Play HIGH priority FX"));
  Serial.println(F("4 = Play VERY HIGH priority FX"));
  Serial.println(F("5 = Stop all FX"));
  Serial.println(F("9 = AUDIO status"));
  Serial.println(F("m = Menu"));
  Serial.println(F("------------------------------------------"));
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("EspBoatAudio - 12_PriorityDemo"));

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
        Serial.println(F("[1] Fill FX0..FX3 with low priority loops"));

        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_FX0,
          "/FIXEDSOUND/AMBIENT.wav",
          0,
          0.25f,
          1.0f,
          1
        );

        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_FX1,
          "/FIXEDSOUND/LONG_1.wav",
          0,
          0.35f,
          1.0f,
          2
        );

        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_FX2,
          "/FIXEDSOUND/LONG_2.wav",
          0,
          0.35f,
          1.0f,
          3
        );

        audio.playVoiceStreamRepeat(
          EspBoatAudio::VOICE_FX3,
          "/FIXEDSOUND/LONG_3.wav",
          0,
          0.35f,
          1.0f,
          4
        );
        break;

      case '2':
        Serial.println(F("[2] MEDIUM priority FX, priority=10"));
        audio.playFxAuto(
          "/FIXEDSOUND/HORN_START.wav",
          1.0f,
          10,
          1.0f
        );
        break;

      case '3':
        Serial.println(F("[3] HIGH priority FX, priority=50"));
        audio.playFxAuto(
          "/FIXEDSOUND/CANNON.wav",
          1.0f,
          50,
          1.0f
        );
        break;

      case '4':
        Serial.println(F("[4] VERY HIGH priority FX, priority=100"));
        audio.playFxAuto(
          "/FIXEDSOUND/FAILSAFE.wav",
          1.0f,
          100,
          1.0f
        );
        break;

      case '5':
        Serial.println(F("[5] Stop all FX"));
        audio.stopAllFx();
        break;

      case '9':
        audio.printVoicesStatus();
        break;

      case 'm':
      case 'M':
        printMenu();
        break;
    }
  }

  pumpAudio();
}