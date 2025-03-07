/*********************************************************************
  Light Control
  
  Commande d'éclairage pour maquette de bateau. Les situations lumineuses des LED individuelles sont contrôlées et appelées via des signaux.
  
  Light control for rc model boat. 
  
  CC-BY SA 2020 Kai Laborenz
*********************************************************************/

// FrSky S.Port Sensor Library
#include "FrSkySportSensor.h"
#include "FrSkySportSensorAss.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetryReader.h"
#include "SoftwareSerial.h"

bool module1onOff[16];
bool module2onOff[16];

unsigned long prevI2CpollingTime = 0;

// Sensor definition to initialize telemetry
FrSkySportSensorAss ass(FrSkySportSensor::ID14); //create a sensor
FrSkySportTelemetryReader telemetry; //create a telemetry object


uint8_t uiValue[4] = { 0, 0, 0, 0 };
bool bValuesUpdated = false;
int iValToggleSwitch = 0;


int timer = 1000;           // The higher the number, the slower the timing.

int navLights[6] = {0,0,0,0,0,0};   // Array of 6 navigation (colreg) light on observation bridge port side
int posLights[2] = {0,0};           // position lights on both sides of the main bridge (na)
int topLights[2] = {0,0};           // 2 front facing toplights on observation bridge
int backLights[2] = {0,0};          // 2 front facing toplights on observation bridge (na)

int searchLight = 0;                // front facing search light on observation bridge
int radar = 0;

const int offLights[6] = {0,0,0,0,0,0};   // all lights off
const int onLights[6] = {1,1,1,1,1,1};    // all lights on
const int diveLights[6] = {1,0,1,0,1,0};  // pattern for signal "diver in water": red-white-red


void printBinary(byte b) {
  for (int i = 7; i >= 0; i-- )
  {
    Serial.print((b >> i) & 0X01);//shift and select first bit
  }
  Serial.println();
}



void setup() {

  // configure telemetry serial port
  telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_12, &ass);
  
  prevI2CpollingTime = millis();
  
  Serial.begin(9600);
  
  // use a for loop to initialize each pin as an output:
  for (int thisPin = 2; thisPin < 12; thisPin++) {
    pinMode(thisPin, OUTPUT);
  }

  Serial.println("Start");

  
}



void loop() {

 //receive telemetry data
  telemetry.receive();  


  if (telemetry.available && ((uiValue[0] != telemetry.aUiRxValue[0])
                           || (uiValue[1] != telemetry.aUiRxValue[1])
                           || (uiValue[2] != telemetry.aUiRxValue[2])
                           || (uiValue[3] != telemetry.aUiRxValue[3]))) {
    uiValue[0] = telemetry.aUiRxValue[0];
    uiValue[1] = telemetry.aUiRxValue[1];
    uiValue[2] = telemetry.aUiRxValue[2];
    uiValue[3] = telemetry.aUiRxValue[3];

    telemetry.available = false;
    bValuesUpdated = true;
  }

  // update display and output
    if (bValuesUpdated) {    
    bValuesUpdated = false;

      //Serial.print(uiValue[0]);
      printBinary(uiValue[0]);
      Serial.print(" ");
      printBinary(uiValue[1]);
      //Serial.print(uiValue[1]);
      Serial.print(" ");
      printBinary(uiValue[2]);
      //Serial.print(uiValue[2]);
      Serial.print(" ");
      printBinary(uiValue[3]);
      //Serial.println(uiValue[3]);
      
    // cruise  
if (bitRead(uiValue[0],0)) { 
    Serial.println("Cruise");
    Serial.println(" ");
      memcpy (navLights, offLights, sizeof navLights);
      memcpy (posLights, onLights, sizeof posLights);
      memcpy (topLights, onLights, sizeof topLights);
      memcpy (backLights, onLights, sizeof backLights);
      searchLight = 0; }
      else {
      memcpy (posLights, offLights, sizeof posLights);
      memcpy (topLights, offLights, sizeof topLights);
      memcpy (backLights, offLights, sizeof backLights);
      }

    // anchor  
if (bitRead(uiValue[0],1)) { 
    Serial.println("Anchor");
    Serial.println(" ");
      memcpy (navLights, offLights, sizeof navLights);
      memcpy (posLights, offLights, sizeof posLights);
      memcpy (topLights, offLights, sizeof topLights);
      memcpy (backLights, offLights, sizeof backLights);
      searchLight = 0;
}

    // diver operations  
if (bitRead(uiValue[0],2)) { 
    Serial.println("Diving");
    Serial.println(" ");
      memcpy (navLights, diveLights, sizeof navLights);
      memcpy (posLights, onLights, sizeof posLights);
      //memcpy (topLights, onLights, sizeof topLights);
      memcpy (backLights, onLights, sizeof backLights);
      searchLight = 0; }
    else {
      memcpy (navLights, offLights, sizeof navLights);
    }

    // test (all lights on)
if (bitRead(uiValue[0],3)) { 
    Serial.println("Testing");
    Serial.println(" ");
      memcpy (navLights, onLights, sizeof navLights);
      memcpy (posLights, onLights, sizeof posLights);
      memcpy (topLights, onLights, sizeof topLights);
      memcpy (backLights, onLights, sizeof backLights);
      searchLight = 1; }
    else {
      memcpy (navLights, offLights, sizeof navLights);
      memcpy (posLights, offLights, sizeof posLights);
      memcpy (topLights, offLights, sizeof topLights);
      memcpy (backLights, offLights, sizeof backLights);
      searchLight = 0; 
    }


    // search light
if (bitRead(uiValue[0],4)) { 
    Serial.println("Searchlight");
    Serial.println(" ");
    searchLight = 1;
    }
    else {
      searchLight = 0;
    }

    // radar
if (bitRead(uiValue[0],5)) { 
    Serial.println("Radar");
    Serial.println(" ");
      radar = 1;
      analogWrite(11,75);
    }
    else {
      radar = 0;
      analogWrite(11,0);
    }
  
  Serial.println("Navlights");
  for (int thisPin = 2; thisPin < 8; thisPin++) {
    digitalWrite(thisPin,navLights[thisPin-2]);
    Serial.print(thisPin);
    Serial.print(": ");
    Serial.println(navLights[thisPin-2]);
  }

    Serial.println("Toplights");
    for (int thisPin = 8; thisPin < 10; thisPin++) {
    digitalWrite(thisPin,topLights[thisPin-8]);
    Serial.print(thisPin);
    Serial.print(": ");
    Serial.println(topLights[thisPin-8]);
  }
    
    Serial.print("Searchlight: ");
    Serial.println(searchLight);
    digitalWrite(10,searchLight);

 }
}
