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

#include <MPX_MSB.h>
#include <TinyGPSPlus.h>
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


TinyGPSPlus gps;

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
HardwareSerial GpsSerial(2);
static const int8_t GPS_RX_PIN = 17; // GPS TX -> ESP32 RX
static const int8_t GPS_TX_PIN = 18; // optional, ESP32 TX -> GPS RX
static inline void beginGpsSerial() {
  GpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}
#else
#define GpsSerial Serial1
static inline void beginGpsSerial() {
  GpsSerial.begin(9600);
}
#endif

float batteryVoltage = 15.0f;
float Celcius = 25.0f;
float TempTank = 30.0f;
static uint32_t lastFixMs = 0;
static float lastAlt = 0.0f;

void setup() {
  beginMpxTelemetry();
  mpx.setIdleMicros(300);
  mpx.setEchoMasking(true);

  mpx.sendVbat(batteryVoltage);
  mpx.sendTmp1(Celcius);
  mpx.sendTmp2(TempTank);
  mpx.mapGpsAddrs(/*alt*/8, /*spd*/4, /*cog*/1);
  mpx.mapVarioAddrs(/*alt*/8, /*vspd*/2);

  beginGpsSerial();
}

void loop() {
  while (GpsSerial.available()) gps.encode(GpsSerial.read());

  if (gps.location.isValid() && gps.date.isUpdated() && gps.time.isUpdated()) {
    double lat = gps.location.lat();
    double lon = gps.location.lng();
    float alt = gps.altitude.isValid() ? gps.altitude.meters() : lastAlt;
    float spd = gps.speed.isValid() ? gps.speed.mps() : 0.0f;
    float cog = gps.course.isValid() ? gps.course.deg() : 0.0f;

    uint32_t now = millis();
    float vspd = 0.0f;
    if (lastFixMs) {
      float dt = (now - lastFixMs) / 1000.0f;
      if (dt > 0.001f) vspd = (alt - lastAlt) / dt;
    }
    lastFixMs = now;
    lastAlt = alt;

    mpx.Gps(lat, lon, alt, spd, cog,
            (uint8_t)(gps.date.year() - 2000),
            (uint8_t)gps.date.month(),
            (uint8_t)gps.date.day(),
            (uint8_t)gps.time.hour(),
            (uint8_t)gps.time.minute(),
            (uint8_t)gps.time.second());
    mpx.Vario(alt, vspd);
  }

  mpx.sendVbat(batteryVoltage);
  mpx.sendTmp1(Celcius);
  mpx.sendTmp2(TempTank);
  mpx.poll();
}
