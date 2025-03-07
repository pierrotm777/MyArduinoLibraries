/*
  FrSky S-Port Telemetry library example for demonstrating the FrSkySportTransferTX class
  (c) Pawelsky 20150725
  Modified by Andreas_
  Not for commercial use
  
  Note that you need Teensy 3.x or 328P based (e.g. Pro Mini, Nano, Uno) board and FrSkySportTelemetry library for this example to work
*/


/* Example_1 is routing up to 19 inputs of an arduino connected to the S.Port pin of the Taranis 
 *  to a second arduino connected to the S.Port connector at the receiver.
 *  The following inputs are transfered to the coresponding outputs:
 *  Digital pin 0 to 11, digital pin 13 and analog pin A0 to A5 (as a digital signal)
 *  Just wire a switch to ground at all the inputs you want to use.
 *  Wire pin12 to the S.Port like shown in the diagram.
 *  txd1 is not used in this example, so you have 32bits on spare to play with
 *  
 *  Example_0 is for debugging. You will get the defined values of txd1 and txd2 at the serial port of the receiver side
 */

//#define Example_0     // uncomment this line for debugging
#define Example_1       // comment this line for debugging


#include "FrSkySportSensor.h"
#include "FrSkySportSensorTransferTX.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportDecoder.h"
#include "SoftwareSerial.h"

   
FrSkySportSensorTransferTX transfertx;                    
FrSkySportDecoder decoder;

uint32_t txd1, txd2; 

// This array maps the output pins. Paste a copy from the transmitter side. If necessary you can change the order.
byte myPins[] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};   //A0=14 A1=15 A2=16 A3=17 A4=18 A5=19
byte pincount = sizeof(myPins);
byte pincount_low, pincount_high;
uint32_t currentTime, displayTime; 

//--------------------------------------------------------------------------------------- 

void setup()
{
  decoder.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_2, &transfertx);
  
  #ifdef Example_0
    Serial.begin(9600);  
  #endif

  #ifdef Example_1 
    for (byte ii = 0; ii < pincount; ii++) {       // Set the selected pins as an output
        pinMode(myPins[ii],OUTPUT);
        digitalWrite(myPins[ii],LOW);
        delay(100);
    }
    delay(500);
    for (byte ii = 0; ii < pincount; ii++) {       // Set the selected pins as an output
        digitalWrite(myPins[ii],HIGH);
        delay(100);
    }
    if (pincount > 15){
      pincount_low = 16;
      pincount_high = pincount - 16;  
    }
    else {
      pincount_low = pincount;
      pincount_high = 0;
    }
  #endif
}

//--------------------------------------------------------------------------------------- 

void loop()
{
  // Call this on every loop
  decoder.decode();
  txd1 = transfertx.gettxd1();
  txd2 = transfertx.gettxd2();


  #ifdef Example_0
    // Display data once a second to not interfeere with data decoding
    currentTime = millis();
    if(currentTime > displayTime)
    {
      displayTime = currentTime + 1000;
    
      Serial.print("txd1 = "); Serial.println(transfertx.gettxd1());   
      Serial.print("txd2 = "); Serial.println(transfertx.gettxd2(), BIN);                           
    }
  #endif


  #ifdef Example_1

   for (byte ii = 0; ii < pincount_high; ii++) {
      if (txd2 & (1 << ii)) {
         digitalWrite(myPins[ii+16],LOW);
      }
      else {
         digitalWrite(myPins[ii+16],HIGH);
      }
   }    
          
    txd2 = txd2 >> 16;
  
   for (byte ii = 0; ii < pincount_low; ii++) {
      if (txd2 & (1 << ii)) {
         digitalWrite(myPins[ii],LOW);
      }
      else {
         digitalWrite(myPins[ii],HIGH);
      }
   }
  #endif  
}



