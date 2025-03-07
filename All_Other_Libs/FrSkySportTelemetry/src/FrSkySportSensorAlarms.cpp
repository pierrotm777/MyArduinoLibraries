/*
  FrSky 8 ALARMS sensor class for Teensy LC/3.x/4.x, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) pierrotm777 20230217
  Not for commercial use
*/

#include "FrSkySportSensorAlarms.h" 

FrSkySportSensorAlarms::FrSkySportSensorAlarms(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorAlarms::setData(uint8_t alarm1, uint8_t alarm2, uint8_t alarm3, uint8_t alarm4, uint8_t alarm5, uint8_t alarm6, uint8_t alarm7, uint8_t alarm8)
{
  alarmData1 = alarm1;
  alarmData2 = alarm2;
  alarmData3 = alarm3;
  alarmData4 = alarm4;
  alarmData5 = alarm5;
  alarmData6 = alarm6;
  alarmData7 = alarm7;
  alarmData8 = alarm8;
}

uint16_t FrSkySportSensorAlarms::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  uint16_t dataId = SENSOR_NO_DATA_ID;
  if(sensorId == id)
  {
    switch(sensorDataIdx)
    {
      case 0:
        sendSingleData(serial, ALARMS_NOTIF1_DATA_ID, dataId, alarmData1, ALARMS_NOTIF1_DATA_PERIOD, alarmDataTime1, now);
        break;
      case 1:
        sendSingleData(serial, ALARMS_NOTIF2_DATA_ID, dataId, alarmData2, ALARMS_NOTIF2_DATA_PERIOD, alarmDataTime2, now);
        break;
      case 2:
        sendSingleData(serial, ALARMS_NOTIF3_DATA_ID, dataId, alarmData3, ALARMS_NOTIF3_DATA_PERIOD, alarmDataTime3, now);
        break;
      case 3:
        sendSingleData(serial, ALARMS_NOTIF4_DATA_ID, dataId, alarmData4, ALARMS_NOTIF4_DATA_PERIOD, alarmDataTime4, now);
        break;
      case 4:
        sendSingleData(serial, ALARMS_NOTIF5_DATA_ID, dataId, alarmData5, ALARMS_NOTIF5_DATA_PERIOD, alarmDataTime5, now);
        break;
      case 5:
        sendSingleData(serial, ALARMS_NOTIF6_DATA_ID, dataId, alarmData6, ALARMS_NOTIF6_DATA_PERIOD, alarmDataTime6, now);
        break;
      case 6:
        sendSingleData(serial, ALARMS_NOTIF7_DATA_ID, dataId, alarmData7, ALARMS_NOTIF7_DATA_PERIOD, alarmDataTime7, now);
        break;
      case 7:
        sendSingleData(serial, ALARMS_NOTIF8_DATA_ID, dataId, alarmData8, ALARMS_NOTIF8_DATA_PERIOD, alarmDataTime8, now);
        break;                               
    }
    sensorDataIdx++;
    if(sensorDataIdx >= ALARMS_DATA_COUNT) sensorDataIdx = 0;
  }
  return dataId;
}

uint16_t FrSkySportSensorAlarms::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {	  
      case ALARMS_NOTIF1_DATA_ID:
        data1 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF2_DATA_ID:
        data2 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF3_DATA_ID:
        data3 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF4_DATA_ID:
        data4 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF5_DATA_ID:
        data5 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF6_DATA_ID:
        data6 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF7_DATA_ID:
        data7 = (uint8_t)data;
        return appId;
      case ALARMS_NOTIF8_DATA_ID:
        data8 = (uint8_t)data;
        return appId;
	}
  }
  return SENSOR_NO_DATA_ID;
}

uint8_t FrSkySportSensorAlarms::getAlarm1() { return alarm[0]; }
uint8_t FrSkySportSensorAlarms::getAlarm2() { return alarm[1]; }
uint8_t FrSkySportSensorAlarms::getAlarm3() { return alarm[2]; }
uint8_t FrSkySportSensorAlarms::getAlarm4() { return alarm[3]; }
uint8_t FrSkySportSensorAlarms::getAlarm5() { return alarm[4]; }
uint8_t FrSkySportSensorAlarms::getAlarm6() { return alarm[5]; }
uint8_t FrSkySportSensorAlarms::getAlarm7() { return alarm[6]; }
uint8_t FrSkySportSensorAlarms::getAlarm8() { return alarm[7]; }