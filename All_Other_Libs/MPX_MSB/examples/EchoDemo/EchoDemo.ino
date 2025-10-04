
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;

void setup(){
  Serial.begin(115200); while(!Serial && millis()<2000){}
  mpx.begin(Serial3, 38400);
  mpx.sendVbat(12.0f);
  Serial.println(F("Echo masking OFF (3s)..."));
  mpx.setEchoMasking(false);
}

void loop(){
  static bool flipped=false;
  if(!flipped && millis()>3000){
    flipped=true;
    mpx.setEchoMasking(true);
    Serial.println(F("Echo masking ON."));
  }
  // Optional: raw bus spy (if you wired a separate RX)
  mpx.poll();
}
