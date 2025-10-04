
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;

void setup(){
  mpx.begin(Serial3,38400);
  mpx.sendVbat(15.0f);
  mpx.sendTmp1(25.0f);
  mpx.sendTmp2(30.0f);
}

void loop(){ mpx.poll(); }
