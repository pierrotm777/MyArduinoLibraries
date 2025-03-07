/* 
  Universal Software Controlled PWM function library  for Arduino nano
  --------------------------------------------------------------------
  Beispielprogramm 4: Single flash and double flash alternating with 
                      2nd RC input to engage afterburner

                      needs version 1.04 of UniPWM library or higher
*/

#include <UniPWMControl.h>
#include <UniPWMMacros.h>

// LED pins 
#define PIN_BOT_STROBE    7   // red single flash alternating with top
#define PIN_TOP_STROBE    8   // red double flash
#define PIN_AFTERBURNER   10  // afterburner

#define RECEIVER_INP       2   // receiver light switch channel
#define RECEIVER_THROTTLE A5   // receiver throttle input, do not use A6/A7 as digital input

SEQUENCE( constOff )    = { HOLD( 0 ) };
SEQUENCE( constOn )     = { HOLD( 200 ) };
SEQUENCE( singleFlash ) = { PAUSE(80), CONST(200,20), PAUSE(40) };
SEQUENCE( doubleFlash ) = { PAUSE(10), CONST(200,20), PAUSE(10), CONST(200,20), PAUSE(80) };

// PWM Control Objekt
UniPWMControl ctrl;

enum { PATTERN_OFF, PATTERN_FLIGHT, PATTERN_AFTERBURNER };

void setup()
{
  Serial.begin(9600);

  ctrl.Init( 3, 2 ); // max 3 output channels, 2 input channels
  ctrl.SetLowBatt( 220, PATTERN_OFF, A7 ); // 840 ~8.28 Volt, switch off value ~ 755 (3.7V) on Pin A7
  ctrl.SetInpChannelPin( RECEIVER_INP );   // first input pin is default when DoLoop is called w/o parameters
  ctrl.SetInpChannelPin( RECEIVER_THROTTLE );

  // off
  ctrl.AddSequence( PATTERN_OFF, PIN_TOP_STROBE, ARRAY( constOff ) ); 
  ctrl.AddSequence( PATTERN_OFF, PIN_BOT_STROBE, ARRAY( constOff ) ); 
  ctrl.AddSequence( PATTERN_OFF, PIN_AFTERBURNER, ARRAY( constOff ) ); 
  // normal flight
  ctrl.AddSequence( PATTERN_FLIGHT, PIN_TOP_STROBE, ARRAY( doubleFlash ) ); 
  ctrl.AddSequence( PATTERN_FLIGHT, PIN_BOT_STROBE, ARRAY( singleFlash ) ); 
  ctrl.AddSequence( PATTERN_FLIGHT, PIN_AFTERBURNER, ARRAY( constOff ) ); 
  // afterburner and lights
  ctrl.AddSequence( PATTERN_AFTERBURNER, PIN_TOP_STROBE, ARRAY( doubleFlash ) ); 
  ctrl.AddSequence( PATTERN_AFTERBURNER, PIN_BOT_STROBE, ARRAY( singleFlash ) ); 
  ctrl.AddSequence( PATTERN_AFTERBURNER, PIN_AFTERBURNER, ARRAY( constOn ) ); 
  
  // assign 3 switch positions to corresponding patterns
  ctrl.AddInputSwitchPos( 31, 39, PATTERN_FLIGHT );
  ctrl.AddInputSwitchPos( 42, 50, PATTERN_OFF );
  ctrl.AddInputSwitchPos( 100, 100, PATTERN_AFTERBURNER ); // pseudo switch position

  ctrl.ActivatePattern( PATTERN_FLIGHT ); // activate normal flight mode
}


void loop()
{
  uint16_t throttle = ctrl.GetInputChannelValue( RECEIVER_THROTTLE ); // throttle value
  uint16_t lightsw  = ctrl.GetInputChannelValue( RECEIVER_INP );      // light switch value

  Serial.print( "Throttle: " );
  Serial.println( throttle );

  if( throttle < 30  )
    lightsw = 100;

  ctrl.DoLoop( lightsw );

  delay( 500 );
}
