/* Basic Bluetooth sketch connects (at 38400) the HC05 to a BT device
	and communicate using the serial monitor at 9600.
	Pairs the BT devuices and allows communication between HC-05 and BT via the Arduino GUI serial monitor and keyboard.
	Connect HC05 VCC to Arduino pin 12
	Connect HC05 EN to Arduino pin 11 through a voltage divider. 
	Connect the HC05 GND to GND
	Connect the HC-05 TX to Arduino pin 2 RX. 
	Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.
*/

#include <HC05Bind.h>
HC05Bind HC05; //create HC05 object
bool paired = false; //paired ok flag

void setup() {
  if(HC05.begin("0000")) //get HC05 ready for pairing and set password
    {
      if(HC05.search()) //find BT devices and pair
			{
				Serial.println(F("ALL OK"));
			}
	   else
			{
				Serial.println(F("***FAILED - EXITING***"));
			}
    }
   else
    {
      Serial.println(F("***FAILED - EXITING***"));
    }
	
}

void loop() {
   // Communications between HC05 and BT Device

    
if(HC05.paired)//check BT devices are paired
    {
       HC05.comms();//start communication with BT device
      
    }
}
