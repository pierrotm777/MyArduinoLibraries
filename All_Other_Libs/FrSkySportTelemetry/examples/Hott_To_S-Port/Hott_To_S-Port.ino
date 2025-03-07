#include "HoTTClient.h"
#include "FrSkySportSensor.h"
#include "FrSkySportSensorFcs.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"
#include "SoftwareSerial.h"

HoTTClient sensor; //Initialize Hott Sensor
uint8_t sensorType = 0;

FrSkySportSensorFcs fcs; // Create FCS-40A sensor with default ID (use ID8 for FCS-150A)
FrSkySportTelemetry telemetry; // Create telemetry object without polling

#define SPORT_PIN FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_4


void setup() 
{
	Serial.begin(19200);
	Serial.println("HoTT-Test");
	Serial.println("Debug connection established.");

	pinMode(LED_BUILTIN, OUTPUT); // initialize digital pin LED_BUILTIN as an output.

	delay(3000); // Wait for esc's to initialize ... if I comment it, nothing changes

	sensor.start();

	// Try to find a resonding sensor
	while (sensorType == 0) 
	{
		digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
		Serial.println("Probing..."); // only debug

		if (sensor.probe(HOTT_AIRESC_MODULE_ID)) 
    {
		sensorType = HOTT_AIRESC_MODULE_ID;
		Serial.println("HoTT ESC found."); // only debug
		}
		else if (sensor.probe(HOTT_ELECTRIC_AIR_MODULE_ID)) 
    {
		sensorType = HOTT_ELECTRIC_AIR_MODULE_ID;
		Serial.println("HoTT EAM found.");// only debug
		}
		else if (sensor.probe(HOTT_GENERAL_AIR_MODULE_ID)) 
    {
		Serial.println("HoTT GAMfound.");// only debug
		sensorType = HOTT_GENERAL_AIR_MODULE_ID;
		}
		else if (sensor.probe(HOTT_GPS_MODULE_ID)) 
    {
		Serial.println("HoTT GPS found.");// only debug
		sensorType = HOTT_GPS_MODULE_ID;
		}
		else if (sensor.probe(HOTT_VARIO_MODULE_ID)) 
    {
		Serial.println("HoTT VARIO found.");// only debug
		sensorType = HOTT_VARIO_MODULE_ID;
		}
	}
	telemetry.begin(SPORT_PIN, &fcs);
}

void loop() 
{
	fcs.setData(5.0, sensor.mainVoltage); //the first value is sent correctly, the second is not, why?
	if (sensor.poll(sensorType)) 
	{
		switch (sensorType) 
		{
			case HOTT_GENERAL_AIR_MODULE_ID:
			Serial.println(sensor.mainVoltage); //It is correctly displayed on the serial monitor
		}
	}
	telemetry.send();
} 