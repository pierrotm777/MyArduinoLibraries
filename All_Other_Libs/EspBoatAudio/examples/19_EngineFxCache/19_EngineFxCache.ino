#include <EspBoatAudio.h>
#include <SdFat.h>

EspBoatAudio audio;
SdFat SD;

void setup()
{
  Serial.begin(115200);

  audio.preloadEngineFxCache(
    "/ENGINES/BEIER/PENICHE/anlassgeraeusch.wav",
    true);

  audio.preloadEngineFxCache(
    "/ENGINES/BEIER/PENICHE/fahrgeraeusch.wav",
    true);

  audio.preloadEngineFxCache(
    "/ENGINES/BEIER/PENICHE/abstellgeraeusch.wav",
    true);

  audio.printEngineFxCacheStatus();
}

void loop()
{
}