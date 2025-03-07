#include <SoftSBusRx.h>
#include <SoftRcPulseOut.h>

// used pins
#define SBUS_PIN          10  // SBUS signal is connected to pin SBUS_PIN
#define SERVO_PIN         2  // The Servo is connected to pin SERVO_PIN

#define CH_IN_SBUS_FRAME  1

static SoftRcPulseOut servo; // The Servo is driven by Channel CH_IN_SBUS_FRAME in the SBUS frame

void setup()
{
  SoftSBusRx.begin(SBUS_PIN); 
  servo.attach(SERVO_PIN); 
}

void loop()
{
  static uint8_t Phase = 0;
  
  if(SoftSBusRx.isSynchro())
  {
    Phase = !Phase; // SBus frame period is 7 or 14 ms: update PWM every 2 SBus frames
    if(Phase)
    {
      servo.write_us(SoftSBusRx.width_us(CH_IN_SBUS_FRAME));
      SoftRcPulseOut::refresh(1);
    }
  }
  
}
