#include <SPI.h>
#include <SD.h>
#include <ProMicroEngineSound.h>

// Selon le schéma fourni :
// - Arduino Pro Micro 5V / 16 MHz
// - SD en SPI
// - CS SD sur D10
// - sortie audio PWM sur D6 / OC4D
//
// Fichiers sur la SD : vrais WAV PCM renommés :
//   /DSL-V12.STA  démarrage moteur
//   /DSL-V12.IDL  idle moteur bouclé
//
// Le fichier testé DSL-V12.STA est : WAV PCM, mono, 8 bits, 16000 Hz.

static const uint8_t SD_CS_PIN     = 10;
static const uint8_t THROTTLE_PIN  = 5;
static const uint8_t AUDIO_PIN     = 6;

static uint16_t readThrottleUs()
{
  uint32_t v = pulseIn(THROTTLE_PIN, HIGH, 25000UL);
  if (v < 850 || v > 2200) {
    return 1000;
  }
  return (uint16_t)v;
}

void setup()
{
  pinMode(THROTTLE_PIN, INPUT);
  pinMode(AUDIO_PIN, OUTPUT);

  Serial.begin(115200);
  delay(500);

  Serial.println(F("ProMicroEngineSound WAV example"));

  if (!ProMicroEngineSound.begin(SD_CS_PIN, "/DSL-V12.STA", "/DSL-V12.IDL", 16000)) {
    Serial.println(F("SD init failed or sound engine init failed"));
    while (1) {
      delay(1000);
    }
  }

  // Q8 : 256 = 1.00x, 384 = 1.50x, 512 = 2.00x.
  ProMicroEngineSound.setPitchRangeQ8(256, 512);
  ProMicroEngineSound.setVolume(220);
  ProMicroEngineSound.startEngine();
}

void loop()
{
  uint16_t throttleUs = readThrottleUs();
  ProMicroEngineSound.setThrottleUs(throttleUs, 1000, 2000);
  ProMicroEngineSound.update();
}
