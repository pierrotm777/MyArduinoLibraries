/*
  FrSky S-Port Telemetry library example
  (c) Pawelsky 20160916
  Not for commercial use
  
  Note that you need Teensy 3.x/LC or 328P based (e.g. Pro Mini, Nano, Uno) board and FrSkySportTelemetry library for this example to work
*/
#include <MLinkExMin.h>
#include <avr/io.h>
#include <avr/wdt.h>
MLinkExMin mLink(13); //LED 13 blinks on every MLink data set
// Uncomment the #define below to enable internal polling of data.
// Use only when there is no device in the S.Port chain (e.g. S.Port capable FrSky receiver) that normally polls the data.
//#define POLLING_ENABLED

#include "FrSkySportSensor.h"
#include "FrSkySportSensorAss.h"
#include "FrSkySportSensorFuel.h"
#include "FrSkySportSensorFcs.h"
#include "FrSkySportSensorFlvss.h"
#include "FrSkySportSensorGps.h"
#include "FrSkySportSensorRpm.h"
#include "FrSkySportSensorSp2uart.h"
#include "FrSkySportSensorVario.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"
//#if !defined(__MK20DX128__) && !defined(__MK20DX256__) && !defined(__MKL26Z64__) && !defined(__MK66FX1M0__) && !defined(__MK64FX512__)
#include "SoftwareSerial.h"
//#endif

FrSkySportSensorAss ass;                               // Create ASS sensor with default ID
FrSkySportSensorFuel fuel;                               // Create ASS sensor with default ID
FrSkySportSensorFcs fcs;                               // Create FCS-40A sensor with default ID (use ID8 for FCS-150A)
FrSkySportSensorFlvss flvss1;                          // Create FLVSS sensor with default ID
FrSkySportSensorFlvss flvss2(FrSkySportSensor::ID15);  // Create FLVSS sensor with given ID
FrSkySportSensorGps gps;                               // Create GPS sensor with default ID
FrSkySportSensorRpm rpm;                               // Create RPM sensor with default ID
FrSkySportSensorSp2uart sp2uart;                       // Create SP2UART Type B sensor with default ID
FrSkySportSensorVario vario;                           // Create Variometer sensor with default ID
//#ifdef POLLING_ENABLED
  FrSkySportTelemetry telemetry(true);                 // Create telemetry object with polling
//#else
//  FrSkySportTelemetry telemetry;                       // Create telemetry object without polling
//#endif

void setup()
{
  // Configure the telemetry serial port and sensors (remember to use & to specify a pointer to sensor)
//#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK66FX1M0__) || defined(__MK64FX512__)
//  telemetry.begin(FrSkySportSingleWireSerial::SERIAL_3, &ass, &fcs, &flvss1, &flvss2, &gps, &rpm, &sp2uart, &vario);
//#else
  telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_3, &ass, &fuel, &fcs, &flvss1, &flvss2, &gps, &rpm, &sp2uart, &vario);
//#endif

  wdt_enable(WDTO_1S);
   Serial.begin(38400);
   wdt_reset();

   
}

void loop()
{

      sp2uart.setData(4.1,5.3); 
      ass.setData(10.0);
      fuel.setData(93);
      fcs.setData(strom, u_min);  
      flvss1.setData(4.07, 4.08, 4.09, 4.10, 4.11, 4.12);  // Cell voltages in volts (cells 1-6)
      flvss2.setData(4.13, 4.14);                          // Cell voltages in volts (cells 7-8)

  // Set GPS data
  gps.setData(48.858289, 2.294502,   // Latitude and longitude in degrees decimal (positive for N/E, negative for S/W)
              245.5,                 // Altitude in m (can be negative)
              100.0,                 // Speed in m/s
              90.23,                 // Course over ground in degrees (0-359, 0 = north)
              14, 9, 14,             // Date (year - 2000, month, day)
              12, 00, 00);           // Time (hour, minute, second) - will be affected by timezone setings in your radio

  rpm.setData(200,    // Rotations per minute
              25.6,   // Temperature #1 in degrees Celsuis (can be negative, will be rounded)
              -7.8);  // Temperature #2 in degrees Celsuis (can be negative, will be rounded)

  vario.setData(250.5,  // Altitude in meters (can be negative)
                -1.5);  // Vertical speed in m/s (positive - up, negative - down)

  telemetry.send(100);
}
