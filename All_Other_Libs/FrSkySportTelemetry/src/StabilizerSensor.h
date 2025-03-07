/*
  Based on FrSky FCS-40A/FCS-150A current sensor class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/

#ifndef _STABILIZER_SENSOR_H_
#define _STABILIZER_SENSOR_H_

#include "FrSkySportSensor.h"

//Note comments for LUA opentx source code...note required ranged of data id for Pop to work.
/*luadoc
@function sportTelemetryPop()
Pops a received SPORT packet from the queue. Please note that only packets using a data ID within 0x5000 to 0x52FF
(frame ID == 0x10), as well as packets with a frame ID equal 0x32 (regardless of the data ID) will be passed to
the LUA telemetry receive queue.
@retval nil queue does not contain any (or enough) bytes to form a whole packet
@retval multiple returns 4 values:
 * sensor ID (number)
 * frame ID (number)
 * data ID (number)
 * value (number)
@status current Introduced in 2.2.0
*/

#define STABILIZER_DEFAULT_ID ID3
#define STABILIZER_DATA_COUNT 1
#define STABILIZER_CONFIG_DATA_ID 0x5000
#define STABILZER_CONFIG_DATA_PERIOD 500

class StabilizerSensor : public FrSkySportSensor
{
public:
  //StabilizerSensor(SensorId id = FCS_DEFAULT_ID);
  StabilizerSensor(SensorId id = STABILIZER_DEFAULT_ID);
  void setData(float config);
  virtual uint16_t send(FrSkySportSingleWireSerial &serial, uint8_t id, uint32_t now);
  virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);
  float getConfig();

private:
  uint32_t configData;
  uint32_t configTime;
  float config;
};

#endif
