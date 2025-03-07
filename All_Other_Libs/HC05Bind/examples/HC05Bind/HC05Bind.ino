/* Basic Bluetooth sketch connects (at 38400) the HC05 to a BT device
	and communicate using the serial monitor at 9600
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
      while(!HC05.search()) //find BT devices and pair
			{
				Serial.println(F("***Trying again***"));
			}
	  Serial.println(F("***ALL OK***"));
    }
   else
    {
      Serial.println(F("***FAILED - EXITING***"));
    }
	
}

void loop() {
   // Start communications between HC05 and BT Device
   //Don't use the delay() function here, it may cause you to miss some NMEA sentence types.
    
if(HC05.paired)//check BT devices are paired
    {
       HC05.gps();//start forming NMEA sentence from GPS
      if(HC05.available)//NMEA sentence received from GPS, limited data if no fix
      {
        if(HC05.fix)//GPS has a fix
        {
          //Serial.print(HC05.NMEAphrase);//un-comment if you want to see raw NMEA sentence
			Serial.print(F("UTC "));
			Serial.print(HC05.UTC);
			Serial.print(F("  Latitude "));
			Serial.print(HC05.latDeg);
			Serial.print(F("degrees "));
			Serial.print(HC05.latMin, 4);
			Serial.print(F(" minutes "));
			Serial.print(HC05.latSector);
			Serial.print(F(" Longitude "));
			Serial.print(HC05.longDeg);
			Serial.print(F("degrees "));
			Serial.print(HC05.longMin, 4);
			Serial.print(F("minutes " ));
			Serial.print(HC05.longSector);
			Serial.print(F("  Altitude(mtrs) "));
			Serial.println(HC05.altitude);
			Serial.print(F(" Satelites tracked "));
			Serial.print(HC05.tracked);
			Serial.print(F("  Satelite PRNs "));
			Serial.print(HC05.PRNs);
			Serial.print(F("  Dilution of position "));
			Serial.print(HC05.DOP, 1);
			Serial.println(F(" mtrs")); 
        }
        else
        {
          Serial.println(F("waiting for GPS fix"));
        }
      }
    }
}
