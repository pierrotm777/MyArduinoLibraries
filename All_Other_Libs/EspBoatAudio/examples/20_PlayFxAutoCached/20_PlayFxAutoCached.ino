#include <EspBoatAudio.h>
#include <SdFat.h>

EspBoatAudio audio;
SdFat SD;

void setup()
{
  Serial.begin(115200);

  audio.preloadFxCache(
    "/FIXEDSOUND/CANNON.wav");

  audio.printFxCacheStatus();
}

void loop()
{
  if (Serial.available())
  {
    Serial.read();

    audio.playFxAutoCached(
      "/FIXEDSOUND/CANNON.wav",
      1.0f,
      50,
      1.0f);
  }

  audio.streamTick();
  audio.chainTick();
}