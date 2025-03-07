/* 
  Universal Software Controlled PWM function library  for Arduino nano
  --------------------------------------------------------------------
  Beispielprogramm 4: Single flash and double flash alternating with 
                      2nd RC input to engage afterburner

                      needs version 1.04 of UniPWM library or higher
*/

#include <UniPWMControl.h>
#include <UniPWMMacros.h>


// Output pins 
#define PIN_STEER  9   // Steer_Servo
#define PIN_DOOR   11   // Door
#define PIN_GEAR   10  // Gear

#define RECEIVER_GEAR     2   // receiver gear switch channel
#define RECEIVER_RUDDER  A5   // receiver rudder input


SEQUENCE( GearDown ) = { HOLD(50) };
SEQUENCE( GearUp )   = { HOLD(25) };
SEQUENCE( DoorDown ) = { RAMP(26,50,100), CONST(50,150), RAMP(50,26,100), PAUSE(0)};
SEQUENCE( DoorUp )   = { RAMP(26,50,100), CONST(50,150), RAMP(50,26,100), PAUSE(0)};
SEQUENCE( rudder )   = { HOLD(200) };
SEQUENCE( ruddercenter )   = { HOLD(45) };



// PWM Control Objekt
UniPWMControl ctrl;

enum { GEAR_UP, GEAR_DOWN, RUDDER_POS };

void setup()
{

  Serial.begin(9600);

  ctrl.Init( 3, 2 ); // max 3 output channels, 2 input channels
//  ctrl.SetLowBatt( 220, PATTERN_OFF, A7 ); // 840 ~8.28 Volt, switch off value ~ 755 (3.7V) on Pin A7
  
  ctrl.SetInpChannelPin( RECEIVER_GEAR, UniPWMInpChannel::INP_NORMAL );   // first input pin is default when DoLoop is called w/o parameters
  ctrl.SetInpChannelPin( RECEIVER_RUDDER, UniPWMInpChannel::INP_NORMAL );
  
  // Gear up
  ctrl.AddSequence( GEAR_UP, PIN_DOOR, ARRAY( DoorUp ), UniPWMChannel::SOFTPWM_SERVO );
  ctrl.AddSequence( GEAR_UP, PIN_GEAR, ARRAY( GearUp ), UniPWMChannel::SOFTPWM_SERVO ); 
  ctrl.AddSequence( RUDDER_POS, PIN_STEER, ARRAY( ruddercenter ), UniPWMChannel::SOFTPWM_SERVO );

  
  
  // Gear Down
  ctrl.AddSequence( GEAR_DOWN, PIN_DOOR, ARRAY( DoorDown ), UniPWMChannel::SOFTPWM_SERVO );
  ctrl.AddSequence( RUDDER_POS, PIN_STEER, ARRAY( rudder ), UniPWMChannel::SOFTPWM_SERVO );
  ctrl.AddSequence( GEAR_DOWN, PIN_GEAR, ARRAY( GearDown ), UniPWMChannel::SOFTPWM_SERVO ); 

  
  
  ctrl.AddInputSwitchPos( 23, 39, GEAR_UP );
  ctrl.AddInputSwitchPos( 40, 47, GEAR_DOWN );
  ctrl.AddInputSwitchPos( 100, 100, RUDDER_POS ); // pseudo switch position

  ctrl.ActivatePattern( GEAR_UP ); // activate normal flight mode
}


void loop()
{
  uint16_t rudder = ctrl.GetInputChannelValue( RECEIVER_RUDDER ); // Rudder value
  uint16_t gear  = ctrl.GetInputChannelValue( RECEIVER_GEAR );    // Gear switch value


  Serial.print( "Rudder: " );
  Serial.println( rudder );

  ctrl.DoLoop();

  delay( 500 );
}