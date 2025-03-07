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

//#define Example_0   // uncomment this line for debugging
#define Example_1     // comment this line for debugging


#include "FrSkySportSensor.h"
#include "FrSkySportSensorTransferTX.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"
#include "SoftwareSerial.h"


FrSkySportSensorTransferTX transfertx;                 // Create TransferTx sensor with default ID
FrSkySportTelemetry telemetry;                         // Create Variometer telemetry object

uint32_t txd1 = 0;                                     // For debugging (example 0), you can define a value to transfer here
uint32_t txd2 = 0;                                     // comment out the #define Example_x lines

// This array maps the input pins. Your can reduce the number of uses pins or change the pins.
byte myPins[] = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20, 21};   //A0=14 A1=15 A2=16 A3=17 A6=20 A7=21
byte pincount = sizeof(myPins);
byte pincount_low, pincount_high;

//--------------------------------------------------------------------------------------- 

void setup()
{
  telemetry.begin(FrSkySportSingleWireSerial::SERIAL_1, &transfertx);

  #ifdef Example_0
    Serial.begin(115200); 
    //uint32_t currentTime, displayTime;  
  #endif


  #ifdef Example_1 
    for (byte ii = 0; ii < pincount; ii++) {       // Set the selected pins as an input and set the pullup resistor
        pinMode(myPins[ii],INPUT_PULLUP);
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

// Set TRANSFERTX sensor data

  #ifdef Example_1
  
   txd2 = 0 ;                                         // Clears the word to transfer
   
   for (byte ii = 0; ii < pincount_low; ii++) {       // fill the bits 0 up to 15
      txd2 |= (digitalRead(myPins[ii]) << ii);    
   }

    txd2 = txd2 << 16;                                // it is not possible to shift more than 32 bits, so we have to split 2x 16bit

   for (byte ii = 0; ii < pincount_high; ii++) {      // fill the bits 16 up to 31
      txd2 |= (digitalRead(myPins[ii+16]) << ii);      
   }      
   
  #endif  

  
  transfertx.setData(txd1, txd2);   // txd1 txd2              
  telemetry.send();
}
