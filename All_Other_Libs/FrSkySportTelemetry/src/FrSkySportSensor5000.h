/*
  FrSky Gas Suite sensor class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20210227
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_5000_H_
#define _FRSKY_SPORT_SENSOR_5000_H_

#include "FrSkySportSensor.h"

#define MY_5000_DEFAULT_ID	ID12
#define MY_5000_DATA_COUNT	8
#define MY_5000_DATA_ID		0x5100
#define MY_5010_DATA_ID		0x5101
#define MY_5020_DATA_ID		0x5102
#define MY_5030_DATA_ID		0x5103
#define MY_5040_DATA_ID		0x5104
#define MY_5050_DATA_ID		0x5105
#define MY_5060_DATA_ID		0x5106
#define MY_5070_DATA_ID		0x5107

#define s5000_DATA_PERIOD	100
#define s5010_DATA_PERIOD	100
#define s5020_DATA_PERIOD	100
#define s5030_DATA_PERIOD	100
#define s5040_DATA_PERIOD	100
#define s5050_DATA_PERIOD	100
#define s5060_DATA_PERIOD	100
#define s5070_DATA_PERIOD	100

class FrSkySportSensor5000 : public FrSkySportSensor
{
  public:
    FrSkySportSensor5000(SensorId id = MY_5000_DEFAULT_ID);
    void setData(uint16_t s1, uint16_t s2, uint16_t s3, uint16_t s4, uint16_t s5, uint16_t s6, uint16_t s7, uint16_t s8);
    virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    uint16_t get5000();
	uint16_t get5010();
	uint16_t get5020();
	uint16_t get5030();
	uint16_t get5040();
	uint16_t get5050();
	uint16_t get5060();
	uint16_t get5070();

  private:
    uint16_t s1Data;
    uint16_t s2Data;
    uint16_t s3Data;
    uint16_t s4Data;
    uint16_t s5Data;
    uint16_t s6Data;
    uint16_t s7Data;
    uint16_t s8Data;
	uint32_t s1Time;
	uint32_t s2Time;
	uint32_t s3Time;
	uint32_t s4Time;
	uint32_t s5Time;
	uint32_t s6Time;
	uint32_t s7Time;
	uint32_t s8Time;
	uint16_t s1;
	uint16_t s2;
	uint16_t s3;
	uint16_t s4;
	uint16_t s5; 
	uint16_t s6; 
	uint16_t s7; 
	uint16_t s8;

};

#endif // _FRSKY_SPORT_SENSOR_5000_H_
