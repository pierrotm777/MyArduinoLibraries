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

EspBoatAudio::AudioHandle lastHandle;
EspBoatAudio::AudioHandle repeatHandle;

String inputLine;

float currentPitch = 1.0f;
float currentVolume = 0.8f;

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
  Serial.println(F("===== EspBoatAudio simple test ====="));
  Serial.println(F("FX       : joue CANNON avec playFxAuto"));
  Serial.println(F("FXS      : joue SEAGULL avec playFxAuto"));
  Serial.println(F("REPEAT   : joue MG en repetition infinie"));
  Serial.println(F("REPEAT3  : joue HORN 3 fois"));
  Serial.println(F("STOP     : stop le dernier son lance"));
  Serial.println(F("STOPR    : stop le repeat"));
  Serial.println(F("STOPALL  : stop tous les FX"));
  Serial.println(F("VOL 80   : volume master en pourcent"));
  Serial.println(F("PITCH 1.5: pitch utilise pour les prochains sons"));
  Serial.println(F("POS      : affiche position du dernier son"));
  Serial.println(F("HELP     : affiche cette aide"));
  Serial.println();
}

void playCannon()
{
  Serial.println(F("PLAY CANNON"));

  // ---------------------------------------------------------------------------
  // playFxAuto(path, volume, priority, pitch)
  // ---------------------------------------------------------------------------
  // path     : chemin du fichier WAV
  // volume   : 0.0f a 1.0f
  // priority : priorite FX, plus grand = plus prioritaire
  // pitch    : 1.0f normal, 0.5f plus lent/grave, 2.0f plus rapide/aigu
  // ---------------------------------------------------------------------------

  lastHandle = audio.playFxAuto(
                 "/FIXEDSOUND/CANNON.wav",
                 currentVolume,
                 200,
                 currentPitch
               );
}

void playSeagull()
{
  Serial.println(F("PLAY SEAGULL"));

  lastHandle = audio.playFxAuto(
                 "/SEAGULL.wav",
                 0.35f,
                 120,
                 currentPitch
               );
}

void playRepeatInfinite()
{
  Serial.println(F("PLAY MG REPEAT INFINITE"));

  // ---------------------------------------------------------------------------
  // playVoiceRepeat(voice, path, playCount, volume, pitch, priority)
  // ---------------------------------------------------------------------------
  // voice     : slot utilise
  // path      : chemin WAV
  // playCount : 0 = infini, 1 = une fois, 3 = trois fois
  // volume    : volume
  // pitch     : vitesse / tonalite
  // priority  : priorite
  // ---------------------------------------------------------------------------

  repeatHandle = audio.playVoiceRepeat(
                   EspBoatAudio::VOICE_FX1,
                   "/FIXEDSOUND/MG.wav",
                   0,
                   0.55f,
                   currentPitch,
                   180
                 );
}

void playRepeat3()
{
  Serial.println(F("PLAY HORN x3"));

  repeatHandle = audio.playVoiceRepeat(
                   EspBoatAudio::VOICE_FX2,
                   "/FIXEDSOUND/HORN.wav",
                   3,
                   0.70f,
                   currentPitch,
                   160
                 );
}

void stopLast()
{
  Serial.println(F("STOP LAST HANDLE"));
  audio.stop(lastHandle);
}

void stopRepeat()
{
  Serial.println(F("STOP REPEAT HANDLE"));
  audio.stop(repeatHandle);
}

void printPosition()
{
  if (!audio.isPlaying(lastHandle)) {
    Serial.println(F("Last sound not playing"));
    return;
  }

  Serial.print(F("Position = "));
  Serial.print(audio.positionMillis(lastHandle));
  Serial.print(F(" ms / "));

  Serial.print(F("Length = "));
  Serial.print(audio.lengthMillis(lastHandle));
  Serial.print(F(" ms / "));

  Serial.print(F("Remaining = "));
  Serial.print(audio.remainingMillis(lastHandle));
  Serial.println(F(" ms"));

  if (audio.inWindow(lastHandle, 100, 250)) {
    Serial.println(F("In window 100-250 ms"));
  }
}

void processCommand(String cmd)
{
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "HELP") {
    printHelp();
  }
  else if (cmd == "FX") {
    playCannon();
  }
  else if (cmd == "FXS") {
    playSeagull();
  }
  else if (cmd == "REPEAT") {
    playRepeatInfinite();
  }
  else if (cmd == "REPEAT3") {
    playRepeat3();
  }
  else if (cmd == "STOP") {
    stopLast();
  }
  else if (cmd == "STOPR") {
    stopRepeat();
  }
  else if (cmd == "STOPALL") {
    Serial.println(F("STOP ALL FX"));
    audio.stopAllFx();
  }
  else if (cmd.startsWith("VOL ")) {
    float v = cmd.substring(4).toFloat();
    v = constrain(v, 0.0f, 100.0f);

    audio.setMasterVolume(v / 100.0f);

    Serial.print(F("Master volume = "));
    Serial.print(v);
    Serial.println(F("%"));
  }
  else if (cmd.startsWith("PITCH ")) {
    currentPitch = cmd.substring(6).toFloat();
    currentPitch = constrain(currentPitch, 0.25f, 3.0f);

    Serial.print(F("Pitch = "));
    Serial.println(currentPitch);
  }
  else if (cmd == "POS") {
    printPosition();
  }
  else {
    Serial.println(F("Commande inconnue. Tape HELP."));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(F("=== EspBoatAudio multi possibilities ==="));

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(25), &sdSPI))) {
    Serial.println(F("SD ERROR"));
    while (1) delay(100);
  }

  Serial.println(F("SD OK"));

  if (!audio.begin(I2S_BCLK, I2S_LRCK, I2S_DOUT, 44100, I2S_NUM_0, 0)) {
    Serial.println(F("AUDIO ERROR"));
    while (1) delay(100);
  }

  Serial.println(F("AUDIO OK"));

  audio.setMasterVolume(0.7f);

  printHelp();
}

void loop()
{
  pumpAudio();

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        processCommand(inputLine);
        inputLine = "";
      }
    } else {
      inputLine += c;
    }
  }

  static uint32_t lastAutoPos = 0;

  if (audio.isPlaying(lastHandle) && millis() - lastAutoPos >= 500) {
    lastAutoPos = millis();

    Serial.print(F("Last pos = "));
    Serial.print(audio.positionMillis(lastHandle));
    Serial.println(F(" ms"));
  }
}