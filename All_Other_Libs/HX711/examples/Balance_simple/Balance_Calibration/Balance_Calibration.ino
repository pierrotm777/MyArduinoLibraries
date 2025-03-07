#include "HX711.h"
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

HX711 scale;                     

float calibration_factor = -3.7;
float units;
float ounces;

void setup() {
   Serial.begin(9600);
   Serial.println("HX711 calibration sketch");
   Serial.println("Remove all weight from scale");
   Serial.println("Press + or a to increase calibration factor");
   Serial.println("Press - or z to decrease calibration factor");
   
   scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
   scale.set_scale();
   scale.tare();

   long zero_factor = scale.read_average();
   Serial.print("Zero factor: ");
   Serial.println(zero_factor);
}

void loop() {
   scale.set_scale(calibration_factor);

   Serial.print("Reading: ");
   units = scale.get_units(), 10;
   if (units < 0) { units = 0.00; }
   ounces = units * 0.035274;

   Serial.print(ounces);
   Serial.print(" grams"); 
   Serial.print(" calibration_factor: ");
   Serial.print(calibration_factor);
   Serial.println();

   if (Serial.available())
   {
       char temp = Serial.read();
       if (temp == '+' || temp == 'a')
          calibration_factor += 1;
       else if (temp == '-' || temp == 'z')
          calibration_factor -= 1;
   }
}