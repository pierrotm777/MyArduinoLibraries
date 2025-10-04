
#include <MPX_MSB.h>
using namespace MPX;

Mpx_Msb mpx;

void setup(){
  mpx.begin(Serial3, 38400);
  mpx.sendTmp1(22.0f);
  mpx.sendTmp2(35.0f);
}

void loop(){ mpx.poll(); }
