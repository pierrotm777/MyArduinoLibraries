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


#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
static const uint8_t AlarmPins[4] = {8, 9, 10, 11};
#else
static const uint8_t AlarmPins[4] = {18, 19, 22, 23};
#endif

void setup() {
  beginMpxTelemetry();
  mpx.setIdleMicros(300);
  mpx.setEchoMasking(true);

  mpx.sendVbat(15.0f);  // addr 3 by default
  mpx.sendTmp1(25.0f);  // addr 6 by default
  mpx.sendTmp2(30.0f);  // addr 7 by default

  mpx.addAlarmDigital(AlarmPins[0],  9, MPX_LIQUID);
  mpx.addAlarmDigital(AlarmPins[1], 10, MPX_LIQUID);
  mpx.addAlarmDigital(AlarmPins[2], 11, MPX_LIQUID);
  mpx.addAlarmDigital(AlarmPins[3], 12, MPX_LIQUID);
}

void loop() {
  // Update values before poll() if they are dynamic.
  mpx.sendVbat(15.0f);
  mpx.sendTmp1(25.0f);
  mpx.sendTmp2(30.0f);
  mpx.poll();
}
