#include "HX711.h"
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;                     

HX711 scale;

float calibration_factor = -15.7;
float units;
float ounces;

void setup() {
   Serial.begin(9600);
   scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
   
   scale.set_scale();
   scale.tare();
   scale.set_scale(calibration_factor);
}

void loop() { 
   Serial.print("Reading: ");
  
   for(int i=0; i<10; i++) { units =+ scale.get_units(), 10; } 
   units / 10;
   ounces = units * 0.035274;
   Serial.print(ounces);
   Serial.print(" grams");  
   Serial.println();
}