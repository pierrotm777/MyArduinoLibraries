/*
  DJI Naza (v1/V2 + PMU, Phantom) CAN data decoder library
  Outputs data to both serial port and SD card
  (c) Pawelsky 20150211
  Not for commercial use

  Requires FlexCan library v1.0 (or newer if compatible)
  https://github.com/teachop/FlexCAN_Library/releases/tag/v1.0

  Requires NazaCanDecoder library by Pawelsky
  http://www.rcgroups.com/forums/showpost.php?p=27129264&postcount=1

  Requires Teensy 3.1 board and CAN transceiver
  Complie with "CPU speed" set to "96MHz (overclock)"
  Refer to naza_can_decoder_wiring.jpg diagram for proper connection
  Connections can be greatly simplified using CAN bus and MicroSD or AllInOne shields by Pawelsky
  (see teensy_shields.jpg or teensy_aio_shield.jpg for installation and naza_can_decoder_wiring_shields.jpg or naza_can_decoder_wiring_aio_shield.jpg for wiring)
*/

#include "NazaCanDecoderLib.h"
#include "FlexCAN.h"
#include "SD.h"
#include "SPI.h"
#include "EEPROM.h"

// Use Serial to output data via USB or Serial1 to use Teensy's first serial port
#define SERIAL_PORT Serial

uint32_t currTime, attiTime, otherTime, clockTime;
char dateTime[20];
byte fileNumber; 
File logFile;
char logFileName[12];

void setup()
{
  SERIAL_PORT.begin(115200);
  NazaCanDecoder.begin();
  SD.begin(10);
  fileNumber = EEPROM.read(0);
  if(fileNumber >= 255) fileNumber = 0; else fileNumber++;
  EEPROM.write(0, fileNumber);
  sprintf(logFileName, "file%03u.log", fileNumber);
}

void loop()
{
  NazaCanDecoder.decode();

  currTime = millis();

  // Display attitude at 10Hz rate so every 100 milliseconds
  if(attiTime < currTime)
  {
    attiTime = currTime + 100;
    logFile = SD.open(logFileName, FILE_WRITE);
    if(logFile)
    {
      SERIAL_PORT.print("Pitch: "); SERIAL_PORT.print(NazaCanDecoder.getPitch());
      logFile.print("Pitch: "); logFile.print(NazaCanDecoder.getPitch());
      SERIAL_PORT.print(", Roll: "); SERIAL_PORT.println(NazaCanDecoder.getRoll());
      logFile.print(", Roll: "); logFile.println(NazaCanDecoder.getRoll());
      logFile.close();
    }
  }

  // Display other data at 5Hz rate so every 200 milliseconds
  if(otherTime < currTime)
  {
    otherTime = currTime + 200;
    logFile = SD.open(logFileName, FILE_WRITE);
    if(logFile)
    {
      SERIAL_PORT.print("Mode: "); 
      logFile.print("Mode: "); 
      switch (NazaCanDecoder.getMode())
      {
        case NazaCanDecoderLib::MANUAL:   SERIAL_PORT.print("MAN"); logFile.print("MAN"); break;
        case NazaCanDecoderLib::GPS:      SERIAL_PORT.print("GPS"); logFile.print("GPS"); break;
        case NazaCanDecoderLib::FAILSAFE: SERIAL_PORT.print("FS");  logFile.print("FS");  break;
        case NazaCanDecoderLib::ATTI:     SERIAL_PORT.print("ATT"); logFile.print("ATT"); break;
        default:                          SERIAL_PORT.print("UNK"); logFile.print("UNK");
      }
      SERIAL_PORT.print(", Bat: "); SERIAL_PORT.println(NazaCanDecoder.getBattery() / 1000.0, 2);
      logFile.print(", Bat: "); logFile.println(NazaCanDecoder.getBattery() / 1000.0, 2);

      SERIAL_PORT.print("Lat: "); SERIAL_PORT.print(NazaCanDecoder.getLat(), 7);
      logFile.print("Lat: "); logFile.print(NazaCanDecoder.getLat(), 7);
      SERIAL_PORT.print(", Lon: "); SERIAL_PORT.print(NazaCanDecoder.getLon(), 7);
      logFile.print(", Lon: "); logFile.print(NazaCanDecoder.getLon(), 7);
      SERIAL_PORT.print(", GPS alt: "); SERIAL_PORT.print(NazaCanDecoder.getGpsAlt());
      logFile.print(", GPS alt: "); logFile.print(NazaCanDecoder.getGpsAlt());
      SERIAL_PORT.print(", COG: "); SERIAL_PORT.print(NazaCanDecoder.getCog());
      logFile.print(", COG: "); logFile.print(NazaCanDecoder.getCog());
      SERIAL_PORT.print(", Speed: "); SERIAL_PORT.print(NazaCanDecoder.getSpeed());
      logFile.print(", Speed: "); logFile.print(NazaCanDecoder.getSpeed());
      SERIAL_PORT.print(", VSI: "); SERIAL_PORT.print(NazaCanDecoder.getVsi());
      logFile.print(", VSI: "); logFile.print(NazaCanDecoder.getVsi());
      SERIAL_PORT.print(", Fix: ");
      logFile.print(", Fix: ");
      switch (NazaCanDecoder.getFixType())
      {
        case NazaCanDecoderLib::NO_FIX:   SERIAL_PORT.print("No fix"); logFile.print("No fix"); break;
        case NazaCanDecoderLib::FIX_2D:   SERIAL_PORT.print("2D");     logFile.print("2D");     break;
        case NazaCanDecoderLib::FIX_3D:   SERIAL_PORT.print("3D");     logFile.print("3D");     break;
        case NazaCanDecoderLib::FIX_DGPS: SERIAL_PORT.print("3D");     logFile.print("DGPS");   break;
        default:                          SERIAL_PORT.print("UNK");    logFile.print("UNK");
      }
      SERIAL_PORT.print(", Sat: "); SERIAL_PORT.println(NazaCanDecoder.getNumSat());
      logFile.print(", Sat: "); logFile.println(NazaCanDecoder.getNumSat());

      SERIAL_PORT.print("Alt: "); SERIAL_PORT.print(NazaCanDecoder.getAlt());
      logFile.print("Alt: "); logFile.print(NazaCanDecoder.getAlt());
      SERIAL_PORT.print(", Heading: "); SERIAL_PORT.println(NazaCanDecoder.getHeading());
      logFile.print(", Heading: "); logFile.println(NazaCanDecoder.getHeading());
      logFile.close();
    }
  }

  // Display date/time at 1Hz rate so every 1000 milliseconds
  if(clockTime < currTime)
  {
    clockTime = currTime + 1000;
    logFile = SD.open(logFileName, FILE_WRITE);
    if(logFile)
    {
      sprintf(dateTime, "%4u.%02u.%02u %02u:%02u:%02u", 
              NazaCanDecoder.getYear() + 2000, NazaCanDecoder.getMonth(), NazaCanDecoder.getDay(),
              NazaCanDecoder.getHour(), NazaCanDecoder.getMinute(), NazaCanDecoder.getSecond());
      SERIAL_PORT.print("Date/Time: "); SERIAL_PORT.println(dateTime); 
      logFile.print("Date/Time: "); logFile.println(dateTime); 
      logFile.close();
    }
  }

  NazaCanDecoder.heartbeat();
}