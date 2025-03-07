/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_RPM_H_
#define _FRSKY_SPORT_SENSOR_RPM_H_

#include "FrSkySportSensor.h"

#define RPM_DEFAULT_ID ID5
#define RPM_DATA_COUNT 3

#define RPM_ROT_DATA_ID 0x0500
#define RPM_ROT_DATA_PERIOD 500

class FrSkySportSensorRpm : public FrSkySportSensor
{
  public:
    FrSkySportSensorRpm(SensorId id = RPM_DEFAULT_ID);
    void setData(uint32_t rpm);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    uint32_t getRpm();


  private:
    uint32_t rpmData;
    uint32_t rpmTime;
    uint32_t rpm;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_RPM_H_
