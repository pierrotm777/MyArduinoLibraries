

#include "FrSkySportSensor.h"
//Note: The value #define RPM_ROT_DATA_PERIOD  in FrSkySportSensorRPM.c
//has been changed from 500 to 100 to allow more frequent updates.
#include "StabilizerSensor.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"
// Demo the quad alphanumeric display LED backpack kit
// scrolls through every character, then scrolls Serial
// input onto the display
StabilizerSensor stabilizerConfigSensor;
FrSkySportTelemetry telemetry;

#define LED 13 // Moteinos have LEDs on D9, nano 13
unsigned long timeBetweenBlinks = 500;
unsigned long timeOfLastBlink;
bool ledOn = true;

unsigned long loopCount = 0;
unsigned long blinkCount = 0;

float demoData;

void blinkLED()
{
  if (millis() - timeOfLastBlink > timeBetweenBlinks)
  {
    ledOn = !ledOn;
    digitalWrite(LED, ledOn);
    timeOfLastBlink = millis();
    blinkCount++;
  }
}
void setup()
{

#if defined(TEENSY_HW)
  telemetry.begin(FrSkySportSingleWireSerial::SERIAL_1, &stabilizerConfigSensor);
#else
  telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_12, &stabilizerConfigSensor);
#endif

  pinMode(LED, OUTPUT);

  stabilizerConfigSensor.setData(1.1);

  unsigned long timeAtVersionSendStart = millis();

  while (millis() - timeAtVersionSendStart < 1000)
  {
    telemetry.send();
  }

  timeOfLastBlink = millis();
}

void loop()
{
  blinkLED();

  demoData = blinkCount + .1;

  stabilizerConfigSensor.setData(demoData);

  telemetry.send();

  loopCount++;
}
