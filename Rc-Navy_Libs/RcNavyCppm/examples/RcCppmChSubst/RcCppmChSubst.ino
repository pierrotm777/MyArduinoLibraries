/*
RcPpmChSubst sketch
by RC Navy (http://p.loussouarn.free.fr/arduino/arduino.html) 2015
This sketch reads an RC CPPM frame, extracts the numbers of channels and their pulse witdhs and substitutes the 4th channel with a fix pulse width (2000 us).

.----------------.   .------------.
| Trainer Output |-->| CppmReader |
|                |   '------------'
|                |         |
| RC Transmitter |         | Forward channels 1 to 3 and set 4th channel width to 2000 us
|                |         V
|                |   .------------.
| Trainer Intput |<--|  CppmGen   |
'----------------'   '------------'

This sketch can work with an Arduino UNO.

PPM output pin is imposed by hardware and is target dependant: for Atmega328P, CppmGen pin is pin#9, CppmReader pin is pin#8

This example code is in the public domain.
*/
#include <Rcul.h>
#include <Cppm.h>

#define PPM_HEADER_US  300
#define PPM_PERIOD_US  20000

#define CH_MAX_NB      4

void setup()
{
  CppmGen.begin(CPPM_GEN_NEG_MOD, CH_MAX_NB, PPM_PERIOD_US, PPM_HEADER_US);/* Change CPPM_GEN_NEG_MOD to CPPM_GEN_POS_MOD for POSitive modulation */
}

void loop()
{
  if((CppmReader.detectedChannelNb() >= CH_MAX_NB) && CppmReader.isSynchro())
  {
    CppmGen.width_us(1, CppmReader.width_us(1)); /* RC Channel#1: forward rx value */
    CppmGen.width_us(2, CppmReader.width_us(2)); /* RC Channel#2: forward rx value */
    CppmGen.width_us(3, CppmReader.width_us(3)); /* RC Channel#3 forward rx value: */
    CppmGen.width_us(4, 2000); /* RC Channel#4: replace rx pulse width with 2000 us */
  }
}
