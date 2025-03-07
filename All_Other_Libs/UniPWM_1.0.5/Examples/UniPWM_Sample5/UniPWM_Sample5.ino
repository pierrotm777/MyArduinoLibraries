/* 
  Universal Software Controlled PWM function library  for Arduino nano
  --------------------------------------------------------------------
  Sample program 5: 2 beacon lights defined by only one SEQUENCE (needs 1.05 of UniPWM library)
*/

#include <UniPWMControl.h>
#include <UniPWMMacros.h>

// Pins for LEDs
#define PIN_BEACON1    3   
#define PIN_BEACON2    4   

// beacon - 2 ramps up - hold - 2 ramps down
SEQUENCE( beacon ) = { RAMP(0,45,20),  RAMP(45,200,20), CONST(200,20), RAMP(200,45,20), RAMP(45,0,20), PAUSE(20) };

// PWM Control Objekt
UniPWMControl ctrl;

void setup()
{
  ctrl.Init( 2 ); // max 2 channels
  ctrl.AddSequence( 0, PIN_BEACON1, ARRAY( beacon ) ); 
  ctrl.AddSequence( 0, PIN_BEACON1, ARRAY( beacon ) ); 
  ctrl.ActivatePattern( 0 ); // we only have 1 pattern, which we activate right now
}

void loop()
{
  delay( 500 );
}
