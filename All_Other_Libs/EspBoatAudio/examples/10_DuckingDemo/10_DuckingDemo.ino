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

EspBoatAudio::AudioHandle ambientHandle;
EspBoatAudio::AudioHandle fxHandle;

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
  Serial.println(F("─────── EspBoatAudio Ducking Demo ───────"));
  Serial.println(F("1 = Start AMBIENT stream loop"));
  Serial.println(F("2 = Duck AMBIENT to 25%"));
  Serial.println(F("3 = Unduck AMBIENT"));
  Serial.println(F("4 = Play CANNON + duck AMBIENT"));
  Serial.println(F("5 = Play HORN + duck AMBIENT"));
  Serial.println(F("6 = Fade AMBIENT out"));
  Serial.println(F("7 = Fade AMBIENT back"));
  Serial.println(F("8 = Stop AMBIENT"));
  Serial.println(F("9 = AUDIO status"));
  Serial.println(F("0 = STOP all"));
  Serial.println(F("-----------------------------------------"));
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("EspBoatAudio - 10_DuckingDemo"));

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

  Serial.println(F("Preload CANNON for fast RAM playback"));
  bool cannonOk = audio.preloadFxCache("/FIXEDSOUND/CANNON.wav");
  Serial.printf("CANNON preload: %s\r\n", cannonOk ? "OK" : "FAIL");

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
        Serial.println(F("[1] Start AMBIENT stream loop"));
        ambientHandle = audio.playVoiceStreamRepeat(
                          EspBoatAudio::VOICE_AMBIENT,
                          "/FIXEDSOUND/AMBIENT.wav",
                          0,
                          0.40f,
                          1.0f,
                          2
                        );
        break;

      case '2':
        Serial.println(F("[2] Duck AMBIENT to 25% in 300 ms"));
        audio.duckVoice(
          EspBoatAudio::VOICE_AMBIENT,
          0.25f,
          300
        );
        break;

      case '3':
        Serial.println(F("[3] Unduck AMBIENT in 800 ms"));
        audio.unduckVoice(
          EspBoatAudio::VOICE_AMBIENT,
          800
        );
        break;

      case '4':
        Serial.println(F("[4] CANNON + AMBIENT duck"));
        audio.duckVoice(
          EspBoatAudio::VOICE_AMBIENT,
          0.20f,
          100
        );

        fxHandle = audio.playFxAuto(
                     "/FIXEDSOUND/CANNON.wav",
                     1.0f,
                     20,
                     1.0f
                   );
        break;

      case '5':
        Serial.println(F("[5] HORN + AMBIENT duck"));
        audio.duckVoice(
          EspBoatAudio::VOICE_AMBIENT,
          0.25f,
          100
        );

        fxHandle = audio.playFxAuto(
                     "/FIXEDSOUND/HORN_START.wav",
                     1.0f,
                     10,
                     1.0f
                   );
        break;

      case '6':
        Serial.println(F("[6] Fade AMBIENT out in 1000 ms"));
        audio.fadeVoice(
          EspBoatAudio::VOICE_AMBIENT,
          0.0f,
          1000
        );
        break;

      case '7':
        Serial.println(F("[7] Fade AMBIENT back to 40% in 1000 ms"));
        audio.fadeVoice(
          EspBoatAudio::VOICE_AMBIENT,
          0.40f,
          1000
        );
        break;

      case '8':
        Serial.println(F("[8] Stop AMBIENT"));
        audio.ambientStop();
        break;

      case '9':
        audio.printVoicesStatus();
        break;

      case '0':
        Serial.println(F("[0] STOP all"));
        audio.stopAllFx();
        audio.engineStop();
        audio.ambientStop();
        break;

      case 'm':
      case 'M':
        printMenu();
        break;
    }
  }

  if (fxHandle.valid() && !audio.isPlaying(fxHandle))
  {
    fxHandle = EspBoatAudio::AudioHandle{};

    if (audio.isPlaying(EspBoatAudio::VOICE_AMBIENT))
    {
      Serial.println(F("[AUTO] FX finished -> unduck AMBIENT"));
      audio.unduckVoice(
        EspBoatAudio::VOICE_AMBIENT,
        800
      );
    }
  }

  pumpAudio();
}