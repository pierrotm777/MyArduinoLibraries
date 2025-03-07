/* 
  Universal Software Controlled PWM function library  for Arduino nano
  --------------------------------------------------------------------
  Beispielprogramm 1: Einfach-Blitz und Doppelblitz alternierend
*/

#include <UniPWMControl.h>
#include <UniPWMMacros.h>

// Pins fuer LEDs
#define PIN_TOP_STROBE    8   // rot doppelt blitzend
#define PIN_BOT_STROBE    6   // rot einfach blitzend, alternierend mit TOP

SEQUENCE( singleFlash ) = { PAUSE(80), CONST(200,20), PAUSE(40) };
SEQUENCE( doubleFlash ) = { PAUSE(10), CONST(200,20), PAUSE(10), CONST(200,20), PAUSE(80) };

// PWM Control Objekt
UniPWMControl ctrl;

void setup()
{
  ctrl.Init( 2 ); // max 2 Kanäle
  ctrl.AddSequence( 0, PIN_TOP_STROBE, ARRAY( doubleFlash ) ); 
  ctrl.AddSequence( 0, PIN_BOT_STROBE, ARRAY( singleFlash ) ); 
  ctrl.ActivatePattern( 0 ); // wir haben nur ein Pattern, das aktivieren wir
}

void loop()
{
  delay( 500 );
}
