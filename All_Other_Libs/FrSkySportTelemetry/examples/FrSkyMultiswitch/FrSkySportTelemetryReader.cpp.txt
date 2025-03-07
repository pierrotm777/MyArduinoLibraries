#include "FrSkySportTelemetryReader.h"

FrSkySportTelemetryReader::FrSkySportTelemetryReader() { }

void FrSkySportTelemetryReader::begin(FrSkySportSingleWireSerial::SerialId id,
                                FrSkySportSensor* sensor1) {
  FrSkySportTelemetryReader::serial.begin(id);
}

void FrSkySportTelemetryReader::receive() {
	if(serial.port != NULL) {
		if (serial.port->available()) {
			uint8_t data= serial.port->read();
			
			if (data == FRSKY_TELEMETRY_START_FRAME) {
				dataCounter = 0;
				available = false;
			}
			
			dataArray[dataCounter] = data;
			dataCounter++;
				
			if ((dataCounter == 10) && (dataArray[1] == 0x0D)) {
				aUiRxValue[0] = dataArray[5];
				aUiRxValue[1] = dataArray[6];
				aUiRxValue[2] = dataArray[7];
				aUiRxValue[3] = dataArray[8];
				
				available = true;
			}			
		}		
	}
}
