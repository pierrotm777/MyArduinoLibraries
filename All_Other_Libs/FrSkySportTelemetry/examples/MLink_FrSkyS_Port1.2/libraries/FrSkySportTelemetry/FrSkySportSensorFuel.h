/*
  FrSky ASS-70/ASS-100 airspeed sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_FUEL_H_
#define _FRSKY_SPORT_SENSOR_FUEL_H_

#include "FrSkySportSensor.h"

#define FUEL_DEFAULT_ID ID14
#define FUEL_DATA_ID 0x0600

#define FUEL_DATA_PERIOD 1000

class FrSkySportSensorFuel : public FrSkySportSensor
{
  public:
    FrSkySportSensorFuel(SensorId id = FUEL_DEFAULT_ID);
    void setData(float fuel);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    float getFuel();

  private:
    uint32_t fuelData;
    uint32_t fuelTime;
    float fuel;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_ASS_H_
