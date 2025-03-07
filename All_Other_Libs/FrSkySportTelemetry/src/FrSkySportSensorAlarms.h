/*
  FrSky 8 ALARMS sensor class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) pierrotm777 20230217
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_ALARMS_H_
#define _FRSKY_SPORT_SENSOR_ALARMS_H_

#include "FrSkySportSensor.h"

#define ALARMS_DEFAULT_ID ID25
#define ALARMS_DATA_COUNT 8
#define ALARMS_NOTIF1_DATA_ID 0x5131
#define ALARMS_NOTIF2_DATA_ID 0x5132
#define ALARMS_NOTIF3_DATA_ID 0x5133
#define ALARMS_NOTIF4_DATA_ID 0x5134
#define ALARMS_NOTIF5_DATA_ID 0x5135
#define ALARMS_NOTIF6_DATA_ID 0x5136
#define ALARMS_NOTIF7_DATA_ID 0x5137
#define ALARMS_NOTIF8_DATA_ID 0x5138

#define ALARMS_NOTIF1_DATA_PERIOD 500
#define ALARMS_NOTIF2_DATA_PERIOD 500
#define ALARMS_NOTIF3_DATA_PERIOD 500
#define ALARMS_NOTIF4_DATA_PERIOD 500
#define ALARMS_NOTIF5_DATA_PERIOD 500
#define ALARMS_NOTIF6_DATA_PERIOD 500
#define ALARMS_NOTIF7_DATA_PERIOD 500
#define ALARMS_NOTIF8_DATA_PERIOD 500

class FrSkySportSensorAlarms : public FrSkySportSensor
{
  public:
    FrSkySportSensorAlarms(SensorId id = ALARMS_DEFAULT_ID);
    void setData(uint8_t alarm1, uint8_t alarm2 = 0, uint8_t alarm3 = 0, uint8_t alarm4 = 0, uint8_t alarm5 = 0, uint8_t alarm6 = 0, uint8_t alarm7 = 0, uint8_t alarm8 = 0);
	virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    uint8_t getAlarm1();
    uint8_t getAlarm2();
    uint8_t getAlarm3();
    uint8_t getAlarm4();
    uint8_t getAlarm5();
    uint8_t getAlarm6();
    uint8_t getAlarm7();
    uint8_t getAlarm8();

	
  private:
    uint8_t alarmData1;
    uint8_t alarmData2;
    uint8_t alarmData3;
    uint8_t alarmData4;
    uint8_t alarmData5;
    uint8_t alarmData6;
	uint8_t alarmData7;
    uint8_t alarmData8;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
    uint8_t data6;
	uint8_t data7;
    uint8_t data8;	
    uint32_t alarmDataTime1;
	uint32_t alarmDataTime2;
	uint32_t alarmDataTime3;
	uint32_t alarmDataTime4;
	uint32_t alarmDataTime5;
	uint32_t alarmDataTime6;
	uint32_t alarmDataTime7;
	uint32_t alarmDataTime8;
	
    uint8_t alarm[6];

};

#endif // _FRSKY_SPORT_SENSOR_ALARMS_H_
