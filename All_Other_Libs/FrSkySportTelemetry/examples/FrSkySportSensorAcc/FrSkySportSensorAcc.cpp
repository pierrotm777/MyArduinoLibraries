/*
  Acceleration sensor class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
  Not for commercial use
  
  Note that FrSky does not yet provide this sensor so it is based on the specification only and may change in the future
*/

#include "FrSkySportSensorAcc.h" 

FrSkySportSensorAcc::FrSkySportSensorAcc(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorAcc::setData(float accX, float accY, float accZ)
{
  FrSkySportSensorAcc::x = (uint32_t)accX;
  FrSkySportSensorAcc::y = (uint32_t)accY;
  FrSkySportSensorAcc::z = (uint32_t)accZ;
}

void FrSkySportSensorAcc::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > xTime)
        {
          xTime = now + ACC_X_DATA_PERIOD;
          serial.sendData(ACC_X_DATA_ID, x);
        }
        else
        {
          serial.sendEmpty(ACC_X_DATA_ID);
        }
        break;
      case 1:
        if(now > yTime)
        {
          yTime = now + ACC_Y_DATA_PERIOD;
          serial.sendData(ACC_Y_DATA_ID, y);
        }
        else
        {
          serial.sendEmpty(ACC_Y_DATA_ID);
        }
        break;
      case 2:
        if(now > zTime)
        {
          zTime = now + ACC_Z_DATA_PERIOD;
          serial.sendData(ACC_Z_DATA_ID, z);
        }
        else
        {
          serial.sendEmpty(ACC_Z_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= ACC_DATA_COUNT) sensorDataIdx = 0;
  }
}
