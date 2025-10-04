
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;

void setup(){
  Serial.begin(115200); while(!Serial && millis()<2000){}
  mpx.begin(Serial3, 38400);

  mpx.addAlarmDigital(2, 5,  MPX_RPM,   true, true, 1.0f, 0.0f, 1.0f);  // 0/100
  mpx.addAlarmDigital(3, 4,  MPX_SPEED, true, true, 2.0f, 0.0f, 10.0f); // 0/2.0 km/h
  mpx.addAlarmDigital(4, 12, MPX_LIQUID,true, true, 1.0f, 0.0f, 1.0f);  // 0/1
  mpx.addAlarmDigital(5, 13, MPX_DIST,  true, true, 1.0f, 0.0f, 10.0f); // 0/1.0 km
}

void loop(){ mpx.poll(); }
