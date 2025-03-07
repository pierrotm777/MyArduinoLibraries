/*
  Author: flarssen, 08/2015
  Vario sensor for FrSky S-port and HUB Telemetry protocol,
  using: 
  FrSky S-Port Telemetry library http://www.rcgroups.com/forums/showthread.php?t=2245978
  Code for MS5611 froam OpenXsensor project:  https://github.com/openXsensor/openXsensor   
*/
#include <SoftwareSerial.h>
#include "config.h"
#include "ms5611.h"

#include "FrSkySportSensor.h"
#include "FrSkySportSensorVario.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"

#define NO_PIN 255        
#define LED_PIN 13                     // the pin connected to onboard LED
#define PIN_SerialTelemetryTX 12       // the pin to use for serial data to the FrSky receiver
#define I2C_MS5611_Add 0x77
#define DDELAY 125

SoftwareSerial serialD(NO_PIN, PIN_SerialTelemetryTX, true);
SoftwareSerial telemetryTypeCheck(PIN_SerialTelemetryTX, NO_PIN, true); // Tx pin will be input
FrSkySportSensorVario vario;            // Sport Vario sensor
FrSkySportTelemetry telemetry;          // Sport telemetry object
MS5611 MS5611(I2C_MS5611_Add);
  

boolean telemetryTypeX;
byte X[] = {3,1,1,3,0};     // -..-
byte D[] = {3,1,1,0};       // -..
uint32_t lastTime, currentTime;

void morse(byte values[])
{
  for(int i=0; values[i]; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(values[i]*100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  delay(300);
}  

boolean isTelemetryTypeX(void) {
  unsigned long startTime;
  boolean x = false;
  
  telemetryTypeCheck.begin(57600);
  startTime = millis();
  while (millis() - startTime < 1000UL) {
    while (telemetryTypeCheck.available()) {
      if(telemetryTypeCheck.read() == FRSKY_TELEMETRY_START_FRAME) {
        x = true;
        break;
      }
    }
  }
  telemetryTypeCheck.end();
  pinMode(PIN_SerialTelemetryTX, OUTPUT);
  return x;
}

void SendDValue(uint8_t ID, uint16_t Value) {
  uint8_t lsb = Value & 0x00ff;
  uint8_t msb = (Value & 0xff00)>>8;
  serialD.write(0x5E);
  serialD.write(ID);
  if(lsb == 0x5E) {
    serialD.write(0x5D);
    serialD.write(0x3E);
  }
  else if(lsb == 0x5D) {
    serialD.write(0x5D);
    serialD.write(0x3D);
  }
  else {
    serialD.write(lsb);
  }
  if(msb == 0x5E) {
    serialD.write(0x5D);
    serialD.write(0x3E);
  }
  else if(msb == 0x5D) {
    serialD.write(0x5D);
    serialD.write(0x3D);
  }
  else {
    serialD.write(msb);
  }
}

void SendDData(void) {
  SendDValue(0x10, (uint16_t)(MS5611.varioData.absoluteAlt/100));
  SendDValue(0x21, (uint16_t)(MS5611.varioData.absoluteAlt%100));
  SendDValue(0x30, (uint16_t)MS5611.varioData.climbRate);
  serialD.write(0x5E); // terminate frame 
  serialD.write(0x0d);  
  serialD.flush();
}

void setup()
{
  pinMode(LED_PIN, OUTPUT); 
  telemetryTypeX = isTelemetryTypeX();  
  if (telemetryTypeX) {
    telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_12, &vario);
    morse(X);
  }
  else {
    serialD.begin(9600);
    morse(D);
  }
//  Serial.begin(115200);
  MS5611.setup();
  currentTime = lastTime = millis();
}

void loop() {
  MS5611.readSensor(); // Read pressure & temperature on MS5611, calculate Altitude and vertical speed
  if (telemetryTypeX) {
    digitalWrite(LED_PIN, HIGH);
    vario.setData((float)(MS5611.varioData.absoluteAlt)/100,  // Altitude in Meters (can be negative)
                   (float)(MS5611.varioData.climbRate)/100);  // Vertical speed in m/s (positive - up, negative - down)
    digitalWrite(LED_PIN, LOW);
  }
  else {
    currentTime = millis();
    if(currentTime-lastTime > DDELAY) {
      digitalWrite(LED_PIN, HIGH);
      SendDData();
      lastTime = currentTime;
      digitalWrite(LED_PIN, LOW);
    } 
  } 
  if (telemetryTypeX){
    telemetry.send();
  }
}

