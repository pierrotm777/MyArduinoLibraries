// I2C library
#include <Wire.h>

// PWM modules library
#include <Adafruit_PWMServoDriver.h>

// FrSky S.Port Sensor Library
#include <FrSkySportSensor.h>
#include <FrSkySportSensorAss.h>
#include <FrSkySportSingleWireSerial.h>
#include <FrSkySportTelemetryReader.h>
#include <SoftwareSerial.h>

#define BLUELIGHT_SWITCH_TIME 50

bool module1onOff[16];
bool module2onOff[16];

bool bluelightActive = false;
uint8_t bluelightNr = 1;
unsigned long prevBlueLightTime = 0;
unsigned long prevI2CpollingTime = 0;

const uint16_t mappingModule1[] PROGMEM =
{
   256, //heck
  1024, //heckschlepp
   128, //navSB
   256, //SBsigO
  1024, //SBsigM
   256, //SBsigU
   128, //
  1024, //blau1
  1024, //blau3
  1024, //blau5
   128, //
   128, //
   128, //
   128, //
   128, //
   128  //
};

const uint16_t mappingModule2[16] PROGMEM =
{
   256, //topp
   256, //toppschlepp
  1024, //navBB
   256, //BBsigO
  1024, //BBsigM
   256, //BBsigU
   128, //
  1024, //blau2
  1024, //blau4
  1024, //blau6
   128, //
   128, //
   128, //
   128, //
   128, //
   128  //
};

// Sensor definition to initialize telemetry
FrSkySportSensorAss ass(FrSkySportSensor::ID14); //create a sensor
FrSkySportTelemetryReader telemetry; //create a telemetry object

//create PWM objects for LED and LED/Servo
Adafruit_PWMServoDriver pwmLed1 = Adafruit_PWMServoDriver(0x40, Wire);
Adafruit_PWMServoDriver pwmLed2 = Adafruit_PWMServoDriver(0x41, Wire);
//Adafruit_PWMServoDriver pwmServo = Adafruit_PWMServoDriver(0x42, Wire);

uint8_t uiValue[4] = { 0, 0, 0, 0 };
bool bValuesUpdated = false;
int iValToggleSwitch = 0;

void setup() {
  // set I2C speed
  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("Telemetry in Read mode OK!");

  // configure telemetry serial port
  //telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_4, &ass);
  //telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_4, &ass);
  telemetry.begin(FrSkySportSingleWireSerial::SERIAL_1, &ass);
  
  // initialize PWM objects
  pwmLed1.begin();
  pwmLed1.setPWMFreq(200);
  
  pwmLed2.begin();
  pwmLed2.setPWMFreq(200);

  prevBlueLightTime = millis();
  prevI2CpollingTime = millis();
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
    
    for (int i = 0; i < 16; i++) { 
      module1onOff[i] = false; 
      module2onOff[i] = false; 
    }

    //check RXbyte1
    //fahrt
    if ((uiValue[0] & 0x01) != 0) {
      module1onOff[0] = true; //heck
      module1onOff[2] = true; //navSB
      module2onOff[0] = true; //topp
      module2onOff[2] = true; //navBB
    }

    //schleppfahrt
    if ((uiValue[0] & 0x02) != 0) {
      module1onOff[0] = true; //heck
      module1onOff[1] = true; //heckSchlepp
      module1onOff[2] = true; //navSB
      module2onOff[0] = true; //topp
      module2onOff[1] = true; //toppschlepp
      module2onOff[2] = true; //navBB
    }

    //blaulicht
    if ((uiValue[0] & 0x04) != 0) {
      bluelightActive = true;
    } else {
      bluelightActive = false;
      pwmLed1.setPin(7, 0, false);
      pwmLed2.setPin(7, 0, false);
      pwmLed1.setPin(8, 0, false);
      pwmLed2.setPin(8, 0, false);
      pwmLed1.setPin(9, 0, false);
      pwmLed2.setPin(9, 0, false);
    }

    if (uiValue[1] != 0) {
      //manövrierbehindert
      if ((uiValue[1] & 0x01) != 0) {
        module1onOff[3] = true; //SBsignalO
        module1onOff[4] = true; //SBsignalM
        module1onOff[5] = true; //SBsignalU
        module2onOff[3] = true; //BBsignalO
        module2onOff[4] = true; //BBsignalM
        module2onOff[5] = true; //BBsignalU-
      }
    }
    
    //write ledState to PWM modules
    for (int i = 0; i < 7; i++) {
      if (module1onOff[i]) {
        pwmLed1.setPin(i, (uint16_t)pgm_read_word(&mappingModule1[i]), false);
        Serial.println("pwmLed1 ON");
      } else {
        pwmLed1.setPin(i, 0, false);
        Serial.println("pwmLed1 OFF");
      }
  
      if (module2onOff[i]) {
        pwmLed2.setPin(i, (uint16_t)pgm_read_word(&mappingModule2[i]), false);
      } else {
        pwmLed2.setPin(i, 0, false);
      }
    }

    for (int i = 10; i < 16; i++) {
      if (module1onOff[i]) {
        pwmLed1.setPin(i, (uint16_t)pgm_read_word(&mappingModule1[i]), false);
      } else {
        pwmLed1.setPin(i, 0, false);
      }
  
      if (module2onOff[i]) {
        pwmLed2.setPin(i, (uint16_t)pgm_read_word(&mappingModule2[i]), false);
      } else {
        pwmLed2.setPin(i, 0, false);
      }
    }
  }

  //calculate BlueLight
  if (bluelightActive && ((millis() - prevBlueLightTime) > BLUELIGHT_SWITCH_TIME)) {
  
    bluelightNr++;
    
    if(bluelightNr > 6) { bluelightNr = 1; }
    
    switch (bluelightNr) {
      case 1:
        pwmLed2.setPin(9, 0, false);
        pwmLed1.setPin(7, (uint16_t)pgm_read_word(&mappingModule1[7]), false);
        break;
      case 2:
        pwmLed1.setPin(7, 0, false);
        pwmLed2.setPin(7, (uint16_t)pgm_read_word(&mappingModule1[7]), false);
        break;
      case 3:
        pwmLed2.setPin(7, 0, false);
        pwmLed1.setPin(8, (uint16_t)pgm_read_word(&mappingModule1[8]), false);
        break;
      case 4:
        pwmLed1.setPin(8, 0, false);
        pwmLed2.setPin(8, (uint16_t)pgm_read_word(&mappingModule1[8]), false);
        break;
      case 5:
        pwmLed2.setPin(8, 0, false);
        pwmLed1.setPin(9, (uint16_t)pgm_read_word(&mappingModule1[9]), false);
        break;
      case 6:
        pwmLed1.setPin(9, 0, false);
        pwmLed2.setPin(9, (uint16_t)pgm_read_word(&mappingModule1[9]), false);
        break;
      default:
        break;
    }

    prevBlueLightTime = millis();
  }
}
