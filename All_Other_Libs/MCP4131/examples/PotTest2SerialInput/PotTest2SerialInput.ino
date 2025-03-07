/*
 * This sketch shows how an Arduino can control a MCP4131 digital potentiometer
 * Sketch utilizes the MCP4131 library
 * https://www.arduino.cc/reference/en/libraries/mcp4131-library/
 * After initialization, serial monitor waits for input
 * User inputs a value between 0 and 128
 * (Note - no error checking in case of invalid input)
 * Program sets potentiometer value to match input
 * and prints out nominal voltage
 * 
 * February 2022
*/


#include <MCP4131.h>

const int chipSelect = 10;
unsigned int wiperValue;
String incomingString;

// Create MCP4131 object nominating digital pin used for Chip Select
MCP4131 Potentiometer(chipSelect);

void setup() {
  // Reset wiper position to 0 Ohms
  wiperValue = 0;
  Potentiometer.writeWiper(wiperValue);

  // Begin Serial port and print out welcome message
  Serial.begin(9600);
  Serial.println("MCP413451 Digital Potentiometer Test");
}

void loop() {
  // Wait for user input via serial monitor
  if (Serial.available() > 0){
     //Read the incoming string
     incomingString = Serial.readString();
     //Convert to a integer
     wiperValue = incomingString.toInt();
     //Set Potentiomter to step value
     Potentiometer.writeWiper(wiperValue);
  
     // Print out user input and nominal voltage
     Serial.print(wiperValue);
     Serial.print('\t');
     Serial.print("Nominal Voltage: ");
     Serial.print((5.0 * wiperValue / 128.0) ,DEC);
     Serial.println();
  }

  delay(200);
}
