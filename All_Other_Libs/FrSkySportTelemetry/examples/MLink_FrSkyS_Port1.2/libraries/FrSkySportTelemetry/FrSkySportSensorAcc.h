/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_RPMT12_H_
#define _FRSKY_SPORT_SENSOR_RPMT12_H_

#include "FrSkySportSensor.h"

#define ACC_DEFAULT_ID ID6
#define ACC_DATA_COUNT 3
#define ACC_X_DATA_ID 0x0700
#define ACC_Y_DATA_ID 0x0710
#define ACC_Z_DATA_ID 0x0720

#define ACC_X_DATA_PERIOD  100
#define ACC_Y_DATA_PERIOD  100
#define ACC_Z_DATA_PERIOD  100

class FrSkySportSensorAcc : public FrSkySportSensor
{
  public:
    FrSkySportSensorAcc(SensorId id = ACC_DEFAULT_ID);
    void setData(float x, float y, float z);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    int32_t getX();
    int32_t getY();
    int32_t getZ();

  private:
    int32_t xData;
    int32_t yData;
    int32_t zData;
    uint32_t xTime;
    uint32_t yTime;
    uint32_t zTime;
    float x;
    float y;
    float z;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_RPM_H_
