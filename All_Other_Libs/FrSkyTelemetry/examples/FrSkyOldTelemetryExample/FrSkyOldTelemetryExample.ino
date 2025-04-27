/*
  FrSky Telemetry library example
  (c) Pawelsky 20170831
  Not for commercial use

  Note that you need Teensy 3.x/LC or 328P/168 based (e.g. Pro Mini, Nano, Uno) board and FrSkyTelemetry library for this example to work

250426 Modified by Pete (El_Supremo) to also allow use of
Teensy 4.0 or T4.1 - a loopback test works but this
has not been tested "live".
At the moment, SERIAL_USB can't be used
*/

// Do a loop test on Serial2 just to verify that
// it will send data
//#define LOOP_TEST 1
/* First 5 lines of output from loop test
00
A89D3C7BA83A3C86D1CEFE86CFFEFE8600
A89FCC86FD2A86ED36FE86DDF686CD66FE86DB9A86CB4A9686BBEAFE86D9323BA84E9267E5C6FE86D796FE86C7A2FE8600
A84D80868BE6FE8689E6FE86F3E232A8A7E086BD36FE869F760CA89B382EA89BFE86B37623A83F3986F51E00
A8E2FE8600
*/

#include <FrSkyTelemetry.h>
#if !defined(ARDUINO_TEENSY41) && !defined(ARDUINO_TEENSY40) && !defined(__MK20DX128__) && !defined(__MK20DX256__) && !defined(__MKL26Z64__) && !defined(__MK66FX1M0__) && !defined(__MK64FX512__)
#include <SoftwareSerial.h>
#endif

FrSkyTelemetry telemetry; // Create telemetry object

void setup()
{

  // Configure the telemetry serial port
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40) || defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK66FX1M0__) || defined(__MK64FX512__)
#ifdef LOOP_TEST
  // Start the Serial monitor for the test
  Serial.begin(9600);
  while(!Serial);
// Use _2 for testing (it's easier to reach)
  telemetry.begin(FrSkyTelemetry::SERIAL_2);
#else
// Original code
  telemetry.begin(FrSkyTelemetry::SERIAL_3);
#endif
#else
  telemetry.begin(FrSkyTelemetry::SOFT_SERIAL_PIN_12);
#endif



}

void loop()
{
  // Set current/voltage sensor (FAS) data
  // (set Voltage source to FAS in menu to use this data for battery voltage,
  //  set Current source to FAS in menu to use this data for current readins)
  telemetry.setFasData(25.3,   // Current consumption in amps
                       12.6);  // Battery voltage in volts

  // Set fuel sensor (FGS) data
  telemetry.setFgsData(55.5);  // Fuel level in percent

  // Set LiPo voltage sensor (FLVS) data (we use two sensors to simulate 8S battery
  // (set Voltage source to Cells in menu to use this data for battery voltage)
  telemetry.setFlvsData(4.07, 4.08, 4.09, 4.10, 4.11, 4.12, 4.13, 4.14);  // Cell voltages in volts (cells 1-8). Cells 9-12 are not used in this example

  // Set variometer sensor (FVAS) data
  telemetry.setFvasData(245.5,   // Altitude in m (can be nevative)
                        -3.5);  // Vertical speed in m/s (can be nevative, 0.0m/s will be set when skipped)

  // Set GPS data
  telemetry.setGpsData(48.858289, 2.294502,   // Latitude and longitude in degrees decimal (positive for N/E, negative for S/W)
                       245.5,                 // Altitude in m (can be nevative)
                       100.0,                 // Speed in m/s
                       90.23,                 // Course over ground in degrees
                       14, 9, 14,             // Date (year - 2000, month, day)
                       12, 00, 00);           // Time (hour, minute, second) - will be affected by timezone setings in your radio

  // Set triaxial acceleration sensor (TAS) data
  telemetry.setTasData(17.95,    // x-axis acceleration in g (can be negative)
                       0.0,      // y-axis acceleration in g (can be negative)
                       -17.95);  // z-axis acceleration in g (can be negative)

  // Set temperature sensor (TEMS) data
  telemetry.setTemsData(25.6,   // Temperature #1 in degrees Celsuis (can be negative)
                        -7.8);  // Temperature #2 in degrees Celsuis (can be negative)

  // Set RPM sensor (RPMS) data
  // (set number of blades to 2 in telemetry menu to get correct rpm value)
  telemetry.setRpmsData(200);  // Rotations per minute

  // Send the telemetry data, note that the data will only be sent for sensors
  // that had their data set at least once. Also it will only be set in defined
  // time intervals, so not necessarily at every call to send() method.
  telemetry.send();

#ifdef LOOP_TEST
  while(Serial2.available() > 0) {
    uint8_t rc = Serial2.read();
    Serial.printf("%02X",rc);
    if(rc == 0)Serial.println();
  }
#endif
}
