/*
  FrSky ASS-70/ASS-100 airspeed sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorAss.h" 

FrSkySportSensorAss::FrSkySportSensorAss(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorAss::setData(float speed, bool unit)
{
send_b=true;

  speedData = (uint32_t)(speed);
}

void FrSkySportSensorAss::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    if(now > speedTime)
    {
      speedTime = now + ASS_SPEED_DATA_PERIOD;
      serial.sendData(ASS_SPEED_DATA_ID, speedData);
    }
    else
    {
      serial.sendEmpty(ASS_SPEED_DATA_ID);
    }
	send_b=false;
  }
}

uint16_t FrSkySportSensorAss::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    if(appId == ASS_SPEED_DATA_ID)
    {
      speed = data / 10.0;
      return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorAss::getSpeed() { return speed; }
