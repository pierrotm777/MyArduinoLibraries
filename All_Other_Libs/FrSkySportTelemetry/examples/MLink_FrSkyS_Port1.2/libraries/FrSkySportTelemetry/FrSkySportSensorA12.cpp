/*
  FrSky S.Port to UART Remote (Type B) converter class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
  
  Note that only analog ports ADC3 and ADC4 are implemented, not the UART part.
*/

#include "FrSkySportSensorA12.h" 

FrSkySportSensorA12::FrSkySportSensorA12(SensorId id) : FrSkySportSensor(id) { }

void FrSkySportSensorA12::setData(float adc1, float adc2)
{
  send_b=true;
  adc1Data = (uint32_t)(adc1*10);
  adc2Data = (uint32_t)(adc2*10);
}

void FrSkySportSensorA12::send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now)
{
  if(sensorId == id && send_b)
  {
    switch(sensorDataIdx)
    {
      case 0:
        if(now > adc1Time)
        {
          adc1Time = now + A12_ADC1_DATA_PERIOD;
          serial.sendData(A12_ADC1_DATA_ID, adc1Data);
        }
        else
        {
          serial.sendEmpty(A12_ADC1_DATA_ID);
        }
        break;
      case 1:
        if(now > adc2Time)
        {
          adc2Time = now + A12_ADC2_DATA_PERIOD;
          serial.sendData(A12_ADC2_DATA_ID, adc2Data);
        }
        else
        {
          serial.sendEmpty(A12_ADC2_DATA_ID);
        }
        break;
    }
    sensorDataIdx++;
    if(sensorDataIdx >= A12_DATA_COUNT) sensorDataIdx = 0;
	send_b=false;
  }
}

uint16_t FrSkySportSensorA12::decodeData(uint8_t id, uint16_t appId, uint32_t data)
{
  if((sensorId == id) || (sensorId == FrSkySportSensor::ID_IGNORE))
  {
    switch(appId)
    {
      case A12_ADC1_DATA_ID:
        adc1 = data/10;
        return appId;
      case A12_ADC2_DATA_ID:
        adc2 = data/10;
        return appId;
    }
  }
  return SENSOR_NO_DATA_ID;
}

float FrSkySportSensorA12::getAdc1() { return adc1; }
float FrSkySportSensorA12::getAdc2() { return adc2; }
