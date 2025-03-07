/*
  FrSky ACC sensor class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Clooney 20150814
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_ACC_H_
#define _FRSKY_SPORT_SENSOR_ACC_H_

#include "FrSkySportSensor.h"

#define ACC_DEFAULT_ID ID11
#define ACC_DATA_COUNT 3
#define AccX_DATA_ID 0x0700
#define AccY_DATA_ID 0x0710
#define AccZ_DATA_ID 0x0720

#define AccX_DATA_PERIOD 100
#define AccY_DATA_PERIOD 100
#define AccZ_DATA_PERIOD 100

class FrSkySportSensorAcc : public FrSkySportSensor
{
  public:
    FrSkySportSensorAcc(SensorId id = ACC_DEFAULT_ID);
    void setData(float AccX, float AccY, float AccZ);
    virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);

  private:
    int32_t AccX;
    int32_t AccY;
    int32_t AccZ;
    uint32_t AccXTime;
    uint32_t AccYTime;
    uint32_t AccZTime;
};

#endif // _FRSKY_SPORT_SENSOR_ACC_H_
