#include "FrSky_FBUS_Telemetry.h"
#include <stdlib.h>
#include <string.h>

FrSky_FBUS_TelemetryClass::FrSky_FBUS_TelemetryClass(void)
: _enabled(false),
  _clearToSend(false),
  _phyId(TELEMETRY_PHY_COMMON),
  _nextSensor(0),
  _responseDelayUs(500),
  _lastRequestUs(0),
  _requests(0),
  _responses(0),
  _voltage_cV(0),
  _current_dA(0),
  _temperature1_C(0),
  _temperature2_C(0),
  _rpm(0),
  _gpsEnabled(false),
  _gps{false, 0, 0, 0, 0, 0, 0},
  _gpsLatLonToggle(0),
  _nmeaIndex(0),
  _ubxState(0),
  _ubxClass(0),
  _ubxId(0),
  _ubxLen(0),
  _ubxIndex(0),
  _ubxCkA(0),
  _ubxCkB(0)
{
}

void FrSky_FBUS_TelemetryClass::setEnabled(bool enabled)
{
  _enabled = enabled;
  if (!enabled) {
    _clearToSend = false;
  }
}

bool FrSky_FBUS_TelemetryClass::enabled() const { return _enabled; }

void FrSky_FBUS_TelemetryClass::setResponseDelayUs(uint16_t delayUs)
{
  _responseDelayUs = delayUs;
}

bool FrSky_FBUS_TelemetryClass::handleDownlink(const uint8_t *frame, uint8_t len, bool checksumRequired)
{
  if (len != (TELEMETRY_LEN + 1)) {
    return false;
  }

  if (checksumRequired && !frskyChecksumIsGood(frame, len)) {
    return false;
  }

  const uint8_t phyId = frame[0];
  const uint8_t frameId = frame[1];

  // Accept the common FBUS/FC telemetry poll. Other phy IDs are ignored for now.
  if (phyId != TELEMETRY_PHY_COMMON) {
    return true;
  }

  if (frameId == TELEMETRY_FRAME_NULL || frameId == TELEMETRY_FRAME_DATA || frameId == TELEMETRY_FRAME_READ) {
    _phyId = phyId;
    _requests++;
    _lastRequestUs = micros();
    _clearToSend = _enabled;
    return true;
  }

  return true;
}

bool FrSky_FBUS_TelemetryClass::process(Stream *serial)
{
  if (!_enabled || !_clearToSend || !serial) {
    return false;
  }

  if ((uint32_t)(micros() - _lastRequestUs) < _responseDelayUs) {
    return false;
  }

  const bool ok = writeNextSensor(serial);
  _clearToSend = false;
  return ok;
}

bool FrSky_FBUS_TelemetryClass::writeNextSensor(Stream *serial)
{
  // Round-robin telemetry. GPS fields are sent only when enabled and fixed.
  for (uint8_t tries = 0; tries < 11; tries++) {
    const uint8_t sensor = _nextSensor++;
    if (_nextSensor >= 10) {
      _nextSensor = 0;
    }

    switch (sensor) {
      case 0:
        return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_VFAS, _voltage_cV);

      case 1:
        return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_CURR, _current_dA);

      case 2:
        return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_TEMP1, (uint32_t)_temperature1_C);

      case 3:
        return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_TEMP2, (uint32_t)_temperature2_C);

      case 4:
        return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_RPM, _rpm);

      case 5:
        if (_gpsEnabled && _gps.fix) {
          const bool sendLon = (_gpsLatLonToggle++ & 0x01) != 0;
          return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_GPS_LATLONG, encodeGpsLatLon(sendLon));
        }
        break;

      case 6:
        if (_gpsEnabled && _gps.fix) {
          return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_GPS_ALT, (uint32_t)_gps.altitudeCm);
        }
        break;

      case 7:
        if (_gpsEnabled && _gps.fix) {
          // FrSky GPS speed expects knots * 1000. 1 cm/s = 0.0194384449 knots.
          const uint32_t knots1000 = (_gps.groundSpeedCms * 1944UL) / 100UL;
          return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_GPS_SPEED, knots1000);
        }
        break;

      case 8:
        if (_gpsEnabled && _gps.fix) {
          return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_GPS_COURSE, _gps.courseDeg100);
        }
        break;

      case 9:
        if (_gpsEnabled && _gps.fix) {
          return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_GPS_SATS, _gps.sats);
        }
        break;
    }
  }

  return writePayload(serial, _phyId, TELEMETRY_FRAME_DATA, SENSOR_ID_VFAS, _voltage_cV);
}

bool FrSky_FBUS_TelemetryClass::writePayload(Stream *serial, uint8_t phyId, uint8_t frameId, uint16_t valueId, uint32_t data)
{
  if (!serial) {
    return false;
  }

  const uint8_t data0 = (uint8_t)(data & 0xFF);
  const uint8_t data1 = (uint8_t)((data >> 8) & 0xFF);
  const uint8_t data2 = (uint8_t)((data >> 16) & 0xFF);
  const uint8_t data3 = (uint8_t)((data >> 24) & 0xFF);

  uint16_t checksum = 0;
  const uint8_t bytes[8] = {
    phyId,
    frameId,
    (uint8_t)(valueId & 0xFF),
    (uint8_t)(valueId >> 8),
    data0,
    data1,
    data2,
    data3
  };
  for (uint8_t i = 0; i < sizeof(bytes); i++) {
    checksum += bytes[i];
    checksum += checksum >> 8;
    checksum &= 0x00FF;
  }
  const uint8_t crc = (uint8_t)(0xFF - checksum);

  serial->write((uint8_t)TELEMETRY_LEN);
  serial->write(phyId);
  serial->write(frameId);
  serial->write((uint8_t)(valueId & 0xFF));
  serial->write((uint8_t)(valueId >> 8));
  serial->write(data0);
  serial->write(data1);
  serial->write(data2);
  serial->write(data3);
  serial->write(crc);

  _responses++;
  return true;
}

bool FrSky_FBUS_TelemetryClass::frskyChecksumIsGood(const uint8_t *data, uint8_t len)
{
  uint16_t checksum = 0;
  for (uint8_t i = 0; i < len; i++) {
    checksum += data[i];
    checksum += checksum >> 8;
    checksum &= 0x00FF;
  }
  return checksum == 0x00FF;
}

void FrSky_FBUS_TelemetryClass::setVoltage_V(float volts)
{
  if (volts < 0.0f) volts = 0.0f;
  _voltage_cV = (uint32_t)(volts * 100.0f + 0.5f);
}

void FrSky_FBUS_TelemetryClass::setCurrent_A(float amps)
{
  if (amps < 0.0f) amps = 0.0f;
  _current_dA = (uint32_t)(amps * 10.0f + 0.5f);
}

void FrSky_FBUS_TelemetryClass::setVoltageRaw_cV(uint32_t centiVolts) { _voltage_cV = centiVolts; }
void FrSky_FBUS_TelemetryClass::setCurrentRaw_dA(uint32_t deciAmps) { _current_dA = deciAmps; }

void FrSky_FBUS_TelemetryClass::setTemperature1_C(float celsius)
{
  _temperature1_C = (int32_t)(celsius + (celsius >= 0.0f ? 0.5f : -0.5f));
}

void FrSky_FBUS_TelemetryClass::setTemperature2_C(float celsius)
{
  _temperature2_C = (int32_t)(celsius + (celsius >= 0.0f ? 0.5f : -0.5f));
}

void FrSky_FBUS_TelemetryClass::setTemperature1Raw_C(int32_t celsius) { _temperature1_C = celsius; }
void FrSky_FBUS_TelemetryClass::setTemperature2Raw_C(int32_t celsius) { _temperature2_C = celsius; }
void FrSky_FBUS_TelemetryClass::setRpm(uint32_t rpm) { _rpm = rpm; }

void FrSky_FBUS_TelemetryClass::setGpsEnabled(bool enabled) { _gpsEnabled = enabled; }
bool FrSky_FBUS_TelemetryClass::gpsEnabled() const { return _gpsEnabled; }

void FrSky_FBUS_TelemetryClass::setGpsFix(bool fix, uint8_t sats)
{
  _gps.fix = fix;
  _gps.sats = sats;
}

void FrSky_FBUS_TelemetryClass::setGps(uint8_t sats, int32_t latE7, int32_t lonE7, int32_t altCm, uint32_t speedCms, uint16_t courseDeg100)
{
  _gps.fix = (sats > 0);
  _gps.sats = sats;
  _gps.latitudeE7 = latE7;
  _gps.longitudeE7 = lonE7;
  _gps.altitudeCm = altCm;
  _gps.groundSpeedCms = speedCms;
  _gps.courseDeg100 = courseDeg100;
}

const FrSkyFbusGpsData &FrSky_FBUS_TelemetryClass::gps() const { return _gps; }

uint32_t FrSky_FBUS_TelemetryClass::encodeGpsLatLon(bool longitude) const
{
  const int32_t valueE7 = longitude ? _gps.longitudeE7 : _gps.latitudeE7;
  uint32_t v = (uint32_t)labs(valueE7);

  // FrSky/OpenTX SmartPort GPS coordinate encoding, same principle as INAV:
  // degrees * 1e7 -> minutes * 10000, with bit31 = longitude and bit30 = negative.
  v = (v + v / 2UL) / 25UL;
  if (longitude) {
    v |= 0x80000000UL;
  }
  if (valueE7 < 0) {
    v |= 0x40000000UL;
  }
  return v;
}

bool FrSky_FBUS_TelemetryClass::gpsProcessNMEA(Stream &gpsSerial)
{
  bool updated = false;
  while (gpsSerial.available() > 0) {
    if (gpsProcessNmeaChar((char)gpsSerial.read())) {
      updated = true;
    }
  }
  return updated;
}

bool FrSky_FBUS_TelemetryClass::gpsProcessNmeaChar(char c)
{
  if (c == '\r') {
    return false;
  }

  if (c == '\n') {
    _nmeaLine[_nmeaIndex] = '\0';
    _nmeaIndex = 0;
    if (_nmeaLine[0] == '$') {
      return parseNmeaLine(_nmeaLine);
    }
    return false;
  }

  if (_nmeaIndex < (sizeof(_nmeaLine) - 1)) {
    _nmeaLine[_nmeaIndex++] = c;
  } else {
    _nmeaIndex = 0;
  }
  return false;
}

int32_t FrSky_FBUS_TelemetryClass::parseNmeaCoordE7(const char *field, const char *hemi)
{
  if (!field || !field[0] || !hemi || !hemi[0]) {
    return 0;
  }

  const double raw = atof(field);
  const int32_t degrees = (int32_t)(raw / 100.0);
  const double minutes = raw - ((double)degrees * 100.0);
  double deg = (double)degrees + minutes / 60.0;

  if (hemi[0] == 'S' || hemi[0] == 'W') {
    deg = -deg;
  }

  return (int32_t)(deg * 10000000.0 + (deg >= 0 ? 0.5 : -0.5));
}

bool FrSky_FBUS_TelemetryClass::parseNmeaLine(char *line)
{
  // Strip checksum marker if present. Lightweight example parser; checksum is not rejected.
  char *star = strchr(line, '*');
  if (star) {
    *star = '\0';
  }

  char *fields[20];
  uint8_t count = 0;
  char *p = strtok(line, ",");
  while (p && count < 20) {
    fields[count++] = p;
    p = strtok(NULL, ",");
  }
  if (count == 0) {
    return false;
  }

  const char *type = fields[0];
  const size_t n = strlen(type);
  if (n < 3) {
    return false;
  }
  type = type + n - 3;

  if (strcmp(type, "GGA") == 0 && count >= 10) {
    const uint8_t fixQuality = (uint8_t)atoi(fields[6]);
    const uint8_t sats = (uint8_t)atoi(fields[7]);
    _gps.fix = (fixQuality > 0);
    _gps.sats = sats;
    if (_gps.fix) {
      _gps.latitudeE7 = parseNmeaCoordE7(fields[2], fields[3]);
      _gps.longitudeE7 = parseNmeaCoordE7(fields[4], fields[5]);
      _gps.altitudeCm = (int32_t)(atof(fields[9]) * 100.0f);
    }
    return true;
  }

  if (strcmp(type, "RMC") == 0 && count >= 9) {
    _gps.fix = (fields[2][0] == 'A');
    if (_gps.fix) {
      _gps.latitudeE7 = parseNmeaCoordE7(fields[3], fields[4]);
      _gps.longitudeE7 = parseNmeaCoordE7(fields[5], fields[6]);
      _gps.groundSpeedCms = (uint32_t)(atof(fields[7]) * 51.4444f + 0.5f); // knots -> cm/s
      _gps.courseDeg100 = (uint16_t)(atof(fields[8]) * 100.0f + 0.5f);
    }
    return true;
  }

  return false;
}

bool FrSky_FBUS_TelemetryClass::gpsProcessUBlox(Stream &gpsSerial)
{
  bool updated = false;
  while (gpsSerial.available() > 0) {
    if (gpsProcessUbxByte((uint8_t)gpsSerial.read())) {
      updated = true;
    }
  }
  return updated;
}

bool FrSky_FBUS_TelemetryClass::gpsProcessUbxByte(uint8_t b)
{
  switch (_ubxState) {
    case 0:
      _ubxState = (b == 0xB5) ? 1 : 0;
      break;
    case 1:
      _ubxState = (b == 0x62) ? 2 : 0;
      break;
    case 2:
      _ubxClass = b; _ubxCkA = b; _ubxCkB = _ubxCkA; _ubxState = 3;
      break;
    case 3:
      _ubxId = b; _ubxCkA += b; _ubxCkB += _ubxCkA; _ubxState = 4;
      break;
    case 4:
      _ubxLen = b; _ubxCkA += b; _ubxCkB += _ubxCkA; _ubxState = 5;
      break;
    case 5:
      _ubxLen |= ((uint16_t)b << 8); _ubxCkA += b; _ubxCkB += _ubxCkA;
      if (_ubxLen > sizeof(_ubxPayload)) {
        _ubxState = 0;
      } else {
        _ubxIndex = 0;
        _ubxState = (_ubxLen == 0) ? 7 : 6;
      }
      break;
    case 6:
      _ubxPayload[_ubxIndex++] = b; _ubxCkA += b; _ubxCkB += _ubxCkA;
      if (_ubxIndex >= _ubxLen) {
        _ubxState = 7;
      }
      break;
    case 7:
      if (b == _ubxCkA) {
        _ubxState = 8;
      } else {
        _ubxState = 0;
      }
      break;
    case 8:
      _ubxState = 0;
      if (b == _ubxCkB && _ubxClass == 0x01 && _ubxId == 0x07) {
        return parseUbxNavPvt(_ubxPayload, _ubxLen);
      }
      break;
  }
  return false;
}

bool FrSky_FBUS_TelemetryClass::parseUbxNavPvt(const uint8_t *payload, uint16_t len)
{
  if (len < 92) {
    return false;
  }

  const uint8_t fixType = payload[20];
  const uint8_t flags = payload[21];
  const bool validFix = (fixType >= 3) && ((flags & 0x01) != 0);

  int32_t lon, lat, height, gSpeed, headMot;
  memcpy(&lon, &payload[24], sizeof(lon));          // deg * 1e7
  memcpy(&lat, &payload[28], sizeof(lat));          // deg * 1e7
  memcpy(&height, &payload[32], sizeof(height));    // mm above ellipsoid
  memcpy(&gSpeed, &payload[60], sizeof(gSpeed));    // mm/s
  memcpy(&headMot, &payload[64], sizeof(headMot));  // deg * 1e5

  _gps.fix = validFix;
  _gps.sats = payload[23];
  _gps.longitudeE7 = lon;
  _gps.latitudeE7 = lat;
  _gps.altitudeCm = height / 10;
  _gps.groundSpeedCms = (gSpeed > 0) ? (uint32_t)(gSpeed / 10) : 0;
  _gps.courseDeg100 = (uint16_t)(headMot / 1000);
  return true;
}

uint32_t FrSky_FBUS_TelemetryClass::requests() const { return _requests; }
uint32_t FrSky_FBUS_TelemetryClass::responses() const { return _responses; }

FrSky_FBUS_TelemetryClass FrSky_FBUS_Telemetry;
