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

uint32_t lastPlay = 0;

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

  audio.setMasterVolume(0.7f);
}

void loop()
{
  audio.streamTick();
  audio.chainTick();

  if (millis() - lastPlay > 5000) {
    lastPlay = millis();

    Serial.println("PLAY FX");
   // playFxAuto(
//   path,      -> chemin WAV
//   volume,    -> 0.0f à 1.0f
//   priority,  -> priorité FX (plus grand = plus prioritaire)
//   pitch      -> vitesse/pitch (1.0 = normal)
// )

audio.playFxAuto(
  "/FIXEDSOUND/CANNON.wav", // path
  0.8f,                     // volume
  200,                      // priority
  1.0f                      // pitch
);
  }
}