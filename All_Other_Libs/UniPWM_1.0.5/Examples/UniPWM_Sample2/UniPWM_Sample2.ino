/* 
  Universal Software Controlled PWM function library  for Arduino nano
  --------------------------------------------------------------------
    Beispielprogramm 2 - Rundumlicht auf onboard LED
*/

#include <UniPWMControl.h>
#include <UniPWMMacros.h>

// Pins for LEDs
#define PIN_TOP_STROBE 13   // eingebaute LED, Rundumlicht, anschwellend/abschwellend

// 2 fach Rampe - flache Rampe bis Helligkeit 45, steile Rampe bis 200, 200 ms Pause
SEQUENCE( beacon) = { RAMP(0,45,20), RAMP(45,200,20), CONST(200,20), RAMP(200,45,20), RAMP(45,0,20), PAUSE(20) };

// PWM Control Objekt
UniPWMControl ctrl;

void setup()
{
  ctrl.Init( 1 ); // ein Kanal
  ctrl.AddSequence( 0, PIN_TOP_STROBE, ARRAY( beacon) ); //Beacon Sequenz
  ctrl.ActivatePattern( 0 );
}

void loop()
{
  delay( 500 );
}
