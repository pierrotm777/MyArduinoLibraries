#include <EspBoatAudio.h>
#include <SdFat.h>

EspBoatAudio audio;
SdFat SD;

void setup()
{
  Serial.begin(115200);

  audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_8MB);

  uint8_t n1 =
    audio.preloadFolderPrefixAdpcmOnly(
      "/FIXEDSOUND",
      "SHORT_"
    );

  uint8_t n2 =
    audio.preloadFolderPrefixAdpcmOnly(
      "/FIXEDSOUND",
      "LONG_"
    );

  Serial.printf("SHORT : %u\r\n", n1);
  Serial.printf("LONG  : %u\r\n", n2);

  audio.printFxCacheStatus();
}

void loop()
{
}