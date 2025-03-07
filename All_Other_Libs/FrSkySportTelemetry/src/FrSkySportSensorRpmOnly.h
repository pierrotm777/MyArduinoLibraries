/*
  FrSky RPM sensor class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210108
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_RPMONLY_H_
#define _FRSKY_SPORT_SENSOR_RPMONLY_H_

#include "FrSkySportSensor.h"

#define RPM_DEFAULT_ID ID5
#define RPM_DATA_COUNT 3
//#define RPM_T1_DATA_ID 0x0400
//#define RPM_T2_DATA_ID 0x0410
#define RPM_ROT_DATA_ID 0x0500

//#define RPM_T1_DATA_PERIOD  1000
//#define RPM_T2_DATA_PERIOD  1000
#define RPM_ROT_DATA_PERIOD 500

class FrSkySportSensorRpmOnly : public FrSkySportSensor
{
  public:
    FrSkySportSensorRpmOnly(SensorId id = RPM_DEFAULT_ID);
    //void setData(uint32_t rpm, float t1 = 0.0, float t2 = 0.0);
	void setData(uint32_t rpm);
    virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    uint32_t getRpm();
    //int32_t getT1();
    //int32_t getT2();

  private:
    uint32_t rpmData;
    //int32_t t1Data;
    //int32_t t2Data;
    uint32_t rpmTime;
    //uint32_t t1Time;
    //uint32_t t2Time;
    uint32_t rpm;
    //int32_t t1;
    //int32_t t2;
};

#endif // _FRSKY_SPORT_SENSOR_RPM_H_
