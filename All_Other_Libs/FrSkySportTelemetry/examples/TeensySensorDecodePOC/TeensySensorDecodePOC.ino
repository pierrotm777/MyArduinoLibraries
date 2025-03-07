

#include "FrSkySportSensor.h"
//#include "StabilizerSensor.h"
#include "StabilizerSensor.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportDecoder.h"

StabilizerSensor stabilizerConfigSensor;
FrSkySportDecoder decoder;

#define LED 13 // Moteinos have LEDs on D9, nano 13
unsigned long timeBetweenBlinks = 600;
unsigned long timeOfLastBlink;
bool ledOn = true;

unsigned long loopCount = 0;
void blinkLED()
{
  if (millis() - timeOfLastBlink > timeBetweenBlinks)
  {
    ledOn = !ledOn;
    digitalWrite(LED, ledOn);
    timeOfLastBlink = millis();
  }
}

void setup()
{
#if defined(TEENSY_HW)
  decoder.begin(FrSkySportSingleWireSerial::SERIAL_1, &stabilizerConfigSensor);
#else
  decoder.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_12, &stabilizerConfigSensor);
#endif

  pinMode(LED, OUTPUT);
  timeOfLastBlink = millis();

  Serial.begin(115200);
}

void loop()
{
  blinkLED();

  decoder.decode();

  Serial.print(loopCount);
  Serial.print(": ");
  Serial.println(stabilizerConfigSensor.getConfig());

  loopCount++;
}