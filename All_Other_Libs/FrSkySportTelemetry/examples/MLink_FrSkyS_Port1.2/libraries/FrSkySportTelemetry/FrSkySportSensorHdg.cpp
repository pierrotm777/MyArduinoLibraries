/*
  FrSky ASS-70/ASS-100 airspeed sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorHdg.h" 

FrSkySportSensorHdg::FrSkySportSensorHdg(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorHdg::setData(float hdg)
{
  send_b=true;
  if(hdg < 360.0 && hdg >= 0.0)
	hdgData = (uint32_t)(hdg * 100.0);
  else	if(hdg == 360.0)
    hdgData = 0;
  else
    send_b=false;  
}

void FrSkySportSensorHdg::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    if(now > hdgTime)
    {
      hdgTime = now + HDG_DATA_PERIOD;
      serial.sendData(HDG_DATA_ID, hdgData);
    }
    else
    {
      serial.sendEmpty(HDG_DATA_ID);
    }
	send_b=false;
  }
}

uint16_t FrSkySportSensorHdg::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    if(appId == HDG_DATA_ID)
    {
      hdg = data ;
      return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorHdg::getHdg() { return hdg; }
