/*
  FrSky ACC sensor class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Clooney 20150814
  Not for commercial use
*/

#include "FrSkySportSensorAcc.h" 

FrSkySportSensorAcc::FrSkySportSensorAcc(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorAcc::setData(float AccX, float AccY, float AccZ)
{
  FrSkySportSensorAcc::AccX = (int32_t)AccX;
  FrSkySportSensorAcc::AccY = (int32_t)AccY;
  FrSkySportSensorAcc::AccZ = (int32_t)AccZ;
}

uint16_t FrSkySportSensorAcc::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  uint16_t dataId = SENSOR_NO_DATA_ID;
  if(sensorId == id)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > AccXTime)
        {
          AccXTime = now + AccX_DATA_PERIOD;
          serial.sendData(AccX_DATA_ID, AccX);
        }
        else
        {
          serial.sendEmpty(AccX_DATA_ID);
        }
        break;
      case 1:
        if(now > AccYTime)
        {
          AccYTime = now + AccY_DATA_PERIOD;
          serial.sendData(AccY_DATA_ID, AccY);
        }
        else
        {
          serial.sendEmpty(AccY_DATA_ID);
        }
        break;
      case 2:
        if(now > AccZTime)
        {
          AccZTime = now + AccZ_DATA_PERIOD;
          serial.sendData(AccZ_DATA_ID, AccZ);
        }
        else
        {
          serial.sendEmpty(AccZ_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= ACC_DATA_COUNT) sensorDataIdx = 0;
  }
  return dataId;
}

