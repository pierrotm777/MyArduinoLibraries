
#include <MPX_MSB.h>
using namespace MPX;

Mpx_Msb mpx;

void setup(){
  mpx.begin(Serial3, 38400);
  mpx.setEchoMasking(true);
  mpx.sendVbat(11.1f);
}

void loop(){ mpx.poll(); }
