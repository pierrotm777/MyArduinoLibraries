#include <EspBoatAudio.h>
#include <SdFat.h>

EspBoatAudio audio;
SdFat SD;

void setup()
{
  Serial.begin(115200);

  audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_8MB);

  Serial.println();
  Serial.println("=== PROFILE 8MB ===");

  audio.printFxCacheStatus();
}

void loop()
{
}