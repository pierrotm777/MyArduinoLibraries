/*
 * This sketch shows how an Arduino can control a MCP4131 digital potentiometer
 * Sketch utilizes the MCP4131 library
 * https://www.arduino.cc/reference/en/libraries/mcp4131-library/
 * After initialization, two loops in program increment and decrement
 * potentiometer from 0 to full value and then back again
 * 
 * February 2022
*/


#include <MCP4131.h>

const int chipSelect = 10;
unsigned int wiperValue;
int i;

// Create MCP4131 object nominating digital pin used for Chip Select
MCP4131 Potentiometer(chipSelect);

void setup() {
  // Reset wiper position to 0 Ohms  
  wiperValue = 0;
  Potentiometer.writeWiper(wiperValue);

  // Begin Serial port and print out welcome message
  Serial.begin(9600);
  Serial.println("MCP4131 Digital Potentiometer Test");
  
}

void loop() {
  // Starting at 0 Ohms increment MCP4131 up to max resistance
  for ( i = 0 ; i <= 128; i++){
    Potentiometer.incrementWiper();
    delay(100);
  }

  // Print out message showing voltage is turning
  Serial.println("5V turning point");
  
  // Starting at max resistance decrement MCP4131 to 0 Ohms
  for ( i = 128 ; i >= 0; i--){
    Potentiometer.decrementWiper();
    delay(100);
  }

  // Print out message showing voltage is turning
  Serial.println("0V turning point");
}
