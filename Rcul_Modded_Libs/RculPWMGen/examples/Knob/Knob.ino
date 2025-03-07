// Controlling a servo position using a potentiometer (variable resistor) 
// by Michal Rinott <http://people.interaction-ivrea.it/m.rinott> 
// Adapted to RculPWMGen library by RC Navy  (http://p.loussouarn.free.fr)
// This sketch can work with ATtiny and Arduino UNO, MEGA, etc...

#include <RculPWMGen.h> 
#include <Rcul.h>
 
RculPWMGen myservo;  // create servo object to control a servo 
 
//Here is the POT_PIN definition for Arduino UNO, MEGA, they do need a 'A' prefix for Analogic definition
#define POT_PIN           A2 // --analog pin--  (not digital) used to connect the potentiometer

#define SERVO_PIN         3  // --digital pin-- (not analog)  used to connect the servo

#define REFRESH_PERIOD_MS 20

#define NOW               1

int val;    // variable to read the value from the analog pin 
 
void setup() 
{ 
  myservo.attach(SERVO_PIN);  // attaches the servo on pin defined by SERVO_PIN to the servo object 
} 
 
void loop() 
{ 
  val = analogRead(POT_PIN);           // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 0, 179);     // scale it to use it with the servo (value between 0 and 180) 
  myservo.write(val);                  // sets the servo position according to the scaled value 
  delay(REFRESH_PERIOD_MS);            // waits for the servo to get there 
  RculPWMGen::refresh(NOW);        // generates the servo pulse Now
}

