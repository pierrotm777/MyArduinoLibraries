/*
RcPpmReader sketch
by RC Navy (http://p.loussouarn.free.fr/arduino/arduino.html) 2015
This sketch reads an RC PPM frame extracts the numbers of channels and their pulse witdhs.
This sketch can work with a Digispark pro, Digispark, Arduino UNO...
The PPM input shall support pin change interrupt.
This example code is in the public domain.
*/

#include <PPMReader.h>
#include <Rcul.h>

#define CPPM_INPUT_PIN  2

PPMReader PPMReader; // Object creation

void setup()
{
  Serial.begin(115200);
  PPMReader::attach(CPPM_INPUT_PIN); /* Attach TinyPpmReader to CPPM_INPUT_PIN pin */
}

void loop()
{
  //PPMReader::suspend(); /* Not needed if an hardware serial is used to display results */
  Serial.print(F("* Period="));Serial.print((int)PPMReader::cppmPeriod_us());Serial.println(F(" us *"));
  Serial.print(F("ChNb="));Serial.println((int)PPMReader::detectedChannelNb());
  for(uint8_t Idx = 1; Idx <= PPMReader::detectedChannelNb(); Idx++) /* From Channel 1 to Max detected */
  {
    Serial.print(F("Ch"));Serial.print(Idx);Serial.print(F("="));Serial.print(PPMReader::width_us(Idx));Serial.println(F(" us"));
  }
  //PPMReader::resume(); /* Not needed if an hardware serial is used to display results */
  delay(500);
}
