/*
  FrSky S.Port to UART Remote (Type B) converter class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
  
  Note that only analog ports ADC1 and ADC2 are implemented, not the UART part.
*/

#ifndef _FRSKY_SPORT_SENSOR_A12_H_
#define _FRSKY_SPORT_SENSOR_A12_H_

#include "FrSkySportSensor.h"

#define A12_DEFAULT_ID ID7
#define A12_DATA_COUNT 2
#define A12_ADC1_DATA_ID 0xF102
#define A12_ADC2_DATA_ID 0xF103

#define A12_ADC1_DATA_PERIOD 500
#define A12_ADC2_DATA_PERIOD 500

class FrSkySportSensorA12 : public FrSkySportSensor
{
  public:
    FrSkySportSensorA12(SensorId id = A12_DEFAULT_ID);
    void setData(float adc1, float adc2);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    float getAdc1();
    float getAdc2();

  private:
    uint32_t adc1Data;
    uint32_t adc2Data;
    uint32_t adc1Time;
    uint32_t adc2Time;
    float adc1;
    float adc2;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_A12_H_
