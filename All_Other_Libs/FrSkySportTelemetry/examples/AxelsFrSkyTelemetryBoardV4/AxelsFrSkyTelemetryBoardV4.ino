/*  An Arduino NANO sends FrSky telemetry data. There are multiple sensors connected
    to the Arduino, including: A GPS, one BMP280 pressure sensor of altitude measuring,
    two DS18S20 temperature sensors and voltage divider for measuring up LiPo cell voltage.

    V4.0
    Axel Brinkeby
    2022-01-07
*/

#include <AltSoftSerial.h>             // https://github.com/PaulStoffregen/AltSoftSerial
#include <TinyGPS++.h>                // https://github.com/mikalhart/TinyGPSPlus
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <RunningAverage.h>         // https://www.arduino.cc/reference/en/libraries/runningaverage/

#include "FrSkySportSensor.h"
#include "FrSkySportSensorFcs.h"
#include "FrskySportSensorRpm.h"
#include "FrSkySportSensorGps.h"
#include "FrSkySportSensorVario.h"

#include "FrSkySportSingleWireSerial.h"     // maybe not needed?

#include "FrSkySportTelemetry.h"    // More about this here:  https://www.rcgroups.com/forums/showthread.php?2245978-FrSky-S-Port-telemetry-library-easy-to-use-and-configurable


const int SMART_PORT_PIN = 2;           // connected to the telemetry port on the receiver
const int BATTERY_SENSE_PIN = A0;
const int TEMP_READ_PIN = 6;            // Temperature sensor connected here
const int STATUS_LED_RED_PIN = 3;
const int STATUS_LED_YELLOW_PIN = 4;
const int STATUS_LED_BLUE_PIN = 5;

const bool USE_BMP280_SENSOR = true;     // set to false if you are using the official altitude sensor instead

const float NUMBER_OF_VOLTAGE_SAMPLES = 10;   // for each measurement, the voltage is sampled this many times, then the average is used

const float VOLTAGE_CALIBRATION_VALUE = 0.0147626f;   // the ADC result is multiplyed with this value to get voltage

OneWire oneWire(TEMP_READ_PIN);
DallasTemperature tempSensors(&oneWire);
Adafruit_BMP280 bmp;                    // BMP280 conected using I2C
RunningAverage AltAverage(50);
RunningAverage VspeedAverage(10);

TinyGPSPlus gps;

FrSkySportSensorFcs fcsSensor;        // Battery voltage and current (not used here)
FrSkySportSensorRpm rpmSensor;        // motor RPM (not used) and two temperature sensors
FrSkySportSensorVario varioSensor;    // Altitude and vertical speed
FrSkySportSensorGps gpsSensor;        // GPS: lat, lon, altitude(m), speed(m/s), course, date(year - 2000, month, day), time(hour, minute, second)
FrSkySportTelemetry telemetry;

int counter = 0;  // used in main loop

float temperature1 = 0;   // temperature readings
float temperature2 = 0;
      
float voltage = 0;      // main battery voltage

unsigned long lastSensorSampleTime = 0;
unsigned long lastPressureSampleTime = 0;
float pressureAtStartAltitude = 1013;     //  Standard pressure at sea level in millibar

float previusAlt = 0;
float vSpeed = 0; 
float batteryVoltage = 0; 

bool GPSfixValid = false;
bool PressureSensorValid = false;
bool batteryValid = false; 
bool temp1valid = false; 
bool temp2valid = false; 


void setup() {
  delay(10);

  pinMode(STATUS_LED_RED_PIN, OUTPUT);
  pinMode(STATUS_LED_YELLOW_PIN, OUTPUT);
  pinMode(STATUS_LED_BLUE_PIN, OUTPUT);
  digitalWrite(STATUS_LED_RED_PIN, HIGH);       // Red LED on during init, in case of error
  digitalWrite(STATUS_LED_YELLOW_PIN, LOW);
  digitalWrite(STATUS_LED_BLUE_PIN, LOW);

  Serial.begin(9600);                 // used for GPS

  // init DS18S20 temperature sensors
  tempSensors.begin();                      // init the temperature sensor,
  tempSensors.setWaitForConversion(false);  // and make the library operate in asynchronous mode
  tempSensors.requestTemperatures();
  
  // BMP280 pressure sensor using the Adafruit library
  if (USE_BMP280_SENSOR) {
    if (bmp.begin(0x76)) {    // 0x76 or 0x77
      // Default settings from datasheet. 
      bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     //Operating Mode.
                      Adafruit_BMP280::SAMPLING_X2,     // Temp. oversampling 
                      Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling 
                      Adafruit_BMP280::FILTER_X16,      // Filtering.
                      Adafruit_BMP280::STANDBY_MS_1);   // Standby time. 
    
      pressureAtStartAltitude = bmp.readPressure() / 100.0f;    // the start altitude is 0 meters
    } else {
      while (1);    // infinite loop with red error LED 
    }
  } 
      
  // If you use internal voltage ref you need to change the volage divider resistors so that the 
  // voltage never gets higher than 1.1 volts for a fully charged battery. 
  analogReference(INTERNAL);      // use internal 1.1V ref
   
  // Another alternative is to use the Vref-pin and bridge it to the 3.3V pin beside it. In that case
  // the volage divider resistors should be chosen so that the voltage never gets higher 
  // than 3.3 volts for a fully charged battery.
  //analogReference(EXTERNAL);        // use the voltage at the Vref-pin as the reference

  // If you don't call the analogReference() -function, %V is used as the reference. I don't recommend
  // this since it is not very stable or accurate. 
  
  if (USE_BMP280_SENSOR) {
    telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_2, &fcsSensor, &rpmSensor, &gpsSensor, &varioSensor); 
  } else {
    telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_2, &fcsSensor, &rpmSensor, &gpsSensor);  
  }

  digitalWrite(STATUS_LED_RED_PIN, LOW);
  Serial.println("Program started!");
}

void loop() {

  bool newData = false;    
  while (Serial.available()) {    // Feed the GPS parser with new data
    byte data = Serial.read();
    if (gps.encode(data)) 
      newData = true;
  }
  
  if (newData)    // New data from the GPS is avalible
  {    
    GPSfixValid = (gps.satellites.value() > 4) & (gps.hdop.hdop() < 2.0);

    if (gps.location.isUpdated()) {
      gpsSensor.setData( gps.location.lat(), gps.location.lng(),   // Latitude and longitude in degrees decimal (positive for N/E, negative for S/W)
                         gps.altitude.meters(),   // Altitude in m (can be negative)
                         gps.speed.mps(),  // Speed in m/s
                         gps.course.deg(),      // Course over ground in degrees (0-359, 0 = north)
                         gps.date.year() - 2000, gps.date.month(), gps.date.day(), // Date (year - 2000, month, day)
                         gps.time.hour(), gps.time.minute(), gps.time.second());    // Time (hour, minute, second)
      
    }
  }    

  if (USE_BMP280_SENSOR && millis() > lastPressureSampleTime + 100) {    // the pressure is sampled 10 times per second
    lastPressureSampleTime = millis();

    AltAverage.addValue(bmp.readAltitude(pressureAtStartAltitude));
    VspeedAverage.addValue((AltAverage.getAverage() - previusAlt) * 10.0f);  // calculate vertical speed
      
    varioSensor.setData(AltAverage.getAverage(), VspeedAverage.getAverage());  // Altitude in meters, Vertical speed in m/s

    previusAlt = AltAverage.getAverage();
    
    if (previusAlt != 0 && previusAlt > -50 && previusAlt < 1000)
      PressureSensorValid = true;
    else
      PressureSensorValid = false;
  }

  if (millis() > lastSensorSampleTime + 1000) {   // voltages and temperature is sampled once per second
    lastSensorSampleTime = millis();

    measureBattery();
    fcsSensor.setData(0.0f, voltage);

    measureTemperatures();
    rpmSensor.setData(gps.satellites.value(), temperature1, temperature2);    // Set FrSky values: rpm, temp1, temp2
  }

  digitalWrite(STATUS_LED_YELLOW_PIN, HIGH);

  if (gps.satellites.value() > 4)
    digitalWrite(STATUS_LED_BLUE_PIN, HIGH); 
      
  telemetry.send();  
  digitalWrite(STATUS_LED_YELLOW_PIN, LOW);  
  digitalWrite(STATUS_LED_BLUE_PIN, LOW);
}

void measureTemperatures() {
  temperature1 = tempSensors.getTempCByIndex(0);
  temp1valid = (temperature1 != -127);  
  temperature2 = tempSensors.getTempCByIndex(1);
  temp2valid = (temperature2 != -127); 

  if (!temp1valid) 
    temperature1 = 0; 
  if (!temp2valid) 
    temperature2 = 0; 
     
  tempSensors.requestTemperatures();  // request new values for the next time
}

void measureBattery() {  
  float sum = 0;
  for (int i = 0; i < NUMBER_OF_VOLTAGE_SAMPLES; i++)
  {
    sum += analogRead(BATTERY_SENSE_PIN) * VOLTAGE_CALIBRATION_VALUE;
    delay(1);
  }  
  voltage = sum / NUMBER_OF_VOLTAGE_SAMPLES;
  
  // is voltage pressent?
  if (voltage > 5.0)      
    batteryValid = true; 
  else {
    batteryValid = false;     
  }
  
}
