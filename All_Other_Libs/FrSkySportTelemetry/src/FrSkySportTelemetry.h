/*
  FrSky telemetry class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210108
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_TELEMETRY_H_
#define _FRSKY_SPORT_TELEMETRY_H_

#include "Arduino.h"
#include "FrSkySportPolling.h"
#include "FrSkySportSensor.h"
#include "FrSkySportSingleWireSerial.h"

#define FRSKY_TELEMETRY_MAX_SENSORS 28

class FrSkySportTelemetry
{
  public:
    FrSkySportTelemetry(bool polling); // This constructor is obsolete and kept for backward compatibility only
    FrSkySportTelemetry(FrSkySportPolling *polling = NULL);
    void begin(FrSkySportSingleWireSerial::SerialId id,
                FrSkySportSensor* sensor1,         FrSkySportSensor* sensor2 =  NULL, FrSkySportSensor* sensor3 =  NULL, 
                FrSkySportSensor* sensor4  = NULL, FrSkySportSensor* sensor5 =  NULL, FrSkySportSensor* sensor6 =  NULL,
                FrSkySportSensor* sensor7  = NULL, FrSkySportSensor* sensor8 =  NULL, FrSkySportSensor* sensor9 =  NULL, 
                FrSkySportSensor* sensor10 = NULL, FrSkySportSensor* sensor11 = NULL, FrSkySportSensor* sensor12 = NULL,
                FrSkySportSensor* sensor13 = NULL, FrSkySportSensor* sensor14 = NULL, FrSkySportSensor* sensor15 = NULL,
                FrSkySportSensor* sensor16 = NULL, FrSkySportSensor* sensor17 = NULL, FrSkySportSensor* sensor18 = NULL,
                FrSkySportSensor* sensor19 = NULL, FrSkySportSensor* sensor20 = NULL, FrSkySportSensor* sensor21 = NULL,
                FrSkySportSensor* sensor22 = NULL, FrSkySportSensor* sensor23 = NULL, FrSkySportSensor* sensor24 = NULL,
                FrSkySportSensor* sensor25 = NULL, FrSkySportSensor* sensor26 = NULL, FrSkySportSensor* sensor27 = NULL,
                FrSkySportSensor* sensor28 = NULL);

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    // ESP32/S3 true single-wire S.Port: RX and TX on the same GPIO.
    void begin(FrSkySportSingleWireSerial::SerialId id,
                int8_t sportPin,
                FrSkySportSensor* sensor1,         FrSkySportSensor* sensor2 =  NULL, FrSkySportSensor* sensor3 =  NULL, 
                FrSkySportSensor* sensor4  = NULL, FrSkySportSensor* sensor5 =  NULL, FrSkySportSensor* sensor6 =  NULL,
                FrSkySportSensor* sensor7  = NULL, FrSkySportSensor* sensor8 =  NULL, FrSkySportSensor* sensor9 =  NULL, 
                FrSkySportSensor* sensor10 = NULL, FrSkySportSensor* sensor11 = NULL, FrSkySportSensor* sensor12 = NULL,
                FrSkySportSensor* sensor13 = NULL, FrSkySportSensor* sensor14 = NULL, FrSkySportSensor* sensor15 = NULL,
                FrSkySportSensor* sensor16 = NULL, FrSkySportSensor* sensor17 = NULL, FrSkySportSensor* sensor18 = NULL,
                FrSkySportSensor* sensor19 = NULL, FrSkySportSensor* sensor20 = NULL, FrSkySportSensor* sensor21 = NULL,
                FrSkySportSensor* sensor22 = NULL, FrSkySportSensor* sensor23 = NULL, FrSkySportSensor* sensor24 = NULL,
                FrSkySportSensor* sensor25 = NULL, FrSkySportSensor* sensor26 = NULL, FrSkySportSensor* sensor27 = NULL,
                FrSkySportSensor* sensor28 = NULL);

    void begin(FrSkySportSingleWireSerial::SerialId id,
                int8_t rxPin, int8_t txPin,
                FrSkySportSensor* sensor1,         FrSkySportSensor* sensor2 =  NULL, FrSkySportSensor* sensor3 =  NULL, 
                FrSkySportSensor* sensor4  = NULL, FrSkySportSensor* sensor5 =  NULL, FrSkySportSensor* sensor6 =  NULL,
                FrSkySportSensor* sensor7  = NULL, FrSkySportSensor* sensor8 =  NULL, FrSkySportSensor* sensor9 =  NULL, 
                FrSkySportSensor* sensor10 = NULL, FrSkySportSensor* sensor11 = NULL, FrSkySportSensor* sensor12 = NULL,
                FrSkySportSensor* sensor13 = NULL, FrSkySportSensor* sensor14 = NULL, FrSkySportSensor* sensor15 = NULL,
                FrSkySportSensor* sensor16 = NULL, FrSkySportSensor* sensor17 = NULL, FrSkySportSensor* sensor18 = NULL,
                FrSkySportSensor* sensor19 = NULL, FrSkySportSensor* sensor20 = NULL, FrSkySportSensor* sensor21 = NULL,
                FrSkySportSensor* sensor22 = NULL, FrSkySportSensor* sensor23 = NULL, FrSkySportSensor* sensor24 = NULL,
                FrSkySportSensor* sensor25 = NULL, FrSkySportSensor* sensor26 = NULL, FrSkySportSensor* sensor27 = NULL,
                FrSkySportSensor* sensor28 = NULL);
#endif
    uint16_t send();
    void setData(uint8_t rssi, float rxBatt);

  private:
    FrSkySportSensor* sensors[FRSKY_TELEMETRY_MAX_SENSORS];
    FrSkySportSingleWireSerial serial;
    uint8_t  sensorCount;
    uint8_t  prevData;
    FrSkySportPolling *pollingClass;
};

#endif // _FRSKY_SPORT_TELEMETRY_H_
