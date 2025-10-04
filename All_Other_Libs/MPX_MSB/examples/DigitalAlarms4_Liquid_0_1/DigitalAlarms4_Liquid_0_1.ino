
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;

void setup(){
  Serial.begin(115200); while(!Serial && millis()<2000){}
  mpx.begin(Serial3, 38400);

  // Quatre entrées digitales pullup (LOW = alarme) sur D2..D5
  mpx.addAlarmDigital(2,  9, MPX_LIQUID, true, true, 1.0f, 0.0f, 1.0f);
  mpx.addAlarmDigital(3, 10, MPX_LIQUID, true, true, 1.0f, 0.0f, 1.0f);
  mpx.addAlarmDigital(4, 11, MPX_LIQUID, true, true, 1.0f, 0.0f, 1.0f);
  mpx.addAlarmDigital(5, 12, MPX_LIQUID, true, true, 1.0f, 0.0f, 1.0f);
}

void loop(){ mpx.poll(); }
