/*
  Author: flarssen, 10/2015
  Vario sensor with accelometer for FrSky S-port and HUB Telemetry protocol,
  using: 
  FrSky S-Port Telemetry library http://www.rcgroups.com/forums/showthread.php?t=2245978
  Code for MS5611 froam OpenXsensor project:  https://github.com/openXsensor/openXsensor    
*/
#include "Wire.h"
#include "SPI.h"
#include "I2Cdev.h"
#include "MPU60X0.h"
#include <SoftwareSerial.h>
#include "config.h"
#include "ms5611.h"
#include "FrSkySportSensor.h"
#include "FrSkySportSensorVario.h"
#include "FrSkySportSensorAcc.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"

#define NO_PIN 255        
#define LED_PIN 13                     // the pin connected to onboard LED
#define PIN_SerialTelemetryTX 12       // the pin to use for serial data to the FrSky receiver
#define I2C_MS5611_Add 0x7
#define DDELAY 125
#define XDELAY 50
#define calRangeX 16.300                // Adjust value to get 2.00G total range when axis points to earth in either direction 
#define calRangeY 16.384                // Adjust value to get 2.00G total range when axis points to earth in either direction
#define calRangeZ 16.550                // Adjust value to get 2.00G total range when axis points to earth in either direction#define calOffsetX 0                    // Zero G error devided by calRangeX
#define calOffsetX -245                 // Zero G error devided by 16384
#define calOffsetY 100                  // Zero G error devided by 16384
#define calOffsetZ 1325                 // Zero G error devided by 16384 
      
SoftwareSerial serialD(NO_PIN, PIN_SerialTelemetryTX, true);
SoftwareSerial telemetryTypeCheck(PIN_SerialTelemetryTX, NO_PIN, true); // Tx pin will be input
FrSkySportSensorVario vario;            // Sport Vario sensor
FrSkySportSensorAcc acc;                // Accelerometer sensor
FrSkySportTelemetry telemetry;          // Sport telemetry object
MS5611 MS5611(I2C_MS5611_Add);
MPU60X0 accelgyro(false, 0x68);

boolean telemetryTypeX;
byte X[] = {3,1,1,3,0};     // -..-
byte D[] = {3,1,1,0};       // -..
uint32_t lastTime, currentTime;
int16_t ax, ay, az;
int16_t gx, gy, gz;
double avgAcc_X, avgAcc_Y, avgAcc_Z; 

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
  SendDValue(0x10, (uint16_t)(MS5611.varioData.absoluteAlt / 100));
  SendDValue(0x21, (uint16_t)(MS5611.varioData.absoluteAlt % 100));
  SendDValue(0x30, (uint16_t)MS5611.varioData.climbRate);
  SendDValue(0x24, (int16_t)((avgAcc_X + calOffsetX)/calRangeX));
  SendDValue(0x25, (int16_t)((avgAcc_Y + calOffsetY)/calRangeY));
  SendDValue(0x26, (int16_t)((avgAcc_Z + calOffsetZ)/calRangeZ));
  serialD.write(0x5E); // terminate frame 
}

void setup()
{
  pinMode(LED_PIN, OUTPUT); 
  telemetryTypeX = isTelemetryTypeX();  
  if (telemetryTypeX) {
      telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_12, &vario, &acc);
    morse(X);
  }
  else {
    serialD.begin(9600);
    morse(D);
  }
  MS5611.setup();
  accelgyro.initialize();
  avgAcc_X = avgAcc_Y = avgAcc_Z = 0.0f;
  currentTime = lastTime = millis();
}

void loop() {
  MS5611.readSensor(); // Read pressure & temperature on MS5611, calculate Altitude and vertical speed
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  avgAcc_X = ax*0.2 + avgAcc_X*0.8;
  avgAcc_Y = ay*0.2 + avgAcc_Y*0.8;
  avgAcc_Z = az*0.2 + avgAcc_Z*0.8;
  currentTime = millis();
  if (telemetryTypeX) {
    if(currentTime-lastTime > XDELAY) {
      digitalWrite(LED_PIN, HIGH);
      vario.setData((float)(MS5611.varioData.absoluteAlt)/100,  // Altitude in Meters (can be negative)
                     (float)(MS5611.varioData.climbRate)/100);  // Vertical speed in m/s (positive - up, negative - down)
      acc.setData(((avgAcc_X + calOffsetX)/calRangeX)/10, ((avgAcc_Y + calOffsetY)/calRangeY)/10, ((avgAcc_Z + calOffsetZ)/calRangeZ)/10);            
      lastTime = currentTime;
      digitalWrite(LED_PIN, LOW);
    }
  }
  else {
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

