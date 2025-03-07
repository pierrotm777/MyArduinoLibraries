/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorRpmT12.h" 

FrSkySportSensorRpmT12::FrSkySportSensorRpmT12(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorRpmT12::setData(uint32_t rpm, float t1, float t2)
{
  rpmData = rpm;
  t1Data = (int32_t)round(t1);
  t2Data = (int32_t)round(t2);
}

void FrSkySportSensorRpmT12::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > t1Time)
        {
          t1Time = now + RPMT12_T1_DATA_PERIOD;
          serial.sendData(RPMT12_T1_DATA_ID, t1Data);
        }
        else
        {
          serial.sendEmpty(RPMT12_T1_DATA_ID);
        }
        break;
      case 1:
        if(now > t2Time)
        {
          t2Time = now + RPMT12_T2_DATA_PERIOD;
          serial.sendData(RPMT12_T2_DATA_ID, t2Data);
        }
        else
        {
          serial.sendEmpty(RPMT12_T2_DATA_ID);
        }
        break;
      case 2:
        if(now > rpmTime)
        {
          rpmTime = now + RPMT12_ROT_DATA_PERIOD;
          serial.sendData(RPMT12_ROT_DATA_ID, rpmData);
        }
        else
        {
          serial.sendEmpty(RPMT12_ROT_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= RPMT12_DATA_COUNT) sensorDataIdx = 0;
  }
}

uint16_t FrSkySportSensorRpmT12::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case RPMT12_T1_DATA_ID:
        t1 = (int32_t)data;
        return appId;
      case RPMT12_T2_DATA_ID:
        t2 = (int32_t)data;
        return appId;
      case RPMT12_ROT_DATA_ID:
        rpm = (uint32_t)(data);
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

uint32_t FrSkySportSensorRpmT12::getRpm() { return rpm; }
int32_t FrSkySportSensorRpmT12::getT1() { return t1; }
int32_t FrSkySportSensorRpmT12::getT2() { return t2; }
