/*
  FrSky HDG sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_RAW_H_
#define _FRSKY_SPORT_SENSOR_RAW_H_

#include "FrSkySportSensor.h"

#define RAW_DEFAULT_ID ID16
#define RAW_DATA_ID 0x0C00

#define RAW_DATA_PERIOD 1 // Aktualisierungsintervall des (Vario-)Wertes in [ms] -> so oft wie möglich

class FrSkySportSensorRaw : public FrSkySportSensor
{
  public:
    FrSkySportSensorRaw(SensorId id = RAW_DEFAULT_ID);
    void setData(float raw);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    float getRaw();

  private:
    uint32_t rawData;
    uint32_t rawTime;
    float raw;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_ASS_H_
