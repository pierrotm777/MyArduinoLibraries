/*
  Data transfer class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20151204
  and Andreas
  Not for commercial use
  
*/

#include "FrSkySportSensorTransferTX.h" 

FrSkySportSensorTransferTX::FrSkySportSensorTransferTX(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorTransferTX::setData(uint32_t txd1, uint32_t txd2)
{
  txd1Data = txd1;
  txd2Data = txd2;
}

uint16_t FrSkySportSensorTransferTX::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  uint16_t dataId = SENSOR_NO_DATA_ID;
  if(sensorId == id)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > txd1Time)
        {
          txd1Time = now + TRANSFERTX_txd1_DATA_PERIOD;
          serial.sendData(TRANSFERTX_txd1_DATA_ID, txd1Data);
        }
        else
        {
          serial.sendEmpty(TRANSFERTX_txd1_DATA_ID);
        }
        break;
      case 1:
        if(now > txd2Time)
        {
          txd2Time = now + TRANSFERTX_txd2_DATA_PERIOD;
          serial.sendData(TRANSFERTX_txd2_DATA_ID, txd2Data);
        }
        else
        {
          serial.sendEmpty(TRANSFERTX_txd2_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= TRANSFERTX_DATA_COUNT) sensorDataIdx = 0;
  }
  return dataId;
}

uint16_t FrSkySportSensorTransferTX::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case TRANSFERTX_txd1_DATA_ID:
        txd1 = data;
        return appId;
      case TRANSFERTX_txd2_DATA_ID:
        txd2 = data;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

uint32_t FrSkySportSensorTransferTX::gettxd1() { return txd1; }
uint32_t FrSkySportSensorTransferTX::gettxd2() { return txd2; }
