/*
RcPpmReader sketch
by RC Navy (http://p.loussouarn.free.fr/arduino/arduino.html) 2015
This sketch reads an RC PPM frame extracts the numbers of channels and their pulse witdhs.
This sketch can work with a Digispark pro, Digispark, Arduino UNO...
The PPM input shall support pin change interrupt.
This example code is in the public domain.
*/

#include <RculPPMReader.h>
#include <Rcul.h>

#define CPPM_INPUT_PIN  2

RculPPMReader RculPPMReader; // Object creation

void setup()
{
  Serial.begin(115200);
  RculPPMReader::attach(CPPM_INPUT_PIN); /* Attach TinyPpmReader to CPPM_INPUT_PIN pin */
}

void loop()
{
  //RculPPMReader::suspend(); /* Not needed if an hardware serial is used to display results */
  Serial.print(F("* Period="));Serial.print((int)RculPPMReader::cppmPeriod_us());Serial.println(F(" us *"));
  Serial.print(F("ChNb="));Serial.println((int)RculPPMReader::detectedChannelNb());
  for(uint8_t Idx = 1; Idx <= RculPPMReader::detectedChannelNb(); Idx++) /* From Channel 1 to Max detected */
  {
    Serial.print(F("Ch"));Serial.print(Idx);Serial.print(F("="));Serial.print(RculPPMReader::width_us(Idx));Serial.println(F(" us"));
  }
  //RculPPMReader::resume(); /* Not needed if an hardware serial is used to display results */
  delay(500);
}
