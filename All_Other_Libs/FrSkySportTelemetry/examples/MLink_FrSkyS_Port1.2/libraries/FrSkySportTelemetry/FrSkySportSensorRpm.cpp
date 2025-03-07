/*
  FrSky RPM sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#include "FrSkySportSensorRpm.h" 

FrSkySportSensorRpm::FrSkySportSensorRpm(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorRpm::setData(uint32_t rpm)
{
  send_b=true;
  rpmData = rpm;

}

void FrSkySportSensorRpm::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
        if(now > rpmTime)
        {
          rpmTime = now + RPM_ROT_DATA_PERIOD;
          serial.sendData(RPM_ROT_DATA_ID, rpmData);
        }
        else
        {
          serial.sendEmpty(RPM_ROT_DATA_ID);
        }
		send_b=false;   

  }
}

uint16_t FrSkySportSensorRpm::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    if(appId = RPM_ROT_DATA_ID)
	{
        rpm = (uint32_t)(data);
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

uint32_t FrSkySportSensorRpm::getRpm() { return rpm; }

