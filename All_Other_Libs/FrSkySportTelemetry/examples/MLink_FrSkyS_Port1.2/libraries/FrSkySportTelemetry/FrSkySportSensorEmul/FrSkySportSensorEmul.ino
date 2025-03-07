/*
  FrSky S-Port Telemetry Decoder library example
  (c) Pawelsky 20160919
  Not for commercial use
  
  Note that you need Teensy 3.x/LC or 328P based (e.g. Pro Mini, Nano, Uno) board and FrSkySportDecoder library for this example to work
*/


#include "FrSkySportSensor.h"
#include "FrSkySportSensorFlvss.h"
#include "FrSkySportSensorFcs.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"

FrSkySportSensorFlvss flvss1;
FrSkySportSensorFlvss flvss2(FrSkySportSensor::ID15);
FrSkySportSensorFcs fcs;
FrSkySportTelemetry telemetry;

void setup()
{
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || defined(__MK66FX1M0__) || defined(__MK64FX512__) //Teensy
  telemetry.begin(FrSkySportSingleWireSerial::SERIAL_3, &flvss1, &flvss2, &fcs);
#else //ATMega 328
  telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_8, &flvss1, &flvss2, &fcs);
#endif

}

void loop()
{

  /* DO YOUR STUFF HERE */

  flvss1.setData(4.10, 4.11, 4.12, 4.13, 4.14, 4.15);
  flvss2.setData(3.10, 3.11, 3.12, 3.13, 3.14, 3.15);
  fcs.setData(25.3, 12.6);
  telemetry.send();
}
