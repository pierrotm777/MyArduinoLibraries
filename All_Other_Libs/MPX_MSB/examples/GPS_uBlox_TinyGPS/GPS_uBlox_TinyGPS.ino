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

/*
  u-blox GNSS example using the SparkFun_u-blox_GNSS_Arduino_Library over I2C.
  The folder name is kept as GPS_uBlox_TinyGPS for compatibility with older archives,
  but this sketch does not use TinyGPS++.
*/
#include <Wire.h>
#include <MPX_MSB.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
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


SFE_UBLOX_GNSS gnss;
float batteryVoltage = 15.0f;
float Celcius = 25.0f;
float TempTank = 30.0f;
static uint32_t lastPvtMs = 0;
static long lastAltMSL = 0;

void setup() {
  beginMpxTelemetry();
  mpx.setIdleMicros(300);
  mpx.setEchoMasking(true);

  mpx.sendVbat(batteryVoltage);
  mpx.sendTmp1(Celcius);
  mpx.sendTmp2(TempTank);
  mpx.mapGpsAddrs(/*alt*/8, /*spd*/4, /*cog*/1);
  mpx.mapVarioAddrs(/*alt*/8, /*vspd*/2);

  Wire.begin();
  if (!gnss.begin()) {
    // Keep telemetry alive even if GNSS is not detected.
  } else {
    gnss.setAutoPVT(true);
    gnss.setAutoVELNED(true);
  }
}

void loop() {
  if (gnss.getPVT()) {
    double lat = gnss.getLatitude() / 1e7;
    double lon = gnss.getLongitude() / 1e7;
    long altMSL = gnss.getAltitudeMSL();
    float alt = altMSL / 1000.0f;
    float spd_mps = gnss.getGroundSpeed() / 1000.0f;
    float cog = gnss.getHeading() / 1e5;
    float vspd = 0.0f;

    if (gnss.getVELNED()) {
      vspd = -(gnss.getDownSpeed()) / 1000.0f;
    } else {
      uint32_t now = millis();
      if (lastPvtMs) {
        float dt = (now - lastPvtMs) / 1000.0f;
        if (dt > 0.001f) vspd = (alt - (lastAltMSL / 1000.0f)) / dt;
      }
      lastPvtMs = now;
      lastAltMSL = altMSL;
    }

    mpx.Gps(lat, lon, alt, spd_mps, cog,
            (uint8_t)(gnss.getYear() - 2000),
            (uint8_t)gnss.getMonth(),
            (uint8_t)gnss.getDay(),
            (uint8_t)gnss.getHour(),
            (uint8_t)gnss.getMinute(),
            (uint8_t)gnss.getSecond());
    mpx.Vario(alt, vspd);
  }

  mpx.sendVbat(batteryVoltage);
  mpx.sendTmp1(Celcius);
  mpx.sendTmp2(TempTank);
  mpx.poll();
}
