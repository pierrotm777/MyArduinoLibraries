#include "Arduino.h"
#include "MLinkEx.h"
#include <avr/pgmspace.h> 



#define SETW 1
#define GETW 2
#define SETE 3
#define GETE 4
#define SET  5
#define GET  6
#define LANG 100

SoftwareSerial swSerial(10,11);



//----------------------------------------------------------------------------------------
float MLinkEx::wert(char aktion_i, float wert,  char index_i)
{
	static float eingang[16];
	static byte einheit_i[16];
	if((index_i >= 0) &&(index_i < 16 ))
	{
		if((aktion_i == SETW))
		{
			eingang[index_i] = wert;
			return eingang[index_i];
		}  
		else if((aktion_i == SETE))
		{
			if(wert > 15) wert = 15;
			if(wert < 0 ) wert = 0;
			einheit_i[index_i] = (byte)wert;
			return einheit_i[index_i];
		}  
		else if((aktion_i == GETW))
		{
			return eingang[index_i];
		}  
		else if((aktion_i == GETE))
		{
			return einheit_i[index_i];
		}  
    
	}
	else return 0;  
}  
//---------------------------------------------------------------------------------------
float MLinkEx::werte(float werte, char einheit_i, char index_i,bool alarm_b)
{
   alarm_flag_b[index_i]=alarm_b;
   wert(SETW,werte,index_i);
   wert(SETE,(float)einheit_i,index_i);
}  
//---------------------------------------------------------------------------------------
byte MLinkEx::adresse(byte adr)
{
	static byte adresse;
	if(adr>0)adresse=adr;
	return adresse;
}  
//---------------------------------------------------------------------------------------
void MLinkEx::senden()
{
	static byte sendebytes_i = 3, emfangbytes_i = 0, emfangbytesdummy_i = 0,  anfragef_i[4], antwort_i = 0, s_index=0;
	static char anfrage0_i = 1,anfrage_i = 0, anfrage1_i = 0,sendung_i[] = {0, 0, 0, 0, 0};
	static int wert_i = 0;
	static float faktor = 1.0;
	static unsigned long zeit1_i=micros();
	unsigned char adresse_i8 = 0, einheit_i8 = 0, adresse_synth_i8 = 0, einheit_synth_i8 = 0;
	byte lb_i, hb_i, ae_i;
	static bool blinken_b = false, serial_b=false, sendet_b=false, frei_b=true;
	bool overflow_b = false;
   
	//if(!sendet_b)
	{
 		bool ser_b=false;
		if(lok_ser==SW_SERIAL)
			ser_b=swSerial.available();
		else if(lok_ser==HW_SERIAL0)
			ser_b=Serial.available();
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)			
		else if(lok_ser==HW_SERIAL1)
			ser_b=Serial1.available();
		else if(lok_ser==HW_SERIAL2)
			ser_b=Serial2.available();
		else if(lok_ser==HW_SERIAL3)
			ser_b=Serial3.available();
#endif//*/
    


		if(ser_b)
		{

			zeit1_i=micros();
            //Bei Timerinterrupt geht Serial.readBytes() nicht => 2x Serial.read() wenn dann kein 2. Byte mehr kommt ist 2. readf=-1

			if(lok_ser==SW_SERIAL)
			{
				anfrage_i= swSerial.read();
				anfrage1_i= swSerial.read();
			}
			else if(lok_ser==HW_SERIAL0)
			{
				anfrage_i= Serial.read();
				anfrage1_i= Serial.read();
			}	
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)			
			else if(lok_ser==HW_SERIAL1)
			{
				anfrage_i= Serial1.read();
				anfrage1_i= Serial1.read();
			}	
			else if(lok_ser==HW_SERIAL2)
			{
				anfrage_i= Serial2.read();
				anfrage1_i= Serial2.read();
			}	
			else if(lok_ser==HW_SERIAL3)
			{
				anfrage_i= Serial3.read();
				anfrage1_i= Serial3.read();
			}	
#endif//*/
		    //Serial.println(anfrage_i,DEC);
		    //Serial.println(anfrage1_i,DEC);
			
			if(s_index>=3 && (anfrage_i >= 0) && (anfrage_i <= 15)  && (anfrage1_i== -1))
			{		
				adresse(anfrage_i);
				s_index=0;


				//Serial.println("OK");


				if(led_pin >= 0)	
					digitalWrite( led_pin, digitalRead( led_pin ) ^ 1 );
			
				switch((byte)wert(GETE,NULL,anfrage_i))
				{
					case 0x00 :  faktor = 1.0;  break;
					case 0x01 :  faktor = 0.1;  break;
					case 0x02 :  faktor = 0.1;  break;
					case 0x03 :  faktor = 0.1;  break;
					case 0x04 :  faktor = 0.1;  break;
					case 0x05 :  faktor = 100;  break;
					case 0x06 :  faktor = 0.1;  break;
					case 0x07 :  faktor = 0.1;  break;
					case 0x08 :  faktor = 1.0;  break;
					case 0x09 :  faktor = 1.0;  break;
					case 0x0A :  faktor = 1.0;  break;
					case 0x0B :  faktor = 1.0;  break;
					case 0x0C :  faktor = 1.0;  break;
					case 0x0D :  faktor = 0.1;  break;
					case 0x0E :  faktor = 1.0;  break;
					case 0x0F :  faktor = 1.0;  break;
					default   :  faktor = 1.0;  break;  
				}

				wert_i = (int)(wert(GETW,NULL,anfrage_i)/faktor);
			
      //Bestimmung des Overflow-Alarms
			//	if(wert_i < -16384 ||  wert_i > 16383)  overflow_b = true;

			//Aufteilung des Wertes in Teilbytes
				//lb_i = (byte)(((abs(wert_i) << 1) + (int)overflow_b) & 0x00FF);
				lb_i = (byte)(((abs(wert_i) << 1) + (int)alarm_flag_b[anfrage_i]) & 0x00FF);
				if(wert_i < 0)
					hb_i = (byte)(((abs(wert_i) << 1) & 0xFF00) >> 8) | 0x80;
				else
					hb_i = (byte)(((abs(wert_i) << 1) & 0xFF00) >> 8);
				ae_i = (byte)((anfrage_i & 0x0F) << 4) + ((byte)wert(GETE,NULL,anfrage_i) & 0x0F);
      
      //Synthese des 2-Byte-Wertepaars und Adressantwort     
            
				sendung_i[0] = ae_i ;
				sendung_i[1] = lb_i ;
				sendung_i[2] = hb_i ;

				while(micros()-zeit1_i < 256){;}
	//			while(micros()-zeit1_i < 1000){;}
			//delayMicroseconds(1900);
				if(lok_ser==SW_SERIAL)
					swSerial.write(sendung_i,3);	
				else if(lok_ser==HW_SERIAL0)
					Serial.write(sendung_i,3);
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)				
				else if(lok_ser==HW_SERIAL1)
					Serial1.write(sendung_i,3);
				else if(lok_ser==HW_SERIAL2)
					Serial2.write(sendung_i,3);
				else if(lok_ser==HW_SERIAL3)
					Serial3.write(sendung_i,3);
#endif//*/		

				
			} 
			s_index++;
		}   //*/
	}

}
//---------------------------------------------------------------------------------------
MLinkEx::MLinkEx(int pin)
{
	led_pin = pin;
	if(led_pin >= 0)
		pinMode(led_pin,OUTPUT);

}
//---------------------------------------------------------------------------------------
MLinkEx::~MLinkEx()
{
	TIMSK1 &= ~_BV(TOIE1);
}
//---------------------------------------------------------------------------------------
bool MLinkEx::begin(byte ser)
{

	static long int zeit_i=micros(),zeitmess_i=micros();
	static byte index_i=0;
	

	
		if(ser==SW_SERIAL)
			swSerial.begin(38400);
		else if(ser==HW_SERIAL0)
			Serial.begin(38400);
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)			
		else if(ser==HW_SERIAL1)
			Serial1.begin(38400);
		else if(ser==HW_SERIAL2)
			Serial2.begin(38400);
		else if(ser==HW_SERIAL3)
			Serial3.begin(38400);
#endif
		if(ser<=SW_SERIAL)	lok_ser=ser;
		else return false;

		if(ser==SW_SERIAL)
			;//while (!swSerial) { ; }
		else if(ser==HW_SERIAL0)
			while (!Serial) { ; }
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)			
		else if(ser==HW_SERIAL1)
			while (!Serial1) { ; }
		else if(ser==HW_SERIAL2)
			while (!Serial2) { ; }
		else if(ser==HW_SERIAL3)
			while (!Serial3) { ; }
#endif
			
		if(ser==SW_SERIAL)
			swSerial.setTimeout(ML_SENS_TIMEOUT);
		else if(ser==HW_SERIAL0)
			Serial.setTimeout(ML_SENS_TIMEOUT);
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)			
		else if(ser==HW_SERIAL1)
			Serial1.setTimeout(ML_SENS_TIMEOUT);
		else if(ser==HW_SERIAL2)
			Serial2.setTimeout(ML_SENS_TIMEOUT);
		else if(ser==HW_SERIAL3)
			Serial3.setTimeout(ML_SENS_TIMEOUT);
#endif
	//*/	

		if(led_pin >= 0)
		{
			pinMode(led_pin,OUTPUT);
			digitalWrite(led_pin,LOW);
		}	

		return true;	

}
//---------------------------------------------------------------------------------------
bool MLinkEx::act_servo( unsigned int* servo_i, unsigned char max)
{
        return false;
}
//---------------------------------------------------------------------------------------
bool MLinkEx::get()
{
	
	static byte lang = 15;
	static unsigned int inByte = 0; 
	int ersetzt = 0;
	static byte wertes[15], wertenib[30];
	static byte adressindex = 0;
	unsigned long int zeit=millis();
	
       /* Return.modus = 0x40;//wertenib[0];
        Return.adresse = 3;//wertenib[3];
        Return.alarm = 0;//wertenib[7] & 0x01; //Alarmbit unterstes Bit im nidrigsnten Nibbles
        Return.checksumme = 0;//(wertenib[10] << 4) + wertenib[11];
        Return.checksummewert = 0;//wertenib[6] + wertenib[7] + wertenib[8] + wertenib[9];
        Return.einheit_nr = 4;//wertenib[5];
        Return.ausgabewert = 10.0;*/

 
	if(lok_ser==SW_SERIAL)
		while(!swSerial.available()) 
		{
			if(millis()-zeit > ML_GET_TIMEOUT) 
				return false;
		}
	else if(lok_ser==HW_SERIAL0)
		while(!Serial.available()) 
		{
			if(millis()-zeit > ML_GET_TIMEOUT) 
				return false;
		}
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)		
	else if(lok_ser==HW_SERIAL1)
		while(!Serial1.available()) 
		{
			if(millis()-zeit > ML_GET_TIMEOUT) 
				return false;
		}
	else if(lok_ser==HW_SERIAL2)
		while(!Serial2.available()) 
		{
			if(millis()-zeit > ML_GET_TIMEOUT) 
				return false;
		}
	else if(lok_ser==HW_SERIAL3)
		while(!Serial3.available()) 
		{
			if(millis()-zeit > ML_GET_TIMEOUT) 
				return false;
		}		
#endif




 		if(lok_ser==SW_SERIAL)
			inByte = swSerial.readBytesUntil(0x02,wertes,(int)lang);//alle einlesen bis zum nächsten Start-Zeichen STX=0x02
 		else if(lok_ser==HW_SERIAL0)
			inByte = Serial.readBytesUntil(0x02,wertes,(int)lang);//alle einlesen bis zum nächsten Start-Zeichen STX=0x02
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__STM32F1__)			
		else if(lok_ser==HW_SERIAL1)
			inByte = Serial1.readBytesUntil(0x02,wertes,(int)lang);//alle einlesen bis zum nächsten Start-Zeichen STX=0x02
		else if(lok_ser==HW_SERIAL2)
			inByte = Serial2.readBytesUntil(0x02,wertes,(int)lang);//alle einlesen bis zum nächsten Start-Zeichen STX=0x02
		else if(lok_ser==HW_SERIAL3)
			inByte = Serial3.readBytesUntil(0x02,wertes,(int)lang);//alle einlesen bis zum nächsten Start-Zeichen STX=0x02
#endif
     /* for(int i = 0; i < inByte; i++)
      {
        Serial.print(" "); 
        Serial.print(i); 
        Serial.print(":"); 
        Serial.print((int)wertes[i],HEX); 
        Serial.print(" "); 
      }
      Serial.print(" Ende "); 
      Serial.print(lang,DEC); 
      Serial.print(" - "); 
      Serial.println(inByte,DEC); //*/
      		
            if(inByte >= 7 &&  inByte <= 15)
            {
                if ((wertes[1] >> 4 && 0x0F) <= 6) // ### alles über MLink-Adresse 6 ignorieren
                {

			if(led_pin >= 0)
			digitalWrite(led_pin,digitalRead(led_pin) ^1);
      //Testausgabe der Rohdaten

      
			for(int i = 0; i < 2*inByte; i+=2)
			{      
				wertenib[i] = (wertes[i/2] & 0xF0) >> 4;
				wertenib[i+1] = wertes[i/2] & 0x0F;
			}
      // filtern nach Nibble-Kombination "1B2" und durch 0 ersetzen 
      //(wird zur Vermeidung der Kombi "0x02"=STX und "0x03"=ETX verwendet, 
      //weil dies Anfang und Ende des Datenstroms definiert  
			ersetzt = 0;
			for(int i = 0; i < 2*inByte; i++) 
			{      
				if((wertenib[i] == 0x1)   && 
					(wertenib[i+1] == 0xB) && 
					(wertenib[i+2] == 0x2))
				{
					ersetzt++;
					wertenib[i] = 0;
					for(int i1 = i+1; i1 < 2*inByte-2; i1++)
					{
						wertenib[i1] = wertenib[i1+2];  
					}
				}      

			}
      //Testausgabe der Nibble-Rohdaten
      /*for(int i = 0; i < 2*inByte - ersetzt; i+=2)
      {
        Serial.print(" "); 
        Serial.print(i/2); 
        Serial.print(":"); 
        Serial.print((int)wertenib[i],HEX); 
        Serial.print(" "); 
        Serial.print((int)wertenib[i+1],HEX); 
        Serial.print(" "); 
      }//*/
      
        /*Serial.print((int)wertenib[6],HEX); 
        Serial.print((int)wertenib[7],HEX); 
        Serial.print(" - "); 
        Serial.print((int)wertenib[8],HEX);
        Serial.print((int)wertenib[9],HEX); 
        Serial.print(" - "); 
        Serial.print((int)wertenib[10],HEX);
        Serial.print((int)wertenib[11],HEX); 
        Serial.println(" "); //*/
        
        /*Serial.print(((((int)wertenib[6]) << 4) + (int)wertenib[7]) >> 1,DEC); 
        Serial.print(" . "); 
        Serial.print((((int)wertenib[8]) << 4) + (int)wertenib[9],DEC); 
        Serial.print(" . S:"); //*/
		
			Return.alarm_adresse = -1;
			Return.trigger = false;
			Return.reset = false;
			Return.prog = false;
			Return.modus = 0;
			Return.adresse = 0;
			Return.alarm = 0; //Alarmbit unterstes Bit im nidrigsnten Nibbles
			Return.checksumme = 0;
			Return.checksummewert = 0;
			Return.checksummeres = 0;
			Return.einheit_nr = -1;
			Return.einheit = "no Data ";
//			-1b 0x02			
//	0	1	0b  0x40
//	2	3	1b	0x40 + adresse
//	4	5	2b	0x30 + einheit_nr
//	6	7	3b	lb
//	8	9	4b	hb
//	10	11	5b	cs
//	12	13	6b	0x03

		
			if(wertenib[5] >= 0x00)
			{
				Return.modus = wertenib[0];
				Return.adresse = wertenib[3];
				Return.alarm = wertenib[7] & 0x01; //Alarmbit unterstes Bit im nidrigsnten Nibbles
				Return.checksumme = (wertenib[10] << 4) + wertenib[11];
				Return.checksummewert =((wertenib[6]<<4) | wertenib[7]) + ((wertenib[8]<<4) | wertenib[9]);
				Return.checksummeres =0xFF & (Return.checksumme | ~Return.checksummewert); //muss CRC (0x16) ergeben
				Return.einheit_nr = wertenib[5];

				byte prog_index=(wertenib[0]<<4)+wertenib[1];
				if(prog_index >= 0x70)
				{
					Return.trigger = false;
					Return.alarm_adresse = prog_index-0x70;
					Return.prog = true;
					if(Return.einheit_nr == 0) Return.reset = true;
					else Return.reset = false;
				}
				else if(prog_index >= 0x50)
				{
					Return.trigger = true;
					Return.alarm_adresse = prog_index-0x50;
					Return.prog = true;
					if(Return.einheit_nr == 0) Return.reset = true;
					else Return.reset = false;
				}
				else if(prog_index <= 0x40)
				{
					Return.trigger = false;
					Return.alarm_adresse = -1;
					Return.prog = false;
					Return.reset = false;
				}


//#ifdef NOTEXT				
				switch(Return.einheit_nr)
				{
					case 0x00 : Return.einheit = F("--");      Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x01 : Return.einheit = F("V");       Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x02 : Return.einheit = F("A");       Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x03 : Return.einheit = F("m/s");     Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x04 : Return.einheit = F("km/h");    Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x05 : Return.einheit = F("1/min");   Return.faktor = 100.0; Return.stelle = 0; break;
					case 0x06 : Return.einheit = F("Grd.C");   Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x07 : Return.einheit = F("Grd.");    Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x08 : Return.einheit = F("m");       Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x09 : Return.einheit = F("%Kap.");   Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x0A : Return.einheit = F("%RSSI");   Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x0B : Return.einheit = F("mAh");     Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x0C : Return.einheit = F("ml");      Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x0D : Return.einheit = F("km");      Return.faktor =   0.1; Return.stelle = 1; break;
					case 0x0E : Return.einheit = F("hPa");     Return.faktor =   1.0; Return.stelle = 0; break;
					case 0x0F : Return.einheit = F("??");      Return.faktor =   1.0; Return.stelle = 0; break;
					default   : Return.einheit = F("...");     Return.faktor =   1.0; Return.stelle = 0; break;  
				}
//#endif				
			}
			if(wertenib[8] < 0x8)
				Return.ausgabewert = (float)((((unsigned short int)(((((unsigned short int)wertenib[6]<<4)+(unsigned short int)wertenib[7]) +
                                   ((((unsigned short int)(0x7 &  wertenib[8])<<4)+(unsigned short int)wertenib[9]) << 8)) ) >> 1))) *Return.faktor;// 
			else
			   Return.ausgabewert = -(float)((((unsigned short int)(((((unsigned short int)wertenib[6]<<4)+(unsigned short int)wertenib[7]) +
                                   ((((unsigned short int)(0x7 &  wertenib[8])<<4)+(unsigned short int)wertenib[9]) << 8)) ) >> 1))) *Return.faktor;// 
			Return.intausgabewert = (signed short int)((Return.ausgabewert) / Return.faktor);

			//Hinweis: erst intausgabewert und dann ausgabewert=intausgabewert*faktor führt zu einer Fehlermeldung im Hardwareseriell-Modul!
//#ifdef NOTEXT			
			if(Return.einheit_nr == 0)
			{
				Return.errorcode=(((short int)wertenib[6]<<4)+(short int)wertenib[7])>>1;
		 	    switch(Return.errorcode)
				{
					  case 0 :  Return.error = F("-OFF-");        break;
					  case 1 :  Return.error = F("Stby/START");   break;
					  case 2 :  Return.error = F("Ignite...");    break; 
					  case 3 :  Return.error = F("acceler.");     break; 
					  case 4 :  Return.error = F("Stabilise");    break; 
					  case 5 :  Return.error = F("LearnHI");      break; 
					  case 6 :  Return.error = F("LearnLO");      break; 
					  case 7 :  Return.error = F("RUN...");       break; 
					  case 8 :  Return.error = F("SlowDown");     break; 
					  case 9 :  Return.error = F("Manual");       break; 
					  case 10 : Return.error = F("SwitchOff");    break; 
					  case 11 : Return.error = F("RUN(reg.)");    break; 
					  case 12 : Return.error = F("AccelrDly");    break; 
					  case 13 : Return.error = F("SpeedCtrl");    break; 
					  case 14 : Return.error = F("Rpm2Ctrl");     break; 
					  case 15 : Return.error = F("PreHeat1");     break; 
					  case 16 : Return.error = F("PreHeat2");     break; 
					  case 17 : Return.error = F("MainFStrt");    break; 
					  case 18 : Return.error = F("--");           break; 
					  case 19 : Return.error = F("Keros.FullOn"); break;  
					  case 20 : Return.error = F("--");           break; 
					  case 21 : Return.error = F("RC-Off");       break; 
					  case 22 : Return.error = F("OverTemp");     break;  
					  case 23 : Return.error = F("IgnTimOut");    break; 
					  case 24 : Return.error = F("AccTimOut");    break; 
					  case 25 : Return.error = F("Acc.Slow");     break; 
					  case 26 : Return.error = F("Over-Rpm");     break;  
					  case 27 : Return.error = F("Low-Rpm");      break;   
					  case 28 : Return.error = F("BattryLow");    break; 
					  case 29 : Return.error = F("Auto-Off");     break;  
					  case 30 : Return.error = F("LowTemp");      break;   
					  case 31 : Return.error = F("HiTempOff");    break; 
					  case 32 : Return.error = F("GlowPlug");     break; 
					  case 33 : Return.error = F("WatchDog");     break;  
					  case 34 : Return.error = F("FailSafe");     break;  
					  case 35 : Return.error = F("Manual");       break; 
					  case 36 : Return.error = F("PowerFai");     break; 
					  case 37 : Return.error = F("TempFail");     break;  
					  case 38 : Return.error = F("FuelFail");     break;  
					  case 39 : Return.error = F("2nd EngF");     break; 
					  case 41 : Return.error = F("2nd Diff");     break; 
					  case 42 : Return.error = F("2nd Comm");     break; 
					  case 43 : Return.error = F("No-OIL");       break;   
					  case 44 : Return.error = F("OverCurr");     break; 
					  case 45 : Return.error = F("No Pump!");     break; 
					  case 46 : Return.error = F("WrongPmp");     break; 
					  case 47 : Return.error = F("-ON-");         break; 
					  case 48 : Return.error = F("Enabled");      break; 
					  case 49 : Return.error = F("Disabled ");    break; 
					  case 50 : Return.error = F("Prop-Fail");    break; 
					  case 51 : Return.error = F("Cooling");      break; 				
					  default : Return.error = F("no Error");
				 }			  
 				
			 }
			 else Return.error = "no Error";//*/
//#endif			
                    }
                    else return false;
                }
                else return false;

		return true;

  
}

