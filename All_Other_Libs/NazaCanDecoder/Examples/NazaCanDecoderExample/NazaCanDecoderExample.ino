/*
  DJI Naza (v1/V2 + PMU, Phantom) CAN data decoder library
  Outputs data to a serial port
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

// Use Serial to output data via USB or Serial1 to use Teensy's first serial port
#define SERIAL_PORT Serial

uint32_t currTime, attiTime, otherTime, clockTime;
char dateTime[20];
uint32_t messageId;

void setup()
{
  SERIAL_PORT.begin(115200);
  NazaCanDecoder.begin();
}

void loop()
{
  messageId = NazaCanDecoder.decode();
//  if(messageId) { SERIAL_PORT.print("Message "); SERIAL_PORT.print(messageId, HEX); SERIAL_PORT.println(" decoded"); }

  currTime = millis();

  // Display attitude at 10Hz rate so every 100 milliseconds
  if(attiTime < currTime)
  {
    attiTime = currTime + 100;
    SERIAL_PORT.print("Pitch: "); SERIAL_PORT.print(NazaCanDecoder.getPitch());
    SERIAL_PORT.print(", Roll: "); SERIAL_PORT.println(NazaCanDecoder.getRoll());
  }

  // Display other data at 5Hz rate so every 200 milliseconds
  if(otherTime < currTime)
  {
    otherTime = currTime + 200;
    SERIAL_PORT.print("Mode: "); 
    switch (NazaCanDecoder.getMode())
    {
      case NazaCanDecoderLib::MANUAL:   SERIAL_PORT.print("MAN"); break;
      case NazaCanDecoderLib::GPS:      SERIAL_PORT.print("GPS"); break;
      case NazaCanDecoderLib::FAILSAFE: SERIAL_PORT.print("FS");  break;
      case NazaCanDecoderLib::ATTI:     SERIAL_PORT.print("ATT"); break;
      default:                          SERIAL_PORT.print("UNK");
    }
    SERIAL_PORT.print(", Bat: "); SERIAL_PORT.println(NazaCanDecoder.getBattery() / 1000.0, 2);

    SERIAL_PORT.print("Lat: "); SERIAL_PORT.print(NazaCanDecoder.getLat(), 7);
    SERIAL_PORT.print(", Lon: "); SERIAL_PORT.print(NazaCanDecoder.getLon(), 7);
    SERIAL_PORT.print(", GPS alt: "); SERIAL_PORT.print(NazaCanDecoder.getGpsAlt());
    SERIAL_PORT.print(", COG: "); SERIAL_PORT.print(NazaCanDecoder.getCog());
    SERIAL_PORT.print(", Speed: "); SERIAL_PORT.print(NazaCanDecoder.getSpeed());
    SERIAL_PORT.print(", VSI: "); SERIAL_PORT.print(NazaCanDecoder.getVsi());
    SERIAL_PORT.print(", Fix: ");
    switch (NazaCanDecoder.getFixType())
    {
      case NazaCanDecoderLib::NO_FIX:   SERIAL_PORT.print("No fix"); break;
      case NazaCanDecoderLib::FIX_2D:   SERIAL_PORT.print("2D");     break;
      case NazaCanDecoderLib::FIX_3D:   SERIAL_PORT.print("3D");     break;
      case NazaCanDecoderLib::FIX_DGPS: SERIAL_PORT.print("DGPS");   break;
      default:                          SERIAL_PORT.print("UNK");
    }
    SERIAL_PORT.print(", Sat: "); SERIAL_PORT.println(NazaCanDecoder.getNumSat());

    SERIAL_PORT.print("Alt: "); SERIAL_PORT.print(NazaCanDecoder.getAlt());
    SERIAL_PORT.print(", Heading: "); SERIAL_PORT.println(NazaCanDecoder.getHeading());
  }

  // Display date/time at 1Hz rate so every 1000 milliseconds
  if(clockTime < currTime)
  {
    clockTime = currTime + 1000;
    sprintf(dateTime, "%4u.%02u.%02u %02u:%02u:%02u", 
            NazaCanDecoder.getYear() + 2000, NazaCanDecoder.getMonth(), NazaCanDecoder.getDay(),
            NazaCanDecoder.getHour(), NazaCanDecoder.getMinute(), NazaCanDecoder.getSecond());
    SERIAL_PORT.print("Date/Time: "); SERIAL_PORT.println(dateTime); 
  }

  NazaCanDecoder.heartbeat();
}