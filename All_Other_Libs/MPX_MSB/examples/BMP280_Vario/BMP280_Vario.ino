/*
  MPX_MSB example adapted for Teensy 4.0 and ESP32-S3.

  Teensy 4.0:
    - MPX telemetry uses Serial3 @38400.

  ESP32 / ESP32-S3:
    - MPX telemetry uses HardwareSerial(1).
    - Default pins below are RX=2, TX=1 because they matched Pierre's test.
    - Change MPX_RX_PIN / MPX_TX_PIN if needed.

  Wiring reminder:
    TX -> 1N4148 diode -> B/D bus, diode cathode on B/D side.
    B/D bus -> 1k resistor -> RX.
    GND common.
*/

#include <Wire.h>
#include <math.h>
#include <Adafruit_BMP280.h>
#include <MPX_MSB.h>
using namespace MPX;

Mpx_Msb mpx;

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
HardwareSerial MpxSerial(1);
static const int8_t MPX_RX_PIN = 2;
static const int8_t MPX_TX_PIN = 1;
static inline void beginMpxTelemetry() {
  mpx.begin(MpxSerial, MPX_RX_PIN, MPX_TX_PIN);
}
#else
static inline void beginMpxTelemetry() {
  mpx.begin(Serial3, 38400);
}
#endif


Adafruit_BMP280 bmp;

#define BMP280_ADDR_PRIMARY   0x76
#define BMP280_ADDR_SECONDARY 0x77

float SEA_LEVEL_HPA = 1013.25f;
float batteryVoltage = 15.0f;
float temp2_demo = 30.0f;
static uint32_t lastMs = 0;
static float lastAlt = 0.0f;

void setup() {
  beginMpxTelemetry();
  mpx.setIdleMicros(300);
  mpx.setEchoMasking(true);

  mpx.sendVbat(batteryVoltage);
  mpx.sendTmp1(25.0f);
  mpx.sendTmp2(temp2_demo);
  mpx.mapVarioAddrs(/*alt*/8, /*vspd*/2);

  Wire.begin();
  bool ok = bmp.begin(BMP280_ADDR_PRIMARY);
  if (!ok) ok = bmp.begin(BMP280_ADDR_SECONDARY);

  if (ok) {
    bmp.setSampling(
      Adafruit_BMP280::MODE_NORMAL,
      Adafruit_BMP280::SAMPLING_X2,
      Adafruit_BMP280::SAMPLING_X16,
      Adafruit_BMP280::FILTER_X16,
      Adafruit_BMP280::STANDBY_MS_500
    );
    float tC = bmp.readTemperature();
    float alt = bmp.readAltitude(SEA_LEVEL_HPA);
    if (isfinite(tC)) mpx.sendTmp1(tC);
    lastAlt = isfinite(alt) ? alt : 0.0f;
    mpx.Vario(lastAlt, 0.0f);
  }
}

void loop() {
  float tC = bmp.readTemperature();
  float altM = bmp.readAltitude(SEA_LEVEL_HPA);

  mpx.sendVbat(batteryVoltage);
  if (isfinite(tC)) mpx.sendTmp1(tC);
  mpx.sendTmp2(temp2_demo);

  if (isfinite(altM)) {
    float vspd = 0.0f;
    uint32_t now = millis();
    if (lastMs != 0) {
      float dt = (now - lastMs) / 1000.0f;
      if (dt > 0.002f) vspd = (altM - lastAlt) / dt;
    }
    lastAlt = altM;
    lastMs = now;
    mpx.Vario(altM, vspd);
  }

  mpx.poll();
}
