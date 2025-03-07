/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorT12.h" 

FrSkySportSensorT12::FrSkySportSensorT12(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorT12::setData(float t1, float t2)
{
  send_b=true;
  t1Data = (int32_t)round(t1);
  t2Data = (int32_t)round(t2);
}

void FrSkySportSensorT12::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > t1Time)
        {
          t1Time = now + T12_T1_DATA_PERIOD;
          serial.sendData(T12_T1_DATA_ID, t1Data);
        }
        else
        {
          serial.sendEmpty(T12_T1_DATA_ID);
        }
        break;
      case 1:
        if(now > t2Time)
        {
          t2Time = now + T12_T2_DATA_PERIOD;
          serial.sendData(T12_T2_DATA_ID, t2Data);
        }
        else
        {
          serial.sendEmpty(T12_T2_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= T12_DATA_COUNT) sensorDataIdx = 0;
	send_b=false;
  }
}

uint16_t FrSkySportSensorT12::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case T12_T1_DATA_ID:
        t1 = (int32_t)data;
        return appId;
      case T12_T2_DATA_ID:
        t2 = (int32_t)data;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

int32_t FrSkySportSensorT12::getT1() { return t1; }
int32_t FrSkySportSensorT12::getT2() { return t2; }
