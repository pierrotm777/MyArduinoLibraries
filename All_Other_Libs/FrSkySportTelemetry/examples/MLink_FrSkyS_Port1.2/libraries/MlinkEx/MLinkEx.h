/*MSB-Protokoll:
Empfänger ist Master, jder Sensor Slave.
Master sendet ID 0..15 (ACT 2..15)
Innerhalb von 256µs muss Slafe darauf 3 Byte senden 
Slave-Byts:
1. HighNibble  Adresse, LowNibble Einheit ID (0..15)
2. LowByte Daten: obere 7 bit Daten, letztes Bit Alarm-Flag (def 0)
3. Byte High-byte

Rx liest sowohl Adress-Daten vom Master als auch Slave-Tx-Daten
SerialX.readBytes ist nicht anwendbar wenn das Lesen der Master-Daten über Timer-Interrupt 
getriggert werden soll (Interrupt-Konflikt) SerialX.read läuft aber.
Daher wird nur jedes 4 eingelesene Byte als Adressen ausgewertet

*/
#ifndef MLinkEx_h
#define MLinkEx_h

#include "Arduino.h"
//#include "SomeSerial.h"
#include <SoftwareSerial.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#define RESOLUTION 65536    // Timer1 is 16 bit

   //Einheit 1=V, 2=A, 3=m/s Steigen, 4=km/h, 5=1/min, 6=*C,
  //          7=*, 8=m, 9=%Sprit, 10=%Signal, 11=mAh, 12=ml, 13=km, 14=hPa, 15=tbd
#define ML_KD     0
#define ML_V      1
#define ML_A      2
#define ML_MpS    3
#define ML_KMpH   4
#define ML_UpMIN  5
#define ML_GRADC  6
#define ML_GRAD   7
#define ML_M      8
#define ML_VHKAP  9
#define ML_VHSIG 10
#define ML_MAH   11
#define ML_ML    12
#define ML_KM    13
#define ML_HPA   14 
#define ML_TBD15 15

#define ML_GET_TIMEOUT 6 //in ms
#define ML_SENS_TIMEOUT 500 //in ms
#define ML_LCD_GRAD 0xDF
#define MAXKANAL 16

#define HW_SERIAL0	0
#define HW_SERIAL1	1
#define HW_SERIAL2	2
#define HW_SERIAL3	3
#define SW_SERIAL	4

#define CRC 0x16




class MLinkEx
{
   public:
    
    MLinkEx(int pin);
	~MLinkEx();
	float werte(float wert, char einheit_i, char index_i,bool alarm_b=false);
	bool begin(byte ser);
	bool get();
	byte adresse(byte adr=0);
	bool act_servo(unsigned int* servo_i, unsigned char max);
	void senden();
	//+++++++++++
    /*Stream* port;

    enum SerialId { SOFT_SERIAL_PIN_2 = 2, SOFT_SERIAL_PIN_3 = 3, SOFT_SERIAL_PIN_4 = 4, SOFT_SERIAL_PIN_5 = 5, SOFT_SERIAL_PIN_6 = 6, SOFT_SERIAL_PIN_7 = 7,
                     SOFT_SERIAL_PIN_8 = 8, SOFT_SERIAL_PIN_9 = 9, SOFT_SERIAL_PIN_10 = 10, SOFT_SERIAL_PIN_11 = 11, SOFT_SERIAL_PIN_12 = 12 };
    SoftwareSerial* softSerial;
    SerialId softSerialId;//*/
	 //+++++++++++
	struct
	{ 
		char  alarm_adresse;
		bool  trigger;
		bool  reset;
		bool  prog;
		float ausgabewert;
		signed short int   intausgabewert;
		String error;
		char   errorcode;
		String einheit;
		char  einheit_nr;
		float faktor;
		char  stelle;
		char  alarm;
		char  adresse;
		char  modus;
		char  checksumme;
		char  checksummewert;
	    char  checksummeres;
	} Return;
	
   private:
	 int led_pin;
	 bool alarm_flag_b[16];
	 float wert(char aktion_i, float wert,  char index_i);
	 byte lok_ser;
};

#endif