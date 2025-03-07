/*
  HC05Bind.cpp - Library for communication Arduino monitor via HC05 to BT device
  Connect HC05 VCC to Arduino pin 12
  Connect HC05 EN to Arduino pin 11 through a voltage divider.
  Connect the HC-05 TX to Arduino pin 2 RX. 
  Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.
  If paired with a bluetooth enabled GPS will display GPS information 
  together with distance and bearing to a final location.
  Created by Brian Lambert.
  Released into the public domain.
*/

#include "Arduino.h"
#include "HC05Bind.h"

SoftwareSerial BTserial(2, 3); // RX | TX
// Connect the HC-05 TX to Arduino pin 2 RX. 
// Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.


HC05Bind::HC05Bind()
{
  
}

//Start up everything, set to command mode, find HC05, clear pairings, set to master
boolean HC05Bind::begin(String pw)
{
  // start th serial communication with the host computer
    Serial.begin(9600);
    Serial.println(F("Comms with Arduino started at 9600"));
 // start communication with the HC-05 using 38400
    BTserial.begin(38400);  
    Serial.println(F("Comms with HC05 started at 38400"));
 //set HC05 to AT command mode
    pinMode(HC05AtPin, OUTPUT); //HC05 to command mode
    pinMode(HC05PowerPin, OUTPUT); //Power up HC05
    setHC05ToCommandMode();
	if(locate())
		{
			if(setPassword(pw))
				{
					if(clearPairings())
						{
							if(setToMaster())
								{
									return true;
								}
							else
								{
									return false;
								}
						}
					else
						{
							return false;
						}

				}
			else
				{
					return false;
				}
		}
	else
		{
			return false;
		}
	
	

}

//Start comms with HC05 and linled BT GPS device

void HC05Bind::gps()//get GPS phrase
	{
		// Keep reading from HC-05 and send to Arduino Serial Monitor when NMEAphrase found
		available = false;
		fix = false;
		if (BTserial.available())
			{  
				c = BTserial.read();
				if(c == '$' & started) //check for second false start
					{
						phrase = "";
						started = false;
						ended = true;
						countChars = 0;
						
					}
				if(c == '$' & ended)// check for true start
					{
						started = true;
						ended = false;
						
					}
				if(started)//continue until end found
					{
						phrase = phrase + c;
						countChars++;
						
					}
				if(c == '\n' & started)//end found
						{
							started = false;
							ended = true;
							if(phrase.substring(countChars-5, countChars - 4) != "*")//check for at least a *
							{
								available = false;
							}
							else if(isValid(phrase))//check for valid NMEA sentence
							{
								available = true;//a valid sentence received
								if(phrase.substring(3,6) == "GGA")//check for GGA sentence
								{
									fix = false;//set no fix
									if(phrase.substring(findComma(6) + 1, findComma(7)) == "1")//check for fix
									{
										fix = true;
										latDeg = phrase.substring(findComma(2) + 1, findComma(2)+3).toInt();//get latidude degrees
										latMin = phrase.substring(findComma(2) + 3, findComma(3)).toFloat();//get latidude minutes
										latSector = phrase.substring(findComma(3) + 1, findComma(4));//N or S
										longDeg = phrase.substring(findComma(4) + 1, findComma(4)+4).toInt();//get latidude degrees
										longMin = phrase.substring(findComma(4) + 4, findComma(5)).toFloat();//get latidude minutes
										longSector = phrase.substring(findComma(5) + 1, findComma(6));//E or W
										UTC = phrase.substring(findComma(1) + 1, findComma(1)+3) + ":"
											+ phrase.substring(findComma(1) + 3, findComma(1)+5) + ":"
											+ phrase.substring(findComma(1) + 5, findComma(2));//UTC time
										altitude = phrase.substring(findComma(9) + 1, findComma(10)).toFloat();
										tracked = phrase.substring(findComma(7) + 1, findComma(8)).toInt();
										DOP = phrase.substring(findComma(8) + 1, findComma(9)).toFloat();//get horizontal dilution of position
										latDegFloat = (float)latDeg + (latMin/60.0);//lat and long as float values
										longDegFloat = (float)longDeg + (longMin/60.0);
										if(latSector == "S")
										{
											latDegFloat = -latDegFloat;
										}
										if(longSector == "W")
										{
											longDegFloat = -longDegFloat;
										}
										
									}
									
								}
								if(phrase.substring(3,6) == "GSA")//check for GSA sentence
								{
									fix = false;//set no fix
									if(phrase.substring(findComma(2) + 1, findComma(3)).toInt() > 1);//check for 2D or 3D fix
									{
										fix = true;
										PRNs = phrase.substring(findComma(3) + 1, findComma(15));//get latidude degrees
									}
									
								}
								NMEAphrase = phrase;
							}
							countChars = 0;
							phrase = "";
							
						}
					
			}
		
	}

//find the position of nth coma in phrase

int HC05Bind::findComma(int nthComa)
{
	int found = 0;
	int oldIndex = 0;
	int newIndex = 0;
	while(found < nthComa & newIndex >=0)
	{
		newIndex = phrase.indexOf(",", oldIndex + 1);//get next comma
		found++;
		oldIndex = newIndex;
	}
	return newIndex;
}
	

//NMEA checksum calculator
// Calculates the checksum for a sentence
String HC05Bind::getChecksum(String sentence) {
        //Start with first Item
	int checksum= (byte)sentence[sentence.indexOf('$')+1];
	// Loop through all chars to get a checksum
	for (int i=sentence.indexOf('$')+2 ; i<sentence.indexOf('*') ; i++){
		// No. XOR the checksum with this character's value
		checksum^=(byte)sentence[i];				
	}
	String upper = String(checksum, HEX);
	upper.toUpperCase();
	// Return the checksum formatted as a two-character hexadecimal
	//Serial.print(" calc  ");
	//Serial.println(upper);
	return upper;
}

bool HC05Bind::isValid(String sentence) 
{
    // Compare the characters after the asterisk to the calculation
	//Serial.print("NMEA  ");
	//Serial.print(sentence.substring(sentence.indexOf("*") + 1,sentence.indexOf("*") + 3));
    return sentence.substring(sentence.indexOf("*") + 1,sentence.indexOf("*") + 3) == getChecksum(sentence);
}

//Start comms with HC05 and linled BT device

void HC05Bind::comms()
	{
		// Keep reading from HC-05 and send to Arduino Serial Monitor
		char c;
		if (BTserial.available())
			{  
				c = BTserial.read();
				Serial.write(c);
			}
 
		// Keep reading from Arduino Serial Monitor and send to HC-05
		if (Serial.available())
			{
				c =  Serial.read();
 
		// mirror the commands back to the serial monitor
        // makes it easy to follow the commands
        Serial.write(c);   
        BTserial.write(c);  
			} 
	}


//search for BT devices
boolean HC05Bind::search()
	{
		paired = false;//BT devices not yet paired
		if(setToAnyBT())
			{
				if(setToEnquiry())
					{
						SppProfile();
						if(find())
							{
								paired = true;//BT devices now paired
								return true;
							}
						else
							{
								return false;
							}
					}
				else
					{
						return false;
					}
			}
		else
			{
				return false;
			}

	}


//Find upt to 5 BT devices
boolean HC05Bind::find()
	{
		sendAtCommand("AT+INQ");
		Serial.println(F("Searching for BT devices"));
		delay(9000);
		reply = readHC05();
		if(reply.indexOf("INQ") >= 0)
			{
				int first;
				int endFirst;
				//int second;
				//int endSecond;
				int chopStart;
				int chopEnd;
				String temp;
				String firstAddress;
				String BTname;
				Serial.println(F("BT devices found"));
				first = reply.indexOf("+INQ");
				endFirst = reply.indexOf(",");
				temp = reply.substring(first+5,endFirst);
				temp.replace(":",",");
				firstAddress = temp;
				sendAtCommand("AT+RNAME?"+firstAddress);
				delay(2000);
				temp = readHC05();
				chopStart = (temp.indexOf(":"))+1;
				chopEnd = temp.indexOf("\r",chopStart);
				BTname = temp.substring(chopStart, chopEnd);
				Serial.print(BTname);
				Serial.print(F(" at address "));
				Serial.println(firstAddress);
				sendAtCommand("AT+PAIR=" + firstAddress + "," + 9);
				delay(9000);
				reply = readHC05();
				if(reply.indexOf("OK") >= 0)
					{
						Serial.print(F("Paired with "));
						Serial.println(BTname);
						sendAtCommand("AT+BIND=" + firstAddress);
						delay(1000);
						reply = readHC05();
						if(reply.indexOf("OK") >= 0)
							{
								Serial.print(F("Bound with "));
								Serial.println(BTname);
								sendAtCommand("AT+CMODE=1");
								delay(1000);
								reply = readHC05();
								if(reply.indexOf("OK") >= 0)
									{
										Serial.println(F("HC05 set to only connect with paired devices"));
										sendAtCommand("AT+LINK="+firstAddress);
										delay(4000);
										reply = readHC05();
										if(reply.indexOf("OK") >= 0)
											{
												Serial.println(F("HC05 linked"));
												return true;
											}
										else
											{
												Serial.println(F("Failed to link"));
												return false;
											}
									}
								else
									{
										Serial.println(F("Failed to set mode 1"));
										return false;
									}
							}
						else
							{
								Serial.println(F("Failed to bind"));
								return false;
							}
					}
				else
					{
						Serial.println(F("Failed to pair, check password"));
						return false;
					}
					
		
			}
		else
			{
				Serial.println(F("Failed to find any BT devices"));
				return false;
			}
	}

//set HC05 enquiry password
boolean HC05Bind::setPassword(String pw)
	{
		sendAtCommand("AT+PSWD=" + pw);
		reply = readHC05();
		if(reply.indexOf("OK") >= 0)
			{
				sendAtCommand("AT+PSWD?");
				reply = readHC05();
				Serial.print(F("HC05 password set to "));
				Serial.println(reply.substring((reply.indexOf(":") + 1),(reply.indexOf("\r"))));//cut of the rubish
				return true;
		
			}
		else
			{
				Serial.println(F("Failed to set password"));
				return false;
			}
	}

//Initiate SPP profile no error checking needed
void HC05Bind::SppProfile()
	{
		sendAtCommand("AT+INIT");
		Serial.println(F("SPP profile initiated"));
	}

//set HC05 enquiry up to 3 devices for 9 seconds
boolean HC05Bind::setToEnquiry()
	{
		sendAtCommand("AT+INQM=0,3,9");
		reply = readHC05();
		if(reply.indexOf("OK") >= 0)
			{
				Serial.println(F("HC05 set to enquiry"));
				return true;
		
			}
		else
			{
				Serial.println(F("Failed to set search for any BT devices"));
				return false;
			}
	}

//set HC05 to connect to any BT devices
boolean HC05Bind::setToAnyBT()
	{
		sendAtCommand("AT+CMODE=0");
		reply = readHC05();
		if(reply.indexOf("OK") >= 0)
			{
				Serial.println(F("HC05 set to find any BT device"));
				return true;
		
			}
		else
			{
				Serial.println(F("Failed to set search for any BT devices"));
				return false;
			}
	}


//******put HC05 into AT cammand mode******
  void HC05Bind::setHC05ToCommandMode()
    {
      digitalWrite(HC05AtPin, HIGH);//set HC05 to Command Mode
      delay(500);
      digitalWrite(HC05PowerPin, HIGH);
      delay(1500);
      Serial.println(F("HC05 set to AT command mode"));
	  Serial.println("****************");
    }

  //************locate HC05**************
  bool HC05Bind::locate()
  {
	sendAtCommand("AT");
    reply = readHC05();
    if(reply.indexOf("OK") >= 0)
      {
        Serial.println(F("HC05 responding"));
		return true;
		
      }
    else
      {
        Serial.println(F("HC05 NOT responding"));
        return false;
      }
  }

//***************Read HC05 ******************//
String HC05Bind::readHC05()
  {
    String response = "";
    char c ="";
    while (BTserial.available())
    {  
        c = BTserial.read();
        response = response + c;
    }
    return response;
  }

  //*****END ReadHC05******//

//**************** serially push out a String to HC05 with BTserial.write()***************//

void HC05Bind::writeString(String stringData) { 

  for (int i = 0; i < stringData.length(); i++)
  {
		BTserial.write(stringData[i]);   // Push each char 1 by 1 on each loop pass
  }
  BTserial.write("\r\n");

}// end writeString

//*****send AT command to HC05****//
void HC05Bind::sendAtCommand(String command)
    {
		writeString(command);
		delay(1000);
    }
 //**END send AT command*******//


bool HC05Bind::clearPairings()
	{
		sendAtCommand("AT+RMAAD");
		reply = readHC05();
		if(reply.indexOf("OK") >= 0)
			{
				Serial.println(F("Existing pairings cleared"));
				return true;
			}	
		else
			{
				Serial.println(F("Clear pairings failed"));
				return false;
			}
	}

bool HC05Bind::setToMaster()
	{
		sendAtCommand("AT+ROLE=1");
		reply = readHC05();
		if(reply.indexOf("OK") >= 0)
			{
				Serial.println(F("HC05 set to master"));
				return true;
			}	
		else
			{
				Serial.println(F("Set to master failed"));
				return false;
			}
	}

float HC05Bind::distance(float lat1,float long1,float lat2,float long2)
{
	theta1 = lat1*TORADS;
	theta2 = lat2*TORADS;
	deltaTheta = (lat2-lat1)*TORADS;
	deltaLanda = (long2-long1)*TORADS;
	a = sin(deltaTheta/2)*sin(deltaTheta/2) + cos(theta1)*cos(theta2)*sin(deltaLanda/2)*sin(deltaLanda/2);
	c1 = 2*atan2(sqrt(a), sqrt(1-a));
	return d = R*c1;
}

float HC05Bind::bearing(float lat1,float long1,float lat2,float long2)
{
	theta1 = lat1*TORADS;
	theta2 = lat2*TORADS;
	landa1 = long1*TORADS;
	landa2 = long2*TORADS;
	y = sin(landa2 - landa1)*cos(theta2);
	x = cos(theta1)*sin(theta2) - sin(theta1)*cos(theta2)*cos(landa2-landa1);
	brng = atan2(y,x)*TODEGS;
	return fmod((brng + 360.0),360.0);

}
