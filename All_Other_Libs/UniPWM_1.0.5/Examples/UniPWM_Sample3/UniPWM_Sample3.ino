/* 
  Universal Software Controlled PWM function library  for Arduino nano
  --------------------------------------------------------------------

  Example program for high level control using UniPWMContrl class

  1.03   01/15/2015  SetLowBatt(...) changed
*/

#include <UniPWMControl.h>
#include <UniPWMMacros.h>

#define ONBOARD_LED 13

// Pins for LEDs
#define LANDING_LIGHT  5
#define TOP_POSLIGHTS  3   // L1, green/red constantly on
#define LOW_POSLIGHTS  4   // green/red constantly on
#define TOP_STROBE     8   // red flashing
#define BOT_STROBE    10   // red flashing, alternating with top
#define BOT_POSLIGHT  11   // white
#define REAR_LIGHT    12   // white, no driver, use external resistor
#define LANDING_SERVO  9   // use pin 9 for analog servo
#define RECEIVER_INP   2   // receiver

// constant lights: on, off, dimmmed
SEQUENCE( constOn )  = { HOLD( 150 ) };
SEQUENCE( constOff ) = { HOLD( 0 ) };
SEQUENCE( dimmedOn ) = { HOLD(50) };
// single flash 200ms, dark 1200ms
SEQUENCE( singleFlash ) = { PAUSE(80), CONST(150,20), PAUSE(40) };
// double flash 2x200ms, pause 800ms
SEQUENCE( doubleFlash ) = { PAUSE(10), CONST(150,20), PAUSE(10), CONST(150,20), PAUSE(80) };
// fader
SEQUENCE( fader ) = { PAUSE(20), RAMP(0,200,150), RAMP(200,0,150 ) };

// beacon - 2 ramps up - hold - 2 ramps down
SEQUENCE( beacon ) = { RAMP(0,45,20),  RAMP(45,200,20), CONST(200,20), RAMP(200,45,20), RAMP(45,0,20), PAUSE(20) };

// beamer
SEQUENCE( beamOn )  = { CONST(0,40),    RAMP(0,250,20 ), HOLD(250) };
SEQUENCE( beamOff ) = { RAMP(250,0,20), HOLD(0) };

// beamer retract servo
#define RETRACT_POS 10 // 238
#define ASCEND_POS  255 // 198
SEQUENCE( retract )  = { RAMP( ASCEND_POS, RETRACT_POS, 100 ), HOLD( RETRACT_POS ) };
SEQUENCE( ascend )   = { RAMP( RETRACT_POS, ASCEND_POS, 100 ), HOLD( ASCEND_POS ) };
SEQUENCE( retracted) = { HOLD( RETRACT_POS ) };

UniPWMControl ctrl;

void setup()
{
  enum { PATTERN_STARTUP, PATTERN_FLIGHT, PATTERN_OFF, PATTERN_LANDING };

  Serial.begin(9600);

  // high level object
  ctrl.Init( 10 );                         // max 10 output channels
  ctrl.SetLowBatt( 400, PATTERN_OFF, A0 ); // 840 ~8.28 Volt, switch off value ~ 755 (3.7V) on Pin A0
  ctrl.SetInpChannelPin( RECEIVER_INP, UniPWMInpChannel::INP_INVERTED );

  // retract servo at startup
  ctrl.AddSequence( PATTERN_STARTUP, LANDING_SERVO, ARRAY( retracted ), UniPWMChannel::ANALOG_SERVO ); // servo

  // flight  
  ctrl.AddSequence( PATTERN_FLIGHT, TOP_STROBE,    ARRAY( beacon ) ); 
  ctrl.AddSequence( PATTERN_FLIGHT, BOT_STROBE,    ARRAY( doubleFlash ) ); 
  ctrl.AddSequence( PATTERN_FLIGHT, REAR_LIGHT,    ARRAY( fader ) );   
  ctrl.AddSequence( PATTERN_FLIGHT, TOP_POSLIGHTS, ARRAY( constOn ) );  
  ctrl.AddSequence( PATTERN_FLIGHT, LOW_POSLIGHTS, ARRAY( singleFlash ) );  
  ctrl.AddSequence( PATTERN_FLIGHT, BOT_POSLIGHT,  ARRAY( dimmedOn ) );  
  ctrl.AddSequence( PATTERN_FLIGHT, LANDING_LIGHT, ARRAY( beamOff ) );
  ctrl.AddSequence( PATTERN_FLIGHT, LANDING_SERVO, ARRAY( retract ), UniPWMChannel::ANALOG_SERVO );

  // off
  ctrl.AddSequence( PATTERN_OFF, TOP_STROBE,    ARRAY( constOff ) ); 
  ctrl.AddSequence( PATTERN_OFF, BOT_STROBE,    ARRAY( constOff ) ); 
  ctrl.AddSequence( PATTERN_OFF, REAR_LIGHT,    ARRAY( constOff ) );   
  ctrl.AddSequence( PATTERN_OFF, TOP_POSLIGHTS, ARRAY( constOff ) );  
  ctrl.AddSequence( PATTERN_OFF, LOW_POSLIGHTS, ARRAY( constOff ) );  
  ctrl.AddSequence( PATTERN_OFF, BOT_POSLIGHT,  ARRAY( constOff ) );  
  ctrl.AddSequence( PATTERN_OFF, LANDING_LIGHT, ARRAY( beamOff ) );
  ctrl.AddSequence( PATTERN_OFF, LANDING_SERVO, ARRAY( retract ), UniPWMChannel::ANALOG_SERVO );

  // landing
  ctrl.AddSequence( PATTERN_LANDING, TOP_STROBE,    ARRAY( beacon ) ); 
  ctrl.AddSequence( PATTERN_LANDING, BOT_STROBE,    ARRAY( doubleFlash ) ); 
  ctrl.AddSequence( PATTERN_LANDING, REAR_LIGHT,    ARRAY( singleFlash ) );   
  ctrl.AddSequence( PATTERN_LANDING, TOP_POSLIGHTS, ARRAY( constOn ) );  
  ctrl.AddSequence( PATTERN_LANDING, LOW_POSLIGHTS, ARRAY( constOn ) );  
  ctrl.AddSequence( PATTERN_LANDING, BOT_POSLIGHT,  ARRAY( dimmedOn ) );  
  ctrl.AddSequence( PATTERN_LANDING, LANDING_LIGHT, ARRAY( beamOn ) );
  ctrl.AddSequence( PATTERN_LANDING, LANDING_SERVO, ARRAY( ascend ), UniPWMChannel::ANALOG_SERVO );

  // assign 3 switch positions to corresponding patterns
  ctrl.AddInputSwitchPos( 1, 35, PATTERN_LANDING );
  ctrl.AddInputSwitchPos( 41, 46, PATTERN_FLIGHT );
  ctrl.AddInputSwitchPos( 50, 99, PATTERN_OFF );
  
  // initalize patterns "flight" and "retracted servo"
  ctrl.ActivatePattern( PATTERN_FLIGHT ); 
  ctrl.ActivatePattern( PATTERN_STARTUP ); 
}


void loop()
{
  ctrl.DoLoop();
  delay( 500 );
  return;
}
