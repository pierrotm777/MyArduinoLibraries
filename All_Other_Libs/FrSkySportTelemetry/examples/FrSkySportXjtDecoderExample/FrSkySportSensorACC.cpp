/*
FrSky FCS-40A/FCS-150A current sensor class for Teensy 3.x and 328P based boards (e.g. Pro Mini, Nano, Uno)
(c) Pawelsky 20151018
Not for commercial use

Modified for ACC output 7.7.2016 Alexander Derks
*/

#include "FrSkySportSensorACC.h" 

FrSkySportSensorACC::FrSkySportSensorACC(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorACC::setData(float ACCX, float ACCY, float ACCZ)
{
	ACCXData = (uint32_t)(ACCX/ 100);
	ACCYData = (uint32_t)(ACCY/ 100);
	ACCZData = (uint32_t)(ACCZ/ 100);
}

void FrSkySportSensorACC::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
	if (sensorId == id)
	{
		switch (sensorDataIdx)
		{
		case 0:
			if (now > ACCXTime)
			{
				ACCXTime = now + ACC_ACCX_DATA_PERIOD;
				serial.sendData(ACC_ACCX_DATA_ID, ACCXData);
			}
			else
			{
				serial.sendEmpty(ACC_ACCX_DATA_ID);
			}
			break;
		case 1:
			if (now > ACCYTime)
			{
				ACCYTime = now + ACC_ACCY_DATA_PERIOD;
				serial.sendData(ACC_ACCY_DATA_ID, ACCYData);
			}
			else
			{
				serial.sendEmpty(ACC_ACCY_DATA_ID);
			}
		case 2:
			if (now > ACCZTime)
			{
				ACCZTime = now + ACC_ACCZ_DATA_PERIOD;
				serial.sendData(ACC_ACCZ_DATA_ID, ACCZData);
			}
			else
			{
				serial.sendEmpty(ACC_ACCZ_DATA_ID);
			}
			break;
		}
		sensorDataIdx++;
		if (sensorDataIdx >= ACC_DATA_COUNT) sensorDataIdx = 0;
	}
}


