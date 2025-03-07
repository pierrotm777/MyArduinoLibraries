/*
  Data transfer class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20151204
  and Andreas
  Not for commercial use
  
*/

#ifndef _FRSKY_SPORT_SENSOR_TRANSFERTX_H_
#define _FRSKY_SPORT_SENSOR_TRANSFERTX_H_

#include "FrSkySportSensor.h"

#define TRANSFERTX_DEFAULT_ID ID20
#define TRANSFERTX_DATA_COUNT 2
#define TRANSFERTX_txd1_DATA_ID 0x1900
#define TRANSFERTX_txd2_DATA_ID 0x1910

#define TRANSFERTX_txd1_DATA_PERIOD 500
#define TRANSFERTX_txd2_DATA_PERIOD 500

class FrSkySportSensorTransferTX : public FrSkySportSensor
{
  public:
    FrSkySportSensorTransferTX(SensorId id = TRANSFERTX_DEFAULT_ID);
    void setData(uint32_t txd1, uint32_t txd2);
    virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    uint32_t gettxd1();
    uint32_t gettxd2();

  private:
    uint32_t txd1Data;
    uint32_t txd2Data;
    uint32_t txd1Time;
    uint32_t txd2Time;
    uint32_t txd1;
    uint32_t txd2;
};

#endif // _FRSKY_SPORT_SENSOR_TRANSFERTX_H_
