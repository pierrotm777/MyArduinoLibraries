/*
  FrSky ASS-70/ASS-100 airspeed sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorFuel.h" 

FrSkySportSensorFuel::FrSkySportSensorFuel(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorFuel::setData(float fuel)
{
  send_b=true;
  fuelData = (uint32_t)(fuel);
}

void FrSkySportSensorFuel::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    if(now > fuelTime)
    {
      fuelTime = now + FUEL_DATA_PERIOD;
      serial.sendData(FUEL_DATA_ID, fuelData);
    }
    else
    {
      serial.sendEmpty(FUEL_DATA_ID);
    }
	send_b=false;
  }
}

uint16_t FrSkySportSensorFuel::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    if(appId == FUEL_DATA_ID)
    {
      fuel = data ;
      return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorFuel::getFuel() { return fuel; }
