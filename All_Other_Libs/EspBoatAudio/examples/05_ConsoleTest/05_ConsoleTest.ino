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

EspBoatAudio::AudioHandle engineHandle;
EspBoatAudio::AudioHandle ambientHandle;
EspBoatAudio::AudioHandle fxHandle;

String line;

void pumpAudio()
{
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.streamTick();
  audio.chainTick();
}

void printHelp()
{
  Serial.println();
  Serial.println("COMMANDES:");
  Serial.println("  HELP");
  Serial.println("  FX");
  Serial.println("  AMBIENT");
  Serial.println("  AMBIENTSTOP");
  Serial.println("  ENGINE");
  Serial.println("  ENGINESTOP");
  Serial.println("  PITCH 0.75..1.30");
  Serial.println("  VOL 0..100");
  Serial.println("  STOPALL");
  Serial.println();
}

void startEngine()
{
  if (!audio.preloadEngineLoop("/ENGINES/VAPEUR_IDL.wav", 0.7f, 1.0f, 255)) {
    Serial.println("ENGINE PRELOAD ERROR");
    return;
  }

  engineHandle = audio.playPreloadedEngineLoop();
  Serial.println("ENGINE STARTED");
}

void processCommand(String cmd)
{
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "HELP") {
    printHelp();
  }

  else if (cmd == "FX") {
    fxHandle = audio.playFxAuto("/FIXEDSOUND/CANNON.wav", 0.9f, 220, 1.0f);
    Serial.println("FX PLAY");
  }

  else if (cmd == "AMBIENT") {
    ambientHandle = audio.playVoiceStream(
                      EspBoatAudio::VOICE_AMBIENT,
                      "/USERSOUND/9_SEAGULL.wav",
                      true,
                      0.4f,
                      1.0f,
                      50
                    );

    Serial.println("AMBIENT STREAM START");
  }

  else if (cmd == "AMBIENTSTOP") {
    audio.stop(ambientHandle);
    Serial.println("AMBIENT STOP");
  }

  else if (cmd == "ENGINE") {
    startEngine();
  }

  else if (cmd == "ENGINESTOP") {
    audio.engineStop();
    Serial.println("ENGINE STOP");
  }

  else if (cmd.startsWith("PITCH ")) {
    float p = cmd.substring(6).toFloat();
    p = constrain(p, 0.5f, 2.0f);

    audio.engineSetPitch(p);

    Serial.print("ENGINE PITCH=");
    Serial.println(p);
  }

  else if (cmd.startsWith("VOL ")) {
    float v = cmd.substring(4).toFloat();
    v = constrain(v, 0.0f, 100.0f);

    audio.setMasterVolume(v / 100.0f);

    Serial.print("MASTER VOL=");
    Serial.println(v);
  }

  else if (cmd == "STOPALL") {
    audio.stopAllFx();
    audio.engineStop();
    audio.stop(ambientHandle);

    Serial.println("STOP ALL");
  }

  else {
    Serial.println("UNKNOWN COMMAND");
  }
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

  Serial.println("SD OK");

  if (!audio.begin(I2S_BCLK, I2S_LRCK, I2S_DOUT, 44100, I2S_NUM_0, 0)) {
    Serial.println("AUDIO ERROR");
    return;
  }

  Serial.println("AUDIO OK");

  audio.setMasterVolume(0.8f);

  printHelp();
}

void loop()
{
  pumpAudio();

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (line.length() > 0) {
        processCommand(line);
        line = "";
      }
    } else {
      line += c;
    }
  }
}