/*
  FrSky Gas Suite sensor class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210509
  Not for commercial use
*/

#include "FrSkySportSensor5000.h" 

FrSkySportSensor5000::FrSkySportSensor5000(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensor5000::setData(uint16_t s1, uint16_t s2, uint16_t s3, uint16_t s4, uint16_t s5, uint16_t s6, uint16_t s7, uint16_t s8)
{
  s1Data = s1;
  s2Data = s2;
  s3Data = s3;
  s4Data = s4;
  s5Data = s5;
  s6Data = s6;
  s7Data = s7;
  s8Data = s8;
}

uint16_t FrSkySportSensor5000::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  uint16_t dataId = SENSOR_NO_DATA_ID;
  if(sensorId == id)
  {
    switch(sensorDataIdx)
    {
      case 0:
        sendSingleData(serial, MY_5000_DATA_ID, dataId, s1Data, s5000_DATA_PERIOD, s1Time, now);
        break;
      case 1:
        sendSingleData(serial, MY_5010_DATA_ID, dataId, s2Data, s5000_DATA_PERIOD, s2Time, now);
        break;
      case 2:
        sendSingleData(serial, MY_5020_DATA_ID, dataId, s3Data, s5000_DATA_PERIOD, s3Time, now);
        break;
      case 3:
        sendSingleData(serial, MY_5030_DATA_ID, dataId, s4Data, s5000_DATA_PERIOD, s4Time, now);
        break;
      case 4:
        sendSingleData(serial, MY_5040_DATA_ID, dataId, s5Data, s5000_DATA_PERIOD, s5Time, now);
        break;
      case 5:
        sendSingleData(serial, MY_5050_DATA_ID, dataId, s6Data, s5000_DATA_PERIOD, s6Time, now);
        break;
      case 6:
        sendSingleData(serial, MY_5060_DATA_ID, dataId, s7Data, s5000_DATA_PERIOD, s7Time, now);
        break;
      case 7:
        sendSingleData(serial, MY_5070_DATA_ID, dataId, s8Data, s5000_DATA_PERIOD, s8Time, now);
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= MY_5000_DATA_COUNT) sensorDataIdx = 0;
  }
  return dataId;
}

uint16_t FrSkySportSensor5000::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case MY_5000_DATA_ID:
        s1 = (uint16_t)data;
        return appId;
      case MY_5010_DATA_ID:
        s2 = (uint16_t)data;
        return appId;
      case MY_5020_DATA_ID:
        s3 = (uint16_t)data;
        return appId;
      case MY_5030_DATA_ID:
        s4 = (uint16_t)data;
        return appId;
      case MY_5040_DATA_ID:
        s5 = (uint16_t)data;
        return appId;
      case MY_5050_DATA_ID:
        s6 = (uint16_t)data;
        return appId;
      case MY_5060_DATA_ID:
        s7 = (uint16_t)data;
        return appId;
      case MY_5070_DATA_ID:
        s8 = (uint16_t)data;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

uint16_t FrSkySportSensor5000::get5000() { return s1; }
uint16_t FrSkySportSensor5000::get5010() { return s2; }
uint16_t FrSkySportSensor5000::get5020() { return s3; }
uint16_t FrSkySportSensor5000::get5030() { return s4; }
uint16_t FrSkySportSensor5000::get5040() { return s5; }
uint16_t FrSkySportSensor5000::get5050() { return s6; }
uint16_t FrSkySportSensor5000::get5060() { return s7; }
uint16_t FrSkySportSensor5000::get5070() { return s8; }
