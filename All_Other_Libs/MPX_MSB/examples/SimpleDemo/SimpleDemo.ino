
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;

void setup(){
  Serial.begin(115200); while(!Serial && millis()<2000){}
  mpx.begin(Serial3, 38400);

  // Valeurs directes
  mpx.sendVbat(15.0f);
  mpx.sendTmp1(25.0f);
  mpx.sendTmp2(30.0f);

  // Deux alarmes digitales: D2 -> addr9, D3 -> addr10 (LIQUID, 0/1 lisible)
  mpx.addAlarmDigital(2, 9,  MPX_LIQUID, /*activeLow*/true, /*pullup*/true, 1.0f, 0.0f, 1.0f);
  mpx.addAlarmDigital(3, 10, MPX_LIQUID, /*activeLow*/true, /*pullup*/true, 1.0f, 0.0f, 1.0f);
}

void loop(){
  // Exemple: mettre à jour périodiquement
  // mpx.sendVbat(...); mpx.sendTmp1(...); mpx.sendTmp2(...);
  mpx.poll();
}
