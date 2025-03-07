/*
  FrSky sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorDistance.h"

FrSkySportSensorDistance::FrSkySportSensorDistance(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorDistance::setData(float altitude, float vsi)
{
  send_b=true;
  if(altitude < -500.0)
	altitudeData = 50000;
  else if(altitude > 9000.0) 	
    altitudeData = 900000;  
  else	
	altitudeData = (int32_t)(altitude * 100.0);
	
  vsiData = (int32_t)(vsi * 100.0);
}

void FrSkySportSensorDistance::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > altitudeTime)
        {
          altitudeTime = now + DISTANCE_ALT_DATA_PERIOD;
          serial.sendData(DISTANCE_ALT_DATA_ID, altitudeData);
        }
        else
        {
          serial.sendEmpty(DISTANCE_ALT_DATA_ID);
        }
        break;
      case 1:
        if(now > vsiTime)
        {
          vsiTime = now + DISTANCE_DIS_DATA_PERIOD;
          serial.sendData(DISTANCE_DIS_DATA_ID, vsiData);
        }
        else
        {
          serial.sendEmpty(DISTANCE_DIS_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= DISTANCE_DATA_COUNT) sensorDataIdx = 0;
	send_b=false;
  }
}

uint16_t FrSkySportSensorDistance::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case DISTANCE_ALT_DATA_ID:
        altitude = ((int32_t)data) / 100.0;
        return appId;
      case DISTANCE_DIS_DATA_ID:
        vsi = ((int32_t)data) / 100.0;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorDistance::getAltitude() { return altitude; }
float FrSkySportSensorDistance::getVsi() { return vsi; }
