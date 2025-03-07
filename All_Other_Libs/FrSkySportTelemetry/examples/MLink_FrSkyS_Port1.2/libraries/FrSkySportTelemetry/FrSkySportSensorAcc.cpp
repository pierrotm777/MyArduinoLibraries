/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorAcc.h" 

FrSkySportSensorAcc::FrSkySportSensorAcc(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorAcc::setData(float x, float y, float z)
{
  send_b=true;
  xData = (int32_t)(x*100);
  yData = (int32_t)(y*100);
  zData = (int32_t)(z*100);
}

void FrSkySportSensorAcc::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > yTime)
        {
          yTime = now + ACC_Y_DATA_PERIOD;
          serial.sendData(ACC_Y_DATA_ID, yData);
        }
        else
        {
          serial.sendEmpty(ACC_Y_DATA_ID);
        }
        break;
      case 1:
        if(now > zTime)
        {
          zTime = now + ACC_Z_DATA_PERIOD;
          serial.sendData(ACC_Z_DATA_ID, zData);
        }
        else
        {
          serial.sendEmpty(ACC_Z_DATA_ID);
        }
        break;
      case 2:
        if(now > xTime)
        {
          xTime = now + ACC_X_DATA_PERIOD;
          serial.sendData(ACC_X_DATA_ID, xData);
        }
        else
        {
          serial.sendEmpty(ACC_X_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= ACC_DATA_COUNT) sensorDataIdx = 0;
	send_b=false;
  }
}

uint16_t FrSkySportSensorAcc::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case ACC_Y_DATA_ID:
        y = data / 100.0;
        return appId;
      case ACC_Z_DATA_ID:
        z = data / 100.0;
        return appId;
      case ACC_X_DATA_ID:
        x = data / 100.0;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

int32_t FrSkySportSensorAcc::getX() { return x; }
int32_t FrSkySportSensorAcc::getY() { return y; }
int32_t FrSkySportSensorAcc::getZ() { return z; }
