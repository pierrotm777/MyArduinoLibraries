/*
  FrSky FCS-40A/FCS-150A current sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "StabilizerSensor.h"

StabilizerSensor::StabilizerSensor(SensorId id) : FrSkySportSensor(id) {}

void StabilizerSensor::setData(float config)
{
  configData = (uint32_t)(config * 10);
}

uint16_t StabilizerSensor::send(FrSkySportSingleWireSerial &serial, uint8_t id, uint32_t now)
{
  uint16_t dataId = SENSOR_NO_DATA_ID;
  if (sensorId == id)
  {
    switch (sensorDataIdx)
    {
    case 0:
      if (now > configTime)
      {
        configTime = now + STABILZER_CONFIG_DATA_PERIOD;
        serial.sendData(STABILIZER_CONFIG_DATA_ID, configData);
      }
      else
      {
        serial.sendEmpty(STABILIZER_CONFIG_DATA_ID);
      }
      break;
    }
    sensorDataIdx++;
    if (sensorDataIdx >= STABILIZER_DATA_COUNT)
      sensorDataIdx = 0;
  }
  return dataId;
}

uint16_t StabilizerSensor::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if ((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch (appId)
    {
    case STABILIZER_CONFIG_DATA_ID:
      config = data;
      return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float StabilizerSensor::getConfig() { return config; }
