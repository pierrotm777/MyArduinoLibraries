#ifndef _FRSKY_SPORT_TELEMETRY_READER_H_
#define _FRSKY_SPORT_TELEMETRY_READER_H_

#include "Arduino.h"
#include "FrSkySportTelemetry.h"

class FrSkySportTelemetryReader
{
  public:
    FrSkySportTelemetryReader();
    void begin(FrSkySportSingleWireSerial::SerialId id,
                FrSkySportSensor* sensor1);
    void receive();

	uint8_t dataCounter = 0;
	bool available = false;
	uint8_t aUiRxValue[4];
		
  private:
    FrSkySportSingleWireSerial serial;
	
	uint8_t dataArray[10];
};

#endif // _FRSKY_SPORT_TELEMETRY_READER_H_
