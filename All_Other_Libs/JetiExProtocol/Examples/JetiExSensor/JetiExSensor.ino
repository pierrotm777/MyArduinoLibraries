/* 
  Jeti Sensor EX Telemetry C++ Library
  
  Main program
  --------------------------------------------------------------------
  
  Copyright (C) 2015 Bernd Wokoeck

  *** Extended notice on additional work and copyrights, see header of JetiExProtocol.cpp ***

  Wiring:

    Arduino Mini  TXD-Pin 0 <-- Receiver "Ext." input (orange cable)

  Ressources:
    Uses built in UART of Arduini Mini Pro 328 
  
  Version history:
  0.90   11/22/2015  created
  0.95   12/23/2015  new sample sensors for GPS and date/time
  0.96   02/21/2016  comPort number as optional parameter for Teensy in Start(...)
                     sensor device id as optional parameter (SetDeviceId(...))
  0.99   06/05/2016  max number of sensors increased to 32 (set MAX_SENSOR to a higher value in JetiExProtocol.h if you need more)
                     bug with TYPE_6b removed
                     DemoSensor delivers 18 demo values now
  1.00   01/29/2017  Some refactoring:
                     - Bugixes for Jetibox keys and morse alarms (Thanks to Ingmar !)
                     - Optimized half duplex control for AVR CPUs in JetiExHardwareSerialInt class (for improved Jetibox key handling)
                     - Reduced size of serial transmit buffer (128-->64 words) 
                     - Changed bitrates for serial communication for AVR CPUs (9600-->9800 bps)
                     - EX encryption removed, as a consequence: new manufacturer ID: 0xA409
                       *** Telemetry setup in transmitter must be reconfigured (telemetry reset) ***
                     - Delay at startup increased (to give receiver more init time)
                     - New HandleMenu() function in JetiexSensor.ini (including more alarm test)
                     - JETI_DEBUG and BLOCKING_MODE removed (cleanup)
  1.0.1  02/15/2017  Support for ATMega32u4 CPU in Leonardo/Pro Micro
                     GetKey routine optimized 
  1.02   03/28/2017  New sensor memory management. Sensor data can be located in PROGMEM

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

**************************************************************/

#include "JetiExProtocol.h"
#include "DemoSensor.h"

// #define JETIEX_DEBUG

JetiExProtocol jetiEx;
DemoSensor     demoSensor;

void HandleMenu();

enum
{
  ID_VOLTAGE = 1,
  ID_ALTITUDE,
  ID_TEMP,
  ID_CLIMB,
  ID_FUEL,
  ID_RPM,
  ID_GPSLON,
  ID_GPSLAT,
  ID_DATE,
  ID_TIME,
  ID_VAL11,
  ID_VAL12,
  ID_VAL13,
  ID_VAL14,
  ID_VAL15,
  ID_VAL16,
  ID_VAL17,
  ID_VAL18,
};

// max. 6 Sensors
// id from 1..15
// name plus unit must be < 20 characters
// precision = 0 --> 0, precision = 1 --> 0.0, precision = 2 --> 0.00
JETISENSOR_CONST sensors[] PROGMEM =
{
  // id             name          unit         data type             precision 
  { ID_VOLTAGE,    "Voltage",    "V",         JetiSensor::TYPE_14b, 1 },
  { ID_ALTITUDE,   "Altitude",   "m",         JetiSensor::TYPE_14b, 0 },
  { ID_TEMP,       "Temp",       "\xB0\x43",  JetiSensor::TYPE_14b, 0 }, // °C
  { ID_CLIMB,      "Climb",      "m/s",       JetiSensor::TYPE_14b, 2 },
  { ID_FUEL,       "Fuel",       "%",         JetiSensor::TYPE_14b, 0 },
  { ID_RPM,        "RPM x 1000", "/min",      JetiSensor::TYPE_14b, 1 },

  { ID_GPSLON,     "Longitude",  " ",         JetiSensor::TYPE_GPS, 0 },
  { ID_GPSLAT,     "Latitude",   " ",         JetiSensor::TYPE_GPS, 0 },
  { ID_DATE,       "Date",       " ",         JetiSensor::TYPE_DT,  0 },
  { ID_TIME,       "Time",       " ",         JetiSensor::TYPE_DT,  0 },

  { ID_VAL11,      "V11",        "U11",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL12,      "V12",        "U12",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL13,      "V13",        "U13",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL14,      "V14",        "U14",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL15,      "V15",        "U15",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL16,      "V16",        "U16",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL17,      "V17",        "U17",       JetiSensor::TYPE_14b, 0 },
  { ID_VAL18,      "V18",        "U18",       JetiSensor::TYPE_14b, 0 },
  { 0 } // end of array
};

void setup()
{
#ifdef JETIEX_DEBUG
  #if defined (CORE_TEENSY) || (__AVR_ATmega32U4__)
    Serial.begin( 9600 );
  #endif
#endif 

  jetiEx.SetDeviceId( 0x76, 0x32 ); // 0x3276
  jetiEx.Start( "ECU", sensors, JetiExProtocol::SERIAL2 );

  jetiEx.SetJetiboxText( JetiExProtocol::LINE1, "Start 1" );
  jetiEx.SetJetiboxText( JetiExProtocol::LINE2, "Start 2" );


  /* add your setup code here */
}

void loop()
{
  /* add your main program code here */

  jetiEx.SetSensorValue( ID_VOLTAGE,  demoSensor.GetVoltage() );
  jetiEx.SetSensorValue( ID_ALTITUDE, demoSensor.GetAltitude() );
  jetiEx.SetSensorValue( ID_TEMP,     demoSensor.GetTemp() ); 
  jetiEx.SetSensorValue( ID_CLIMB,    demoSensor.GetClimb() );
  jetiEx.SetSensorValue( ID_FUEL,     demoSensor.GetFuel() );
  jetiEx.SetSensorValue( ID_RPM,      demoSensor.GetRpm() );

  jetiEx.SetSensorValueGPS( ID_GPSLON, true,  11.55616f ); // E 11° 33' 22.176"
  jetiEx.SetSensorValueGPS( ID_GPSLAT, false, 48.24570f ); // N 48° 14' 44.520"
  jetiEx.SetSensorValueDate( ID_DATE,  29, 12, 2015 );
  jetiEx.SetSensorValueTime( ID_TIME,  19, 16, 37 );

  jetiEx.SetSensorValue( ID_VAL11,    demoSensor.GetVal(4) );
  jetiEx.SetSensorValue( ID_VAL12,    demoSensor.GetVal(5) );
  jetiEx.SetSensorValue( ID_VAL13,    demoSensor.GetVal(6) );
  jetiEx.SetSensorValue( ID_VAL14,    demoSensor.GetVal(7) );
  jetiEx.SetSensorValue( ID_VAL15,    demoSensor.GetVal(8) );
  jetiEx.SetSensorValue( ID_VAL16,    demoSensor.GetVal(9) );
  jetiEx.SetSensorValue( ID_VAL17,    demoSensor.GetVal(10) );
  jetiEx.SetSensorValue( ID_VAL18,    demoSensor.GetVal(11) );

  HandleMenu();

  jetiEx.DoJetiSend(); 
}

void HandleMenu()
{
  static char _buffer[ 17 ];
  static int _x = 0, _y = 0;

  uint8_t c = jetiEx.GetJetiboxKey();

  if( c == 0 )
    return;

#ifdef JETIEX_DEBUG
  #if defined (CORE_TEENSY) || (__AVR_ATmega32U4__)
    Serial.println( c );
  #endif
#endif 

  // down
  if( c == 0xb0 )
  {
    _y++;
  }

  // up
  if( c == 0xd0 )
  { 
    if( _y > 0 )
      _y--;
  }

  // right
  if( c == 0xe0 ) 
  {
    _x++;
    // jetiEx.SetJetiAlarm( 'U' );  // Alarm "U"
  }

  // left
  if( c == 0x70 ) 
  {
    if( _x > 0 )
      _x--;
    else
      jetiEx.SetJetiAlarm( 'U' );  // Alarm "U"
  }

  sprintf( _buffer, "Menu x/y: %d/%d", _x, _y );
  jetiEx.SetJetiboxText( JetiExProtocol::LINE1, _buffer );

  sprintf( _buffer, "Key: 0x%2.2x", c );
  jetiEx.SetJetiboxText( JetiExProtocol::LINE2, _buffer );
}

