#include "SK33AccessPoint.h"
#include "SK33WebServer.h"
#include "SK33RcvData.h"

// I2C library
#include <Wire.h>

// Library für display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// PWM modules library
#include <Adafruit_PWMServoDriver.h>

// FrSky S.Port Sensor Library
#include "FrSkySportSensor.h"
#include "FrSkySportSensorAss.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetryReader.h"
#include "SoftwareSerial.h"

#define BLUELIGHT_SWITCH_TIME 50
#define WIPER_STEP_TIME 10
#define RX_WEB_TOGGLE_SWITCH 13
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates


struct PwmChannel {
  bool IsLED;
  uint16_t min;
  uint16_t max;
  uint16_t value;
  uint16_t txInterval;
  unsigned long lastTx;
};

PwmChannel Module1[] = {{  true,   0,  256, 0, 0,  0 },  //heck
            {  true,   0, 1024, 0, 0,  0 },  //heckschlepp
            {  true,   0,  128, 0, 0,  0 },  //navSB
            {  true,   0,  256, 0, 0,  0 },  //SBsigO
            {  true,   0, 1024, 0, 0,  0 },  //SBsigM
            {  true,   0,  128, 0, 0,  0 },  //SBsigU
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0, 1024, 0, BLUELIGHT_SWITCH_TIME, 0 },  //blau1
            {  true,   0, 1024, 0, BLUELIGHT_SWITCH_TIME, 0 },  //blau3
            {  true,   0, 1024, 0, BLUELIGHT_SWITCH_TIME, 0 }}; //blau5

PwmChannel Module2[] = {{  true,   0,  256, 0, 0,  0 },  //topp
            {  true,   0,  256, 0, 0,  0 },  //toppschlepp
            {  true,   0, 1024, 0, 0,  0 },  //navBB
            {  true,   0,  256, 0, 0,  0 },  //BBsigO
            {  true,   0, 1024, 0, 0,  0 },  //BBsigM
            {  true,   0,  256, 0, 0,  0 },  //BBsigU
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0, 1024, 0, BLUELIGHT_SWITCH_TIME, 0 },  //blau2
            {  true,   0, 1024, 0, BLUELIGHT_SWITCH_TIME, 0 },  //blau4
            {  true,   0, 1024, 0, BLUELIGHT_SWITCH_TIME, 0 }}; //blau6

PwmChannel Module3[] = {{  true,   0, 1024, 0, 0,  0 },  //Mastsucher
            {  true,   0, 1024, 0, 0,  0 },  //SucherBB
            {  true,   0, 1024, 0, 0,  0 },  //SucherSB
            { false, 250,  450, 0, 0,  0 },  //Radar
            {  true,   0, 1024, 0, 0,  0 },
            {  true,   0,  256, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            {  true,   0,  128, 0, 0,  0 },
            { false, 250,  450, 0, WIPER_STEP_TIME, 0 },  //Wischer1
            { false, 250,  450, 0, WIPER_STEP_TIME, 0 },  //Wischer2
            { false, 250,  450, 0, WIPER_STEP_TIME, 0 },  //Wischer3
            { false, 250,  450, 0, WIPER_STEP_TIME, 0 },  //Wischer4
            { false, 250,  450, 0, WIPER_STEP_TIME, 0 }}; //Wischer5

bool bluelightActive = false;
bool bWipersActive = false;
bool bWiper1Direction = true;
bool bWiper2Direction = true;
bool bWiper3Direction = true;
bool bWiper4Direction = true;
bool bWiper5Direction = true;

uint8_t bluelightNr = 1;

unsigned long prevBlueLightTime = 0;
unsigned long prevWiperStepTime = 0;

int iValToggleSwitch = 0;
int speedWiper1 = 2;
int speedWiper2 = 3;
int speedWiper3 = 4;
int speedWiper4 = 5;
int speedWiper5 = 2;

// Sensor definition to initialize telemetry
FrSkySportSensorAss ass(FrSkySportSensor::ID14); //create a sensor
FrSkySportTelemetryReader telemetry; //create a telemetry object

//initialize display
Adafruit_SSD1306 display;

// create PWM objects for LED and LED/Servo
Adafruit_PWMServoDriver pwmModule1 = Adafruit_PWMServoDriver(0x40, Wire);
Adafruit_PWMServoDriver pwmModule2 = Adafruit_PWMServoDriver(0x41, Wire);
Adafruit_PWMServoDriver pwmModule3 = Adafruit_PWMServoDriver(0x44, Wire);

SK33AccessPoint SK33ap;
SK33WebServer SK33webS;
SK33RcvData SK33data;

void setup() {
  pinMode(RX_WEB_TOGGLE_SWITCH, INPUT_PULLUP);

  // set I2C speed
  Wire.pins(4, 5);
  Wire.begin();
  Wire.setClock(400000);

  display = Adafruit_SSD1306(128, 32, &Wire); //create a display object

  // configure telemetry serial port
  telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_14, &ass);

  // initialize PWM objects
  pwmModule1.begin();
  pwmModule1.setPWMFreq(200);

  pwmModule2.begin();
  pwmModule2.setPWMFreq(200);

  pwmModule3.begin();
  pwmModule3.setPWMFreq(SERVO_FREQ);

  // initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display(); 

  prevBlueLightTime = millis();

  Serial.begin(115200);
  SK33ap.startWiFi();
  SK33webS.init();
  SK33webS.begin();
  //SK33data.Print();

  for (uint16_t i = 0; i < 16; i++) {
    pwmModule1.setPin(i, Module1[i].min, false);
    pwmModule2.setPin(i, Module2[i].min, false);
    pwmModule3.setPin(i, Module3[i].min, false);
  } //for 0-15 channels
  
  for (int i = 0; i < 16; i++) {
    Module1[i].lastTx = millis();
    Module2[i].lastTx = millis();
    Module3[i].lastTx = millis();
  } 
}

void Fahrt() {
  //Serial.println("Fahrt()");
  if ((SK33data.GetValue0() & 0x01) != 0) {
    display.setCursor(0, 16);
    display.print("F");

    Module1[0].value = Module1[0].max;//heck
    Module1[1].value = Module1[1].min;//heckSchlepp
    Module1[2].value = Module1[2].max;//navSB
    Module2[0].value = Module2[0].max;//topp
    Module2[1].value = Module2[1].min;//toppschlepp
    Module2[2].value = Module2[2].max;//navBB
  } 
  if ((SK33data.GetValue0() & 0x02) != 0) {
    display.setCursor(0, 16);
    display.print("F");
    display.setCursor(12, 16);
    display.print("S");       

    Module1[0].value = Module1[0].max;//heck
    Module1[1].value = Module1[1].max;//heckSchlepp
    Module1[2].value = Module1[2].max;//navSB
    Module2[0].value = Module2[0].max;//topp
    Module2[1].value = Module2[1].max;//toppschlepp
    Module2[2].value = Module2[2].max;//navBB
  }
  
  if (!((SK33data.GetValue0() & 0x01) != 0) && !((SK33data.GetValue0() & 0x02) != 0)) {
    Module1[0].value = Module1[0].min;//heck
    Module1[1].value = Module1[1].min;//heckSchlepp
    Module1[2].value = Module1[2].min;//navSB
    Module2[0].value = Module2[0].min;//topp
    Module2[1].value = Module2[1].min;//toppschlepp
    Module2[2].value = Module2[2].min;//navBB
  }
}

void Schleppfahrt() {
  if ((SK33data.GetValue0() & 0x02) != 0) {
    display.setCursor(0, 16);
    display.print("F");
    display.setCursor(12, 16);
    display.print("S");       

    Module1[0].value = Module1[0].max;//heck
    Module1[1].value = Module1[1].max;//heckSchlepp
    Module1[2].value = Module1[2].max;//navSB
    Module2[0].value = Module2[0].max;//topp
    Module2[1].value = Module2[1].max;//toppschlepp
    Module2[2].value = Module2[2].max;//navBB
  } else {
    Module1[0].value = Module1[0].min;//heck
    Module1[1].value = Module1[1].min;//heckSchlepp
    Module1[2].value = Module1[2].min;//navSB
    Module2[0].value = Module2[0].min;//topp
    Module2[1].value = Module2[1].min;//toppschlepp
    Module2[2].value = Module2[2].min;//navBB
  }
}

void Bluelight() {
  if ((SK33data.GetValue0() & 0x04) != 0) {
    display.setCursor(24, 16);
    display.print("B");
    bluelightActive = true;
  } else {
    bluelightActive = false;
    Module1[13].value = Module1[13].min;
    Module2[13].value = Module2[13].min;
    Module1[14].value = Module1[14].min;
    Module2[14].value = Module2[14].min;
    Module1[15].value = Module1[15].min;
    Module2[15].value = Module2[15].min;
  }
}

void calculateBluelight() {
  //calculate BlueLight
  if (bluelightActive && ((millis() - prevBlueLightTime) > BLUELIGHT_SWITCH_TIME)) {
  
    bluelightNr++;
    
    if(bluelightNr > 6) { bluelightNr = 1; }
    
    switch (bluelightNr) {
      case 1:
        Module2[15].value = Module2[15].min;
        Module1[13].value = Module1[13].max;
        break;
      case 2:
        Module1[13].value = Module1[13].min;
        Module2[13].value = Module2[13].max;
        break;
      case 3:
        Module2[13].value = Module2[13].min;
        Module1[14].value = Module1[14].max;
        break;
      case 4:
        Module1[14].value = Module1[14].min;
        Module2[14].value = Module2[14].max;
        break;
      case 5:
        Module2[14].value = Module2[14].min;
        Module1[15].value = Module1[15].max;
        break;
      case 6:
        Module1[15].value = Module1[15].min;
        Module2[15].value = Module2[15].max;
        break;
      default:
        break;
    }

    prevBlueLightTime = millis();
  }
}

void Radar() {
  if ((SK33data.GetValue0() & 0x10) != 0) {
    display.setCursor(60, 16);
    display.print("R");       
    Module3[3].value = Module3[3].max;
  } else {
    Module3[3].value = Module3[3].min;
  }
}

void Wipers() {
  if ((SK33data.GetValue0() & 0x40) != 0) {
    display.setCursor(36, 16);
    display.print("W");
    bWipersActive = true;
    } else {
    bWipersActive = false;
  }
}

void calculateWipers() {
  //calculate wipers
  if (bWipersActive && ((millis() - prevWiperStepTime) > WIPER_STEP_TIME)) {
    //wiper 1
    if (bWiper1Direction) { 
      Module3[11].value += speedWiper1;
      Module3[11].value = min(Module3[11].value, Module3[11].max);
    } else {
      Module3[11].value -= speedWiper1;
      Module3[11].value = max(Module3[11].value, Module3[11].min);
    }
    if (Module3[11].value == Module3[11].max) bWiper1Direction = false;
    if (Module3[11].value == Module3[11].min) bWiper1Direction = true;
    
    //wiper 2
    if (bWiper2Direction) { 
      Module3[12].value += speedWiper2;
      Module3[12].value = min(Module3[12].value, Module3[12].max);
    } else {
      Module3[12].value -= speedWiper2;
      Module3[12].value = max(Module3[12].value, Module3[12].min);
    }
    if (Module3[12].value == Module3[12].max) bWiper2Direction = false;
    if (Module3[12].value == Module3[12].min) bWiper2Direction = true;
    
    //wiper 3
    if (bWiper3Direction) { 
      Module3[13].value += speedWiper3;
      Module3[13].value = min(Module3[13].value, Module3[13].max);
    } else {
      Module3[13].value -= speedWiper3;
      Module3[13].value = max(Module3[13].value, Module3[13].min);
    }
    if (Module3[13].value == Module3[13].max) bWiper3Direction = false;
    if (Module3[13].value == Module3[13].min) bWiper3Direction = true;
    
    //wiper 4
    if (bWiper4Direction) { 
      Module3[14].value += speedWiper4;
      Module3[14].value = min(Module3[14].value, Module3[14].max);
    } else {
      Module3[14].value -= speedWiper4;
      Module3[14].value = max(Module3[14].value, Module3[14].min);
    }
    if (Module3[14].value == Module3[14].max) bWiper4Direction = false;
    if (Module3[14].value == Module3[14].min) bWiper4Direction = true;
    
    //wiper 5
    if (bWiper5Direction) { 
      Module3[15].value += speedWiper5;
      Module3[15].value = min(Module3[15].value, Module3[15].max);
    } else {
      Module3[15].value -= speedWiper5;
      Module3[15].value = max(Module3[15].value, Module3[15].min);
    }
    if (Module3[15].value == Module3[15].max) bWiper5Direction = false;
    if (Module3[15].value == Module3[15].min) bWiper5Direction = true;

    /*Serial.print(Module3[11].value);
    Serial.print(" ");
    Serial.print(Module3[12].value);
    Serial.print(" ");
    Serial.print(Module3[13].value);
    Serial.print(" ");
    Serial.print(Module3[14].value);
    Serial.print(" ");
    Serial.println(Module3[15].value);*/
      
    prevWiperStepTime = millis();
  }
}

void stopWipers() {
  if ((Module3[11].value != 0) && (Module3[12].value != 0) && (Module3[13].value != 0) && (Module3[14].value != 0) && (Module3[15].value != 0)) {
    //calculate wipers
    if ((millis() - prevWiperStepTime) > WIPER_STEP_TIME) {
      //wiper 1
      if (bWiper1Direction) { 
        Module3[11].value += speedWiper1;
        Module3[11].value = min(Module3[11].value, Module3[11].max);
      } else {
        Module3[11].value -= speedWiper1;
        Module3[11].value = max(Module3[11].value, Module3[11].min);
      }
      if (Module3[11].value == Module3[11].max) bWiper1Direction = false;
      
      //wiper 2
      if (bWiper2Direction) { 
        Module3[12].value += speedWiper2;
        Module3[12].value = min(Module3[12].value, Module3[12].max);
      } else {
        Module3[12].value -= speedWiper2;
        Module3[12].value = max(Module3[12].value, Module3[12].min);
      }
      if (Module3[12].value == Module3[12].max) bWiper2Direction = false;
      
      //wiper 3
      if (bWiper3Direction) { 
        Module3[13].value += speedWiper3;
        Module3[13].value = min(Module3[13].value, Module3[13].max);
      } else {
        Module3[13].value -= speedWiper3;
        Module3[13].value = max(Module3[13].value, Module3[13].min);
      }
      if (Module3[13].value == Module3[13].max) bWiper3Direction = false;
      
      //wiper 4
      if (bWiper4Direction) { 
        Module3[14].value += speedWiper4;
        Module3[14].value = min(Module3[14].value, Module3[14].max);
      } else {
        Module3[14].value -= speedWiper4;
        Module3[14].value = max(Module3[14].value, Module3[14].min);
      }
      if (Module3[14].value == Module3[14].max) bWiper4Direction = false;
      
      //wiper 5
      if (bWiper5Direction) { 
        Module3[15].value += speedWiper5;
        Module3[15].value = min(Module3[15].value, Module3[15].max);
      } else {
        Module3[15].value -= speedWiper5;
        Module3[15].value = max(Module3[15].value, Module3[15].min);
      }
      if (Module3[15].value == Module3[15].max) bWiper5Direction = false;

      /*Serial.print(Module3[11].value);
      Serial.print(" ");
      Serial.print(Module3[12].value);
      Serial.print(" ");
      Serial.print(Module3[13].value);
      Serial.print(" ");
      Serial.print(Module3[14].value);
      Serial.print(" ");
      Serial.println(Module3[15].value);*/     
      
      prevWiperStepTime = millis();
    }
  }
}

void Manoevrierbehindert() {
  if ((SK33data.GetValue1() & 0x01) != 0) {
    display.setCursor(48, 16);
    display.print("M");       

    Module1[3].value = Module1[3].max;//SBsignalO
    Module1[4].value = Module1[4].max;//SBsignalM
    Module1[5].value = Module1[5].max;//SBsignalU
    Module2[3].value = Module2[3].max;//BBsignalO
    Module2[4].value = Module2[4].max;//BBsignalM
    Module2[5].value = Module2[5].max;//BBsignalU
  } else {
    Module1[3].value = Module1[3].min;//SBsignalO
    Module1[4].value = Module1[4].min;//SBsignalM
    Module1[5].value = Module1[5].min;//SBsignalU
    Module2[3].value = Module2[3].min;//BBsignalO
    Module2[4].value = Module2[4].min;//BBsignalM
    Module2[5].value = Module2[5].min;//BBsignalU
  }
}

void loop () {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  iValToggleSwitch = digitalRead(RX_WEB_TOGGLE_SWITCH);

  if(iValToggleSwitch == HIGH) {
    // receive telemetry data
    telemetry.receive();  

    if (telemetry.available && ((SK33data.GetValue0() != telemetry.aUiRxValue[0])
                || (SK33data.GetValue1() != telemetry.aUiRxValue[1])
                || (SK33data.GetValue2() != telemetry.aUiRxValue[2])
                || (SK33data.GetValue3() != telemetry.aUiRxValue[3]))) {
      //set telemetry data to data blocks
      SK33data.SetValue0(telemetry.aUiRxValue[0]);
      SK33data.SetValue1(telemetry.aUiRxValue[1]);
      SK33data.SetValue2(telemetry.aUiRxValue[2]);
      SK33data.SetValue3(telemetry.aUiRxValue[3]);

      telemetry.available = false;
      SK33data.Changed();

      display.setCursor(0, 0);
      display.print("RX");
    }
  } else {
    SK33webS.handleClient();

    display.setCursor(0, 0);
    display.print("IC");
  }

  // update display and output
  if (SK33data.IsChanged()) {    
    Fahrt();
    Bluelight();
    Radar();
    Wipers();
    Manoevrierbehindert();
    
    display.display();
  } //IsChanged?

  //send data to PWM Modules
  for (uint16_t i = 0; i < 16; i++) {
     if (Module1[i].txInterval == 0) {
       if (SK33data.IsChanged()) {
        //Serial.print(Module1[0].value);
        pwmModule1.setPin(i, Module1[i].value, false);
       }
     } else {
       if ((millis() - Module1[i].lastTx) > Module1[i].txInterval) {
         pwmModule1.setPin(i, Module1[i].value, false);
         Module1[i].lastTx = millis();
       }
     }
        
    if (Module2[i].txInterval == 0) {
       if (SK33data.IsChanged()) {
        //Serial.print(Module1[0].value);
        pwmModule2.setPin(i, Module2[i].value, false);
       }
     } else {
       if ((millis() - Module2[i].lastTx) > Module2[i].txInterval) {
         pwmModule2.setPin(i, Module2[i].value, false);
         Module2[i].lastTx = millis();
       }
     }
     
     if (Module3[i].txInterval == 0) {
       if (SK33data.IsChanged()) {
        //Serial.print(Module1[0].value);
        pwmModule3.setPin(i, Module3[i].value, false);
       }
     } else {
       if ((millis() - Module3[i].lastTx) > Module3[i].txInterval) {
         pwmModule3.setPin(i, Module3[i].value, false);
         Module3[i].lastTx = millis();
       }
     }
  } //for 0-15 channels
  calculateBluelight();
  if (bWipersActive) calculateWipers();
  if (!bWipersActive) stopWipers();
  
  SK33data.Reset();
} //loop
