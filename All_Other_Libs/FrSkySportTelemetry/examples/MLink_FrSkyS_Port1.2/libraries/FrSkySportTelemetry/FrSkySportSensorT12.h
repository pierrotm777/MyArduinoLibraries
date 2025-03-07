/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_T12_H_
#define _FRSKY_SPORT_SENSOR_T12_H_

#include "FrSkySportSensor.h"

#define T12_DEFAULT_ID ID5
#define T12_DATA_COUNT 3
#define T12_T1_DATA_ID 0x0400
#define T12_T2_DATA_ID 0x0410


#define T12_T1_DATA_PERIOD  3000
#define T12_T2_DATA_PERIOD  3000


class FrSkySportSensorT12 : public FrSkySportSensor
{
  public:
    FrSkySportSensorT12(SensorId id = T12_DEFAULT_ID);
    void setData(float t1 = 0.0, float t2 = 0.0);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);

    int32_t getT1();
    int32_t getT2();

  private:

    int32_t t1Data;
    int32_t t2Data;

    uint32_t t1Time;
    uint32_t t2Time;

    int32_t t1;
    int32_t t2;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_RPM_H_
