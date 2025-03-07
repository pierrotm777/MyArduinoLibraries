#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

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
#define RX_WEB_TOGGLE_SWITCH 13

bool module1onOff[16];
bool module2onOff[16];

bool bluelightActive = false;
uint8_t bluelightNr = 1;
unsigned long prevBlueLightTime = 0;

const uint16_t mappingModule1[] =
{
   256, //heck
  1024, //heckschlepp
   128, //navSB
   256, //SBsigO
  1024, //SBsigM
   256, //SBsigU
   128, //
   128, //
   128, //
   128, //
   128, //
   128, //
   128,  //
  1024, //blau1
  1024, //blau3
  1024 //blau5
};

const uint16_t mappingModule2[16] =
{
   256, //topp
   256, //toppschlepp
  1024, //navBB
   256, //BBsigO
  1024, //BBsigM
   256, //BBsigU
   128, //
   128, //
   128, //
   128, //
   128, //
   128, //
   128, //
  1024, //blau2
  1024, //blau4
  1024  //blau6
};

// Sensor definition to initialize telemetry
FrSkySportSensorAss ass(FrSkySportSensor::ID14); //create a sensor
FrSkySportTelemetryReader telemetry; //create a telemetry object

Adafruit_SSD1306 display;

//create PWM objects for LED and LED/Servo
Adafruit_PWMServoDriver pwmLed1 = Adafruit_PWMServoDriver(&Wire, 0x40);
Adafruit_PWMServoDriver pwmLed2 = Adafruit_PWMServoDriver(&Wire, 0x41);
//Adafruit_PWMServoDriver pwmServo = Adafruit_PWMServoDriver(&Wire, 0x42);

/* This are the WiFi access point settings. Update them to your likin */
const char *ssid = "SK33";
const char *password = "theodorstorm";

// Define a web server at port 80 for HTTP
ESP8266WebServer server(80);

uint8_t uiValue[4] = { 0, 0, 0, 0 };
bool bValuesUpdated = false;
int iValToggleSwitch = 0;

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
  pwmLed1.begin();
  pwmLed1.setPWMFreq(200);
  
  pwmLed2.begin();
  pwmLed2.setPWMFreq(200);
  
  //pwmServo.begin();
  //pwmServo.setPWMFreq(100);

  //initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display(); 

  prevBlueLightTime = millis();

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  //set-up the custom IP address
  WiFi.mode(WIFI_AP_STA);
  //WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   // subnet FF FF FF 00  
  
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
 
  server.on ( "/", handleRoot );
  server.on ( "/FON", handleFahrtOn );
  server.on ( "/FOFF", handleFahrtOff );
  server.on ( "/SON", handleSchleppOn );
  server.on ( "/SOFF", handleSchleppOff );
  server.on ( "/BON", handleBluelightOn );
  server.on ( "/BOFF", handleBluelightOff );
  server.on ( "/MON", handleManeuverOn );
  server.on ( "/MOFF", handleManeuverOff );

  server.onNotFound ( handleNotFound );
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
    
  iValToggleSwitch = digitalRead(RX_WEB_TOGGLE_SWITCH);
  
  if(iValToggleSwitch == HIGH) {
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

      display.setCursor(0, 0);
      display.print("RX");
    }
  } else {
    server.handleClient();

    display.setCursor(0, 0);
    display.print("IC");
  }  
  
  // update display and output
  if (bValuesUpdated) {    
    bValuesUpdated = false;
    
    for (int i = 0; i < 16; i++) { 
      module1onOff[i] = false; 
      module2onOff[i] = false; 
    }

    //check RXbyte1
    if (uiValue[0] != 0) {
      Fahrt();
      Schleppfahrt();
      Blaulicht();
    }

    //check RXbyte2
    if (uiValue[1] != 0) {
       Manoevrierbehindert();
    }
    
    //write ledState to PWM modules
    for (int i = 0; i < 13; i++) {
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
   
    display.display();
  }

  //calculate BlueLight
  if (bluelightActive && ((millis() - prevBlueLightTime) > BLUELIGHT_SWITCH_TIME)) {
  
    bluelightNr++;
    
    if(bluelightNr > 6) { bluelightNr = 1; }
    
    switch (bluelightNr) {
      case 1:
        pwmLed2.setPin(15, 0, false);
        pwmLed1.setPin(13, mappingModule1[13], false);
        break;
      case 2:
        pwmLed1.setPin(13, 0, false);
        pwmLed2.setPin(13, mappingModule1[13], false);
        break;
      case 3:
        pwmLed2.setPin(13, 0, false);
        pwmLed1.setPin(14, mappingModule1[14], false);
        break;
      case 4:
        pwmLed1.setPin(14, 0, false);
        pwmLed2.setPin(14, mappingModule1[14], false);
        break;
      case 5:
        pwmLed2.setPin(14, 0, false);
        pwmLed1.setPin(15, mappingModule1[15], false);
        break;
      case 6:
        pwmLed1.setPin(15, 0, false);
        pwmLed2.setPin(15, mappingModule1[15], false);
        break;
      default:
        break;
    }

    prevBlueLightTime = millis();
  }
}
