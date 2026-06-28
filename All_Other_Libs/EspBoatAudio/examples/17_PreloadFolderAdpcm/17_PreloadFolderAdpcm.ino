#include <EspBoatAudio.h>
#include <SdFat.h>

EspBoatAudio audio;
SdFat SD;

void setup()
{
  Serial.begin(115200);

  audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_8MB);

  Serial.println();
  Serial.println("=== PRELOAD ADPCM ===");

  uint8_t n =
    audio.preloadFolderAdpcmOnly("/FIXEDSOUND");

  Serial.printf("Charges : %u sons\r\n", n);

  audio.printFxCacheStatus();
}

void loop()
{
}