/*
  FrSky ASS-70/ASS-100 airspeed sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorRxBt.h" 

FrSkySportSensorRxBt::FrSkySportSensorRxBt(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorRxBt::setData(float rxbt)
{
  send_b=true;
  rxbtData = (uint32_t)(rxbt);
}

void FrSkySportSensorRxBt::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    if(now > rxbtTime)
    {
      rxbtTime = now + RXBT_DATA_PERIOD;
      serial.sendData(RXBT_DATA_ID, rxbtData);
    }
    else
    {
      serial.sendEmpty(RXBT_DATA_ID);
    }
	send_b=false;
  }
}

uint16_t FrSkySportSensorRxBt::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    if(appId == RXBT_DATA_ID)
    {
      rxbt = data ;
      return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorRxBt::getRxBt() { return rxbt; }
