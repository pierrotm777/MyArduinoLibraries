/*
  FrSky ASS-70/ASS-100 airspeed sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorRaw.h" 

FrSkySportSensorRaw::FrSkySportSensorRaw(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorRaw::setData(float raw)
{
  send_b=true;
  rawData = (uint32_t)(raw);
}

void FrSkySportSensorRaw::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    if(now > rawTime)
    {
      rawTime = now + RAW_DATA_PERIOD;
      serial.sendData(RAW_DATA_ID, rawData);
    }
    else
    {
      serial.sendEmpty(RAW_DATA_ID);
    }
	send_b=false;
  }
}

uint16_t FrSkySportSensorRaw::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    if(appId == RAW_DATA_ID)
    {
      raw = data ;
      return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorRaw::getRaw() { return raw; }
