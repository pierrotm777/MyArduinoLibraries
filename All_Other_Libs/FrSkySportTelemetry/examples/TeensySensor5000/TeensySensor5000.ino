

#include "FrSkySportSensor.h"
#include "FrSkySportSensor5000.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"

FrSkySportSensor5000 my5000;
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


  telemetry.begin(FrSkySportSingleWireSerial::SERIAL_3, &my5000);

  pinMode(LED, OUTPUT);

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

  my5000.setData(1,0,1,0,1,0,1,1);

  telemetry.send();

  loopCount++;
}
