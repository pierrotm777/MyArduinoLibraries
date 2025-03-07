/*
  FrSky sensor data polling class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_POLLING_H_
#define _FRSKY_SPORT_POLLING_H_

#include "Arduino.h"
#include "FrSkySportSensor.h"
#include "FrSkySportSingleWireSerial.h"

#define FRSKY_POLLIED_ID_COUNT 28

#define POLLING_RSSI_DATA_ID   0xF101
#define POLLING_ID_POLL_TIME   12
#define POLLING_RSSI_POLL_TIME 444 // 444 is standard - 888 is working too (for speeding up other pollings if necessary)

class FrSkySportPolling
{
  public:
    FrSkySportPolling();
    FrSkySportSensor::SensorId pollData(FrSkySportSingleWireSerial& serial, uint32_t now,uint32_t rssi);

  private:
    FrSkySportSensor::SensorId getNextId();
    const FrSkySportSensor::SensorId POLLED_ID_TABLE[FRSKY_POLLIED_ID_COUNT] = { FrSkySportSensor::ID1,  FrSkySportSensor::ID2,
                                                                                  FrSkySportSensor::ID3,  FrSkySportSensor::ID4,
                                                                                  FrSkySportSensor::ID5,  FrSkySportSensor::ID6,
                                                                                  FrSkySportSensor::ID7,  FrSkySportSensor::ID8,
                                                                                  FrSkySportSensor::ID9,  FrSkySportSensor::ID10,
                                                                                  FrSkySportSensor::ID11, FrSkySportSensor::ID12,
                                                                                  FrSkySportSensor::ID13, FrSkySportSensor::ID14,
                                                                                  FrSkySportSensor::ID15, FrSkySportSensor::ID16,
                                                                                  FrSkySportSensor::ID17, FrSkySportSensor::ID18,
                                                                                  FrSkySportSensor::ID19, FrSkySportSensor::ID20,
                                                                                  FrSkySportSensor::ID21, FrSkySportSensor::ID22,
                                                                                  FrSkySportSensor::ID23, FrSkySportSensor::ID24,
                                                                                  FrSkySportSensor::ID25, FrSkySportSensor::ID26,
                                                                                  FrSkySportSensor::ID27, FrSkySportSensor::ID28 };
    uint8_t  nextPollIdIdx;
    uint32_t nextPollTime;
    uint32_t rssiPollTime;
};

#endif // _FRSKY_SPORT_POLLING_H_

