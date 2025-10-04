
#include <MPX_MSB.h>
using namespace MPX;

Mpx_Msb mpx;

void setup(){
  mpx.begin(Serial3, 38400);
  mpx.addAlarmDigital(18,  9, MPX_LIQUID);
  mpx.addAlarmDigital(19, 10, MPX_LIQUID);
  mpx.addAlarmDigital(22, 11, MPX_LIQUID);
  mpx.addAlarmDigital(23, 12, MPX_LIQUID);
}

void loop(){ mpx.poll(); }
