/*
  FrSky Variometer (high precision) sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _FRSKY_SPORT_SENSOR_VARIO_H_
#define _FRSKY_SPORT_SENSOR_VARIO_H_

#include "FrSkySportSensor.h"

#define VARIO_DEFAULT_ID ID1
#define VARIO_DATA_COUNT 2
#define VARIO_ALT_DATA_ID 0x0100
#define VARIO_VSI_DATA_ID 0x0110

#define VARIO_ALT_DATA_PERIOD 1000 // Aktualisierungsintervall der barometrischen Höhe aus dem Vario in [ms]
#define VARIO_VSI_DATA_PERIOD 1 // Aktualisierungsintervall des Variowertes [ms] -> sooft wie möglich (unterschiedliche Werte führen zur SendEmpty)

class FrSkySportSensorVario : public FrSkySportSensor
{
  public:
    FrSkySportSensorVario(SensorId id = VARIO_DEFAULT_ID);
    void setData(float altitude, float vsi);
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
    float getAltitude();
    float getVsi();

  private:
    int32_t altitudeData;
    int32_t vsiData;
    uint32_t altitudeTime;
    uint32_t vsiTime;
    float altitude;
    float vsi;
	bool send_b;
};

#endif // _FRSKY_SPORT_SENSOR_VARIO_H_
