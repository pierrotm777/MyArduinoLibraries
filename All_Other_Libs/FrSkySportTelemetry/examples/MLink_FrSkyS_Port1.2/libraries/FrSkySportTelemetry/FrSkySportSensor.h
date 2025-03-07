/*
  FrSky sensor base class for Teensy 3.x/LC and 328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20160919
  Not for commercial use
*/
/*
#define RSSI_ID            0xf101
#define ADC1_ID            0xf102
#define ADC2_ID            0xf103
#define BATT_ID            0xf104
#define SWR_ID             0xf105
#define T1_FIRST_ID        0x0400
#define T1_LAST_ID         0x040f
#define T2_FIRST_ID        0x0410
#define T2_LAST_ID         0x041f
#define RPM_FIRST_ID       0x0500
#define RPM_LAST_ID        0x050f
#define FUEL_FIRST_ID      0x0600
#define FUEL_LAST_ID       0x060f
#define ALT_FIRST_ID       0x0100
#define ALT_LAST_ID        0x010f
#define VARIO_FIRST_ID     0x0110
#define VARIO_LAST_ID      0x011f
#define ACCX_FIRST_ID      0x0700
#define ACCX_LAST_ID       0x070f
#define ACCY_FIRST_ID      0x0710
#define ACCY_LAST_ID       0x071f
#define ACCZ_FIRST_ID      0x0720
#define ACCZ_LAST_ID       0x072f
#define CURR_FIRST_ID      0x0200
#define CURR_LAST_ID       0x020f
#define VFAS_FIRST_ID      0x0210
#define VFAS_LAST_ID       0x021f
#define GPS_SPEED_FIRST_ID 0x0830
#define GPS_SPEED_LAST_ID  0x083f
#define CELLS_FIRST_ID     0x0300
#define CELLS_LAST_ID      0x030f
//*/

#ifndef _FRSKY_SPORT_SENSOR_H_
#define _FRSKY_SPORT_SENSOR_H_

#include "Arduino.h"
#include "FrSkySportSingleWireSerial.h"

#define SENSOR_NO_DATA_ID 0x0000

class FrSkySportSensor
{
  public:
    // IDs of all sensors (including the CRC)
/*
    enum SensorId { ID1 = 0x00,  ID2 = 0xA1,  ID3 = 0x22,  ID4 = 0x83,  ID5 = 0xE4,  ID6 = 0x45,  ID7 = 0xC6,
                    ID8 = 0x67,  ID9 = 0x48,  ID10 = 0xE9, ID11 = 0x6A, ID12 = 0xCB, ID13 = 0xAC, ID14 = 0x0D,
                    ID15 = 0x8E, ID16 = 0x2F, ID17 = 0xD0, ID18 = 0x71, ID19 = 0xF2, ID20 = 0x53, ID21 = 0x34,
                    ID22 = 0x95, ID23 = 0x16, ID24 = 0xB7, ID25 = 0x98, ID26 = 0x39, ID27 = 0xBA, ID28 = 0x1b, ID_IGNORE = 0XFF };
*/

    // IDs of used sensors (for optimized timing)

    enum SensorId { ID1 = 0xFF,  ID2 = 0xFF,  ID3 = 0xFF,  ID4 = 0x83,  ID5 = 0xFF,  ID6 = 0xFF,  ID7 = 0xFF,
                    ID8 = 0xFF,  ID9 = 0xFF,  ID10 = 0xFF, ID11 = 0xFF, ID12 = 0xFF, ID13 = 0xAC, ID14 = 0x0D,
                    ID15 = 0xFF, ID16 = 0xFF, ID17 = 0xFF, ID18 = 0xFF, ID19 = 0xFF, ID20 = 0xFF, ID21 = 0xFF,
                    ID22 = 0xFF, ID23 = 0xFF, ID24 = 0xFF, ID25 = 0x98, ID26 = 0xFF, ID27 = 0xFF, ID28 = 0xFF, ID_IGNORE = 0XFF };


    // virtual function for sending a byte of data
    virtual void send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now);
    // virtual function for reading sensor data 
    virtual uint16_t decodeData(uint8_t id, uint16_t appId, uint32_t data);

  protected:
    FrSkySportSensor(SensorId id);
    SensorId sensorId;
    uint8_t sensorDataIdx;

  private:
    FrSkySportSensor();
};

#endif // _FRSKY_SPORT_SENSOR_H_
