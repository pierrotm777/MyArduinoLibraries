#pragma once

#include <Arduino.h>

struct FrSkyFbusGpsData {
  bool fix;
  uint8_t sats;
  int32_t latitudeE7;      // degrees * 1e7
  int32_t longitudeE7;     // degrees * 1e7
  int32_t altitudeCm;      // centimetres
  uint32_t groundSpeedCms; // cm/s
  uint16_t courseDeg100;   // degrees * 100
};

class FrSky_FBUS_TelemetryClass {
public:
  // F.Port2 / FBUS telemetry downlink/uplink frame constants.
  // Wire format: [0x08][phyId][frameId][valueId_L][valueId_H][data0..3][crc]
  static constexpr uint8_t TELEMETRY_LEN = 8; // data bytes after length, before CRC
  static constexpr uint8_t TELEMETRY_PHY_COMMON = 0x1B;
  static constexpr uint8_t TELEMETRY_FRAME_NULL = 0x00;
  static constexpr uint8_t TELEMETRY_FRAME_DATA = 0x10;
  static constexpr uint8_t TELEMETRY_FRAME_READ = 0x30;

  // Common FrSky SmartPort sensor IDs used inside FBUS telemetry payloads.
  static constexpr uint16_t SENSOR_ID_CURR = 0x0200;
  static constexpr uint16_t SENSOR_ID_VFAS = 0x0210;
  static constexpr uint16_t SENSOR_ID_TEMP1 = 0x0400;
  static constexpr uint16_t SENSOR_ID_TEMP2 = 0x0410;
  static constexpr uint16_t SENSOR_ID_RPM = 0x0500;
  static constexpr uint16_t SENSOR_ID_GPS_LATLONG = 0x0800;
  static constexpr uint16_t SENSOR_ID_GPS_ALT = 0x0820;
  static constexpr uint16_t SENSOR_ID_GPS_SPEED = 0x0830;
  static constexpr uint16_t SENSOR_ID_GPS_COURSE = 0x0840;
  static constexpr uint16_t SENSOR_ID_GPS_TIME_DATE = 0x0850;
  static constexpr uint16_t SENSOR_ID_GPS_SATS = 0x5103; // Non-standard/OpenTX/EdgeTX friendly custom sensor.

  FrSky_FBUS_TelemetryClass(void);

  void setEnabled(bool enabled);
  bool enabled() const;
  void setResponseDelayUs(uint16_t delayUs);

  void setVoltage_V(float volts);      // Sent as VFAS, 0.01 V units.
  void setCurrent_A(float amps);       // Sent as CURR, 0.1 A units.
  void setVoltageRaw_cV(uint32_t centiVolts);
  void setCurrentRaw_dA(uint32_t deciAmps);

  void setTemperature1_C(float celsius); // Sent as T1, 1 deg C units.
  void setTemperature2_C(float celsius); // Sent as T2, 1 deg C units.
  void setTemperature1Raw_C(int32_t celsius);
  void setTemperature2Raw_C(int32_t celsius);
  void setRpm(uint32_t rpm);             // Sent as RPM raw value.

  // GPS telemetry. Coordinates use the usual GPS convention: degrees * 1e7.
  void setGpsEnabled(bool enabled);
  bool gpsEnabled() const;
  void setGpsFix(bool fix, uint8_t sats = 0);
  void setGps(uint8_t sats, int32_t latE7, int32_t lonE7, int32_t altCm, uint32_t speedCms, uint16_t courseDeg100);
  const FrSkyFbusGpsData &gps() const;
  bool gpsProcessNMEA(Stream &gpsSerial);  // Parses basic NMEA GGA/RMC sentences from a GPS serial stream.
  bool gpsProcessUBlox(Stream &gpsSerial); // Parses UBX NAV-PVT frames from a u-blox GPS stream.
  bool gpsProcessNmeaChar(char c);
  bool gpsProcessUbxByte(uint8_t b);

  // Called by FrSky_FBUS when a telemetry downlink/poll frame is received.
  bool handleDownlink(const uint8_t *frame, uint8_t len, bool checksumRequired);

  // Called by FrSky_FBUS::read(). Can also be called manually in loop().
  bool process(Stream *serial);

  uint32_t requests() const;
  uint32_t responses() const;

private:
  bool _enabled;
  bool _clearToSend;
  uint8_t _phyId;
  uint8_t _nextSensor;
  uint16_t _responseDelayUs;
  uint32_t _lastRequestUs;
  uint32_t _requests;
  uint32_t _responses;

  uint32_t _voltage_cV;
  uint32_t _current_dA;
  int32_t _temperature1_C;
  int32_t _temperature2_C;
  uint32_t _rpm;

  bool _gpsEnabled;
  FrSkyFbusGpsData _gps;
  uint8_t _gpsLatLonToggle;

  char _nmeaLine[96];
  uint8_t _nmeaIndex;

  uint8_t _ubxState;
  uint8_t _ubxClass;
  uint8_t _ubxId;
  uint16_t _ubxLen;
  uint16_t _ubxIndex;
  uint8_t _ubxPayload[100];
  uint8_t _ubxCkA;
  uint8_t _ubxCkB;

  bool writeNextSensor(Stream *serial);
  bool writePayload(Stream *serial, uint8_t phyId, uint8_t frameId, uint16_t valueId, uint32_t data);
  uint32_t encodeGpsLatLon(bool longitude) const;
  static int32_t parseNmeaCoordE7(const char *field, const char *hemi);
  bool parseNmeaLine(char *line);
  bool parseUbxNavPvt(const uint8_t *payload, uint16_t len);
  static bool frskyChecksumIsGood(const uint8_t *data, uint8_t len);
};

extern FrSky_FBUS_TelemetryClass FrSky_FBUS_Telemetry;
