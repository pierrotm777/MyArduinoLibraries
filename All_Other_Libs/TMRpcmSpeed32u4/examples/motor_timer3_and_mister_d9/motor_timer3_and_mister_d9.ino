/*
  TMRpcmSpeed32u4 - Motor (D6) + Mister (~113kHz on D9) coexist demo

  Use case:
    - Keep the motor audio on D6 (Timer4 PWM)
    - Generate ~113 kHz drive on D9 for a piezo mister board
    - Avoid timer conflict by running the audio sample ISR on Timer3 (not Timer1)

  IMPORTANT:
    - D9 must be a Timer1 output (OC1A/OC1B depending on core mapping).
      On SparkFun Pro Micro / Leonardo cores, Arduino pin 9 is typically OC1A.
    - The mister board you showed expects a high-frequency square (around 113 kHz),
      NOT a low-frequency analogWrite() PWM.

  Wiring (minimal):
    - SD card CS on D10 (change SD_CS_PIN if needed)
    - Mister gate input on D9 (through your existing RC network)

  Serial commands:
    o : mister ON
    f : mister OFF
*/

#define TMRPCM_32U4_AUDIO_TIMER 3   // <-- audio ISR uses Timer3, frees Timer1 for mister

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcmSpeed32u4.h>

TMRpcmSpeed32u4 audio;

const uint8_t SD_CS_PIN = 10;
const uint8_t MISTER_PIN = 9;  // Timer1 OC1x output

// -------- Mister (Timer1) @ ~113kHz --------
static const uint32_t F_CPU_HZ  = 16000000UL;
static const uint32_t TARGET_HZ = 113000UL;

// Fast PWM, TOP=ICR1, prescaler=1 : f = F_CPU / (1*(1+TOP))
static const uint16_t TOP1 = (uint16_t)((F_CPU_HZ / TARGET_HZ) - 1);

static void misterSetDutyPermille(uint16_t permille) {
  if (permille > 1000) permille = 1000;
  uint32_t v = (uint32_t)(TOP1 + 1) * permille / 1000;
  if (v == 0) v = 1;
  if (v > TOP1) v = TOP1;
  OCR1A = (uint16_t)v;
}

static void misterInitTimer1_113k() {
  pinMode(MISTER_PIN, OUTPUT);
  digitalWrite(MISTER_PIN, LOW);

  // Timer1 Fast PWM, TOP=ICR1 (mode 14)
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12);

  // Prescaler /1
  TCCR1B |= (1 << CS10);

  ICR1 = TOP1;
  misterSetDutyPermille(500); // 50%

  // Start OFF by default (disconnect OC1A)
  TCCR1A &= ~(1 << COM1A1);
  digitalWrite(MISTER_PIN, LOW);
}

static void misterOn() {
  // Connect OC1A to pin (non-inverting PWM)
  TCCR1A |= (1 << COM1A1);
}

static void misterOff() {
  TCCR1A &= ~(1 << COM1A1);
  digitalWrite(MISTER_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 1500)) { delay(1); }

  if (Serial) {
    Serial.println(F("Motor (Timer3/D6) + Mister (Timer1/D9) demo"));
  }

  if (!SD.begin(SD_CS_PIN)) {
    if (Serial) Serial.println(F("SD FAIL"));
    while (1) {}
  }

  if (!audio.begin()) {
    if (Serial) Serial.println(F("audio.begin FAILED"));
    while (1) {}
  }

  audio.setVolume(8);
  audio.loopPlayback = true;

  if (!audio.play("/IDLE.WAV")) {
    if (Serial) Serial.println(F("play() failed: put /IDLE.WAV on SD (8-bit mono PCM)"));
  }

  misterInitTimer1_113k();

  if (Serial) {
    Serial.print(F("Timer1 TOP=")); Serial.println(TOP1);
    Serial.println(F("Commands: o=ON, f=OFF"));
  }
}

void loop() {
  // Keep audio flowing
  audio.update();

  // Simple serial control for the mister
  if (Serial && Serial.available()) {
    char c = (char)Serial.read();
    if (c == 'o') { misterOn();  Serial.println(F("mister ON")); }
    if (c == 'f') { misterOff(); Serial.println(F("mister OFF")); }
  }
}
