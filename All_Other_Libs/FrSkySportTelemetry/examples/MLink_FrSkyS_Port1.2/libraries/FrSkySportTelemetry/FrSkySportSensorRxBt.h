/*
  FrSky HDG sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_RXBT_H_
#define _FRSKY_SPORT_SENSOR_RXBT_H_

#include "FrSkySportSensor.h"

#define RXBT_DEFAULT_ID ID17
#define RXBT_DATA_ID 0xF104

#define RXBT_DATA_PERIOD 2000 // Aktualisierungsintervall des Wertes in [ms]

class FrSkySportSensorRxBt : public FrSkySportSensor
{
  public:
    FrSkySportSensorRxBt(SensorId id = RXBT_DEFAULT_ID);
    void setData(float rxbt);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    float getRxBt();

  private:
    uint32_t rxbtData;
    uint32_t rxbtTime;
    float rxbt;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_RXBT_H_
