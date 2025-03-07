/*
  FrSky Variometer (high precision) sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorVario.h" 

FrSkySportSensorVario::FrSkySportSensorVario(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorVario::setData(float altitude, float vsi)
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

void FrSkySportSensorVario::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > altitudeTime)
        {
          altitudeTime = now + VARIO_ALT_DATA_PERIOD;
          serial.sendData(VARIO_ALT_DATA_ID, altitudeData);
        }
        else
        {
          serial.sendEmpty(VARIO_ALT_DATA_ID);
        }
        break;
      case 1:
        if(now > vsiTime)
        {
          vsiTime = now + VARIO_VSI_DATA_PERIOD;
          serial.sendData(VARIO_VSI_DATA_ID, vsiData);
        }
        else
        {
          serial.sendEmpty(VARIO_VSI_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= VARIO_DATA_COUNT) sensorDataIdx = 0;
	send_b=false;
  }
}

uint16_t FrSkySportSensorVario::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case VARIO_ALT_DATA_ID:
        altitude = ((int32_t)data) / 100.0;
        return appId;
      case VARIO_VSI_DATA_ID:
        vsi = ((int32_t)data) / 100.0;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorVario::getAltitude() { return altitude; }
float FrSkySportSensorVario::getVsi() { return vsi; }
