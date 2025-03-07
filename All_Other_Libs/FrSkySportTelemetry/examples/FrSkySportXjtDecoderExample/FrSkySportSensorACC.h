/*
FrSky FCS-40A/FCS-150A current sensor class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
(c) Pawelsky 20151108
Not for commercial use

Modified for ACC output 7.7.2016 Alexander Derks

*/

#ifndef _FRSKY_SPORT_SENSOR_ACC_H_
#define _FRSKY_SPORT_SENSOR_ACC_H_

#include "FrSkySportSensor.h"

#define ACC_DEFAULT_ID ID28 // ID3 is the default for FCS-40A sensor, for FCS-150A use ID8.
#define ACC_DATA_COUNT 3
#define ACC_ACCX_DATA_ID 0x0700
#define ACC_ACCY_DATA_ID 0x0710
#define ACC_ACCZ_DATA_ID 0x0720

#define ACC_ACCX_DATA_PERIOD 500
#define ACC_ACCY_DATA_PERIOD 500
#define ACC_ACCZ_DATA_PERIOD 500


class FrSkySportSensorACC : public FrSkySportSensor
{
public:
	FrSkySportSensorACC(SensorId id = ACC_DEFAULT_ID);
	void setData(float ACCX, float ACCY, float ACCZ);
	virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
	
private:
	uint32_t ACCXData;
	uint32_t ACCYData;
	uint32_t ACCZData;
	uint32_t ACCXTime;
	uint32_t ACCYTime;
	uint32_t ACCZTime;
	float ACCX;
	float ACCY;
	float ACCZ;
};




#endif // _FRSKY_SPORT_SENSOR_ACC_H_