/*
CppmGenSweep sketch
by RC Navy (http://p.loussouarn.free.fr/arduino/arduino.html) 2016
This sketch generates an RC PPM frame transporting 4 RC channels.
The 3 first channels have fixed pulse width (tunable), and the 4th channel sweeps between 1000 and 2000 us.
It can be extended up to 12 RC channels.
This sketch can work with a all board equipped with an ATmega328P.

PPM output pin is imposed by hardware and is target dependant: (actually, only ATmega328P is supported)
       - ATmega328P (Arduino UNO, Nano V3):
         TIMER(1), CHANNEL(A) -> OC1A -> PB1 -> Pin#9


This example code is in the public domain.
*/
#include <Cppm.h>
#include <Rcul.h>

#define CH_MAX_NB  4

#define STEP_US    5

#define PULSE_WIDTH_MIN_US    1000
#define PULSE_WIDTH_MAX_US    2000

#define PPM_HEADER_US         300
#define PPM_PERIOD_US         20000

uint16_t Width_us = PULSE_WIDTH_MAX_US;
uint16_t Step_us  = STEP_US;

void setup()
{
  CppmGen.begin(CPPM_GEN_NEG_MOD, CH_MAX_NB, PPM_PERIOD_US, PPM_HEADER_US);/* Change CPPM_GEN_NEG_MOD to CPPM_GEN_POS_MOD for POSitive PPM modulation */
  CppmGen.width_us(1, 500);  /* RC Channel#1 */
  CppmGen.width_us(2, 1000); /* RC Channel#2 */
  CppmGen.width_us(3, 1500); /* RC Channel#3 */
  CppmGen.width_us(4, 2000); /* RC Channel#4 */
}

void loop()
{
  CppmGen.width_us(CH_MAX_NB, Width_us); /* Sweep RC Channel#4 */
  Width_us += Step_us;
  if(Width_us > PULSE_WIDTH_MAX_US) Step_us = -STEP_US;
  if(Width_us < PULSE_WIDTH_MIN_US) Step_us = +STEP_US;
  delay(10);
}
