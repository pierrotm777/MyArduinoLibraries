/*
  FrSky HDG sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_HDG_H_
#define _FRSKY_SPORT_SENSOR_HDG_H_

#include "FrSkySportSensor.h"

#define HDG_DEFAULT_ID ID15
#define HDG_DATA_ID 0x0840

#define HDG_DATA_PERIOD 500

class FrSkySportSensorHdg : public FrSkySportSensor
{
  public:
    FrSkySportSensorHdg(SensorId id = HDG_DEFAULT_ID);
    void setData(float hdg);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    float getHdg();

  private:
    uint32_t hdgData;
    uint32_t hdgTime;
    float hdg;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_ASS_H_
