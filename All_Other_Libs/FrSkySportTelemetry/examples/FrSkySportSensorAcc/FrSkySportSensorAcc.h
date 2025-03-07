/*
  Acceleration sensor class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)

  Not for commercial use
  
  Note that FrSky does not yet provide this sensor so it is based on the specification only and may change in the future
*/

#ifndef _FRSKY_SPORT_SENSOR_ACC_H_
#define _FRSKY_SPORT_SENSOR_ACC_H_

#include "FrSkySportSensor.h"

#define ACC_DEFAULT_ID ID11
#define ACC_DATA_COUNT 3
#define ACC_X_DATA_ID 0x0700
#define ACC_Y_DATA_ID 0x0710
#define ACC_Z_DATA_ID 0x0720

#define ACC_X_DATA_PERIOD 100
#define ACC_Y_DATA_PERIOD 100
#define ACC_Z_DATA_PERIOD 100

class FrSkySportSensorAcc : public FrSkySportSensor
{
  public:
    FrSkySportSensorAcc(SensorId id = ACC_DEFAULT_ID);
    void setData(float accX, float accY, float accZ);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);

  private:
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t xTime;
    uint32_t yTime;
    uint32_t zTime;
};

#endif // _FRSKY_SPORT_SENSOR_ACC_H_
