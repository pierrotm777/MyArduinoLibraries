#include<MSP.h>

MSP msp;

void setup()
{
  Serial.begin(115200);
  msp.begin(Serial);
}

void loop()
{
  msp_rc_t rc;
  if (msp.request(MSP_RC, &rc, sizeof(rc))) {
  	
    uint16_t roll     = rc.channelValue[0];
    uint16_t pitch    = rc.channelValue[1];
    uint16_t yaw      = rc.channelValue[2];
    uint16_t throttle = rc.channelValue[3];
    
  }
}