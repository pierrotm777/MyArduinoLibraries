#include <SPI.h>
#include <SD.h>

// Choose the timer used for the audio sample ISR:
//   1 = Timer1 (default)
//   3 = Timer3 (frees Timer1 for other uses such as a 113kHz mister on D9)
#define TMRPCM_32U4_AUDIO_TIMER 3
#include <TMRpcmSpeed32u4.h>

#define SD_CS 10

TMRpcmSpeed32u4 audio;

void setup() {
  Serial.begin(115200);
  delay(300);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAIL");
    while (1);
  }
  Serial.println("SD OK");

  if (!audio.begin()) {
    Serial.println("audio.begin FAIL");
    while (1);
  }
  Serial.println("audio.begin OK");

  audio.setSampleRate(16000);
  audio.setVolume(8);
  audio.loopPlayback = true;

  audio.play("/ENGINE.IDL");  // put a PCM 8-bit mono WAV (renamed .IDL) on SD root
}

void loop() {
  audio.update();
}
