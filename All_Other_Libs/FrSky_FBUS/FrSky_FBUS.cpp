#include "FrSky_FBUS.h"
#include <string.h>

FrSky_FBUSClass::FrSky_FBUSClass(void)
: _serial(nullptr),
  _hwSerial(nullptr),
  _channelCount(DEFAULT_CHANNELS),
  _expectedChannels(DEFAULT_CHANNELS),
  _failsafe(false),
  _frameLost(false),
  _rssi(0),
  _synchro(0x00),
  _checksumRequired(true),
  _interByteTimeoutUs(120),
  _lastByteUs(0),
  _state(WAIT_LEN),
  _lengthByte(0),
  _typeByte(0),
  _payloadIndex(0),
  _payloadToRead(0),
  _lastRawLen(0),
  _lastStatus(NO_FRAME),
  _goodFrames(0),
  _crcErrors(0),
  _lengthErrors(0),
  _typeErrors(0)
{
  for (uint8_t i = 0; i < MAX_FBUS_CHANNELS; i++) {
    _channels[i] = 0;
  }
}

void FrSky_FBUSClass::serialAttach(Stream *RxStream)
{
  _serial = RxStream;
}

void FrSky_FBUSClass::begin(HardwareSerial &serial, uint32_t baud)
{
  _hwSerial = &serial;
  _serial = &serial;

  // FBUS is non-inverted. Do NOT use SBUS inversion here.
  _hwSerial->begin(baud, SERIAL_8N1);
  resetParser(NO_FRAME);
}

#if defined(ARDUINO_ARCH_ESP32)
void FrSky_FBUSClass::begin(HardwareSerial &serial, int8_t rxPin, int8_t txPin, uint32_t baud, uint32_t rxBufferSize)
{
  _hwSerial = &serial;
  _serial = &serial;

  // ESP32: pins are remappable, so the user must choose them explicitly.
  // FBUS is non-inverted. Do NOT use SBUS inversion here.
  if (rxBufferSize > 0) {
    _hwSerial->setRxBufferSize(rxBufferSize);
  }
  _hwSerial->begin(baud, SERIAL_8N1, rxPin, txPin);
  resetParser(NO_FRAME);
}
#endif

void FrSky_FBUSClass::begin(uint32_t baud)
{
  // Only configure the UART if begin(HardwareSerial&, ...) was used before.
  // If the user only called serialAttach(Stream*), the stream is assumed already configured.
  if (_hwSerial) {
    _hwSerial->begin(baud, SERIAL_8N1);
  }
  resetParser(NO_FRAME);
}

void FrSky_FBUSClass::end()
{
  if (_hwSerial) {
    _hwSerial->end();
  }
}


FrSky_FBUSClass::CpuType FrSky_FBUSClass::cpuType()
{
#if defined(ARDUINO_ARCH_ESP32)
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    return CPU_ESP32_S3;
  #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    return CPU_ESP32_S2;
  #elif defined(CONFIG_IDF_TARGET_ESP32C3)
    return CPU_ESP32_C3;
  #elif defined(CONFIG_IDF_TARGET_ESP32C6)
    return CPU_ESP32_C6;
  #elif defined(CONFIG_IDF_TARGET_ESP32H2)
    return CPU_ESP32_H2;
  #else
    return CPU_ESP32;
  #endif
#elif defined(__IMXRT1062__)
  return CPU_TEENSY_40;
#elif defined(__MKL26Z64__)
  return CPU_TEENSY_LC;
#elif defined(ARDUINO_ARCH_RP2040)
  return CPU_RP2040;
#elif defined(ARDUINO_ARCH_AVR) || defined(__AVR__)
  return CPU_AVR;
#else
  return CPU_UNKNOWN;
#endif
}

const char *FrSky_FBUSClass::cpuName()
{
  switch (cpuType()) {
    case CPU_AVR:       return "AVR";
    case CPU_TEENSY_40: return "Teensy 4.x / IMXRT1062";
    case CPU_TEENSY_LC: return "Teensy LC / MKL26Z64";
    case CPU_RP2040:    return "RP2040";
    case CPU_ESP32:     return "ESP32";
    case CPU_ESP32_S2:  return "ESP32-S2";
    case CPU_ESP32_S3:  return "ESP32-S3";
    case CPU_ESP32_C3:  return "ESP32-C3";
    case CPU_ESP32_C6:  return "ESP32-C6";
    case CPU_ESP32_H2:  return "ESP32-H2";
    default:            return "Unknown";
  }
}

bool FrSky_FBUSClass::isEsp32Cpu()
{
#if defined(ARDUINO_ARCH_ESP32)
  return true;
#else
  return false;
#endif
}

void FrSky_FBUSClass::setChecksumRequired(bool enabled)
{
  _checksumRequired = enabled;
}

void FrSky_FBUSClass::setInterByteTimeoutUs(uint32_t timeoutUs)
{
  _interByteTimeoutUs = timeoutUs;
}

void FrSky_FBUSClass::setExpectedChannels(uint8_t count)
{
  if (count == 16 || count == 24) {
    _expectedChannels = count;
  }
}

bool FrSky_FBUSClass::read()
{
  bool gotFrame = false;

  if (!_serial) {
    return false;
  }

  while (_serial->available() > 0) {
    const uint8_t b = (uint8_t)_serial->read();
    const uint32_t now = micros();

    if (_state != WAIT_LEN && _lastByteUs != 0 && (uint32_t)(now - _lastByteUs) > _interByteTimeoutUs) {
      resetParser(TIMEOUT);
    }
    _lastByteUs = now;

    switch (_state) {
      case WAIT_LEN:
        if (b == CONTROL_LEN_16CH || b == CONTROL_LEN_24CH) {
          _lengthByte = b;
          _payloadIndex = 0;
          _payloadToRead = b + 1; // payload bytes + CRC byte after the type byte
          _lastRawLen = 0;
          _lastRaw[_lastRawLen++] = b;
          _state = WAIT_TYPE;
        } else if (b == FrSky_FBUS_TelemetryClass::TELEMETRY_LEN) {
          // Telemetry downlink frame, no control type byte after length.
          // Format after length: phyId + SmartPort payload(7) + CRC = 9 bytes.
          _lengthByte = b;
          _typeByte = 0;
          _payloadIndex = 0;
          _payloadToRead = FrSky_FBUS_TelemetryClass::TELEMETRY_LEN + 1; // data bytes + CRC
          _lastRawLen = 0;
          _lastRaw[_lastRawLen++] = b;
          _state = WAIT_DATA;
        } else {
          _lastStatus = BAD_LENGTH;
          _lengthErrors++;
        }
        break;

      case WAIT_TYPE:
        _typeByte = b;
        _lastRaw[_lastRawLen++] = b;
        if (_typeByte != FRAME_TYPE_RC) {
          _typeErrors++;
          resetParser(BAD_TYPE);
        } else {
          _state = WAIT_DATA;
        }
        break;

      case WAIT_DATA:
        if (_payloadIndex >= sizeof(_payloadAndCrc)) {
          resetParser(OVERSIZE);
          break;
        }
        _payloadAndCrc[_payloadIndex++] = b;
        if (_lastRawLen < sizeof(_lastRaw)) {
          _lastRaw[_lastRawLen++] = b;
        }

        if (_payloadIndex >= _payloadToRead) {
          if (acceptFrame()) {
            gotFrame = true;
          }
          resetParser(gotFrame ? OK : _lastStatus);
        }
        break;
    }
  }

  processTelemetry();
  return gotFrame;
}

void FrSky_FBUSClass::resetParser(Status status)
{
  _state = WAIT_LEN;
  _lengthByte = 0;
  _typeByte = 0;
  _payloadIndex = 0;
  _payloadToRead = 0;
  _lastByteUs = 0;
  _lastStatus = status;
}

bool FrSky_FBUSClass::acceptFrame()
{
  if (_lengthByte == FrSky_FBUS_TelemetryClass::TELEMETRY_LEN) {
    if (!FrSky_FBUS_Telemetry.handleDownlink(_payloadAndCrc, _payloadIndex, _checksumRequired)) {
      _crcErrors++;
      _lastStatus = BAD_CRC;
      return false;
    }
    _lastStatus = OK;
    return false; // This was a telemetry request, not a new RC frame.
  }

  // Checksum is calculated over type + payload + crc, exactly like INAV checks
  // its internal buffer excluding the local FT_CONTROL marker.
  uint8_t crcBuf[1 + sizeof(_payloadAndCrc)];
  crcBuf[0] = _typeByte;
  memcpy(&crcBuf[1], _payloadAndCrc, _payloadIndex);

  if (_checksumRequired && !frskyChecksumIsGood(crcBuf, (uint8_t)(1 + _payloadIndex))) {
    _crcErrors++;
    _lastStatus = BAD_CRC;
    return false;
  }

  const uint8_t payloadLen = _lengthByte;
  if (!decodeRcFrame(_payloadAndCrc, payloadLen)) {
    _lastStatus = BAD_LENGTH;
    _lengthErrors++;
    return false;
  }

  _goodFrames++;
  _synchro = 0xFF;
  _lastStatus = OK;
  return true;
}

bool FrSky_FBUSClass::processTelemetry()
{
  return FrSky_FBUS_Telemetry.process(_serial);
}

bool FrSky_FBUSClass::decodeRcFrame(const uint8_t *payload, uint8_t payloadLen)
{
  uint8_t count;
  uint8_t packedBytes;

  if (payloadLen == CONTROL_LEN_16CH) {
    count = 16;
    packedBytes = 22;
  } else if (payloadLen == CONTROL_LEN_24CH) {
    count = 24;
    packedBytes = 33;
  } else {
    return false;
  }

  if (_expectedChannels == 16 && count == 24) {
    // Accept 24ch frames even when 16ch was expected; user can still read the first 16.
  }

  for (uint8_t i = 0; i < count; i++) {
    _channels[i] = decode11Bit(payload, (uint16_t)i * 11);
  }
  for (uint8_t i = count; i < MAX_FBUS_CHANNELS; i++) {
    _channels[i] = 0;
  }

  const uint8_t flags = payload[packedBytes];
  _rssi = payload[packedBytes + 1];

  // Same common SBUS flag convention. FBUS/F.Port2 frames normally carry compatible flags here.
  _frameLost = (flags & 0x04) != 0;
  _failsafe  = (flags & 0x08) != 0;
  _channelCount = count;

  return true;
}

uint16_t FrSky_FBUSClass::decode11Bit(const uint8_t *data, uint16_t bitIndex)
{
  const uint16_t byteIndex = bitIndex >> 3;
  const uint8_t bitOffset = bitIndex & 0x07;

  uint32_t value = (uint32_t)data[byteIndex] |
                   ((uint32_t)data[byteIndex + 1] << 8) |
                   ((uint32_t)data[byteIndex + 2] << 16);

  value >>= bitOffset;
  return (uint16_t)(value & 0x07FF);
}

uint16_t FrSky_FBUSClass::rawToUs(uint16_t raw)
{
  // Standard FrSky/SBUS-like useful range: 172 -> 988 us, 1811 -> 2012 us.
  // Clamped and remapped to a convenient 1000..2000 us API.
  if (raw < 172) raw = 172;
  if (raw > 1811) raw = 1811;
  return (uint16_t)map(raw, 172, 1811, 1000, 2000);
}

bool FrSky_FBUSClass::frskyChecksumIsGood(const uint8_t *data, uint8_t len)
{
  uint16_t checksum = 0;
  for (uint8_t i = 0; i < len; i++) {
    checksum += data[i];
    checksum += checksum >> 8;
    checksum &= 0x00FF;
  }
  return checksum == 0x00FF;
}

void FrSky_FBUSClass::setTelemetryEnabled(bool enabled) { FrSky_FBUS_Telemetry.setEnabled(enabled); }
bool FrSky_FBUSClass::telemetryEnabled() const { return FrSky_FBUS_Telemetry.enabled(); }
void FrSky_FBUSClass::setTelemetryResponseDelayUs(uint16_t delayUs) { FrSky_FBUS_Telemetry.setResponseDelayUs(delayUs); }
void FrSky_FBUSClass::setVoltage_V(float volts) { FrSky_FBUS_Telemetry.setVoltage_V(volts); }
void FrSky_FBUSClass::setCurrent_A(float amps) { FrSky_FBUS_Telemetry.setCurrent_A(amps); }
void FrSky_FBUSClass::setVoltageRaw_cV(uint32_t centiVolts) { FrSky_FBUS_Telemetry.setVoltageRaw_cV(centiVolts); }
void FrSky_FBUSClass::setCurrentRaw_dA(uint32_t deciAmps) { FrSky_FBUS_Telemetry.setCurrentRaw_dA(deciAmps); }
void FrSky_FBUSClass::setTemperature1_C(float celsius) { FrSky_FBUS_Telemetry.setTemperature1_C(celsius); }
void FrSky_FBUSClass::setTemperature2_C(float celsius) { FrSky_FBUS_Telemetry.setTemperature2_C(celsius); }
void FrSky_FBUSClass::setTemperature1Raw_C(int32_t celsius) { FrSky_FBUS_Telemetry.setTemperature1Raw_C(celsius); }
void FrSky_FBUSClass::setTemperature2Raw_C(int32_t celsius) { FrSky_FBUS_Telemetry.setTemperature2Raw_C(celsius); }
void FrSky_FBUSClass::setRpm(uint32_t rpm) { FrSky_FBUS_Telemetry.setRpm(rpm); }
void FrSky_FBUSClass::setGpsEnabled(bool enabled) { FrSky_FBUS_Telemetry.setGpsEnabled(enabled); }
bool FrSky_FBUSClass::gpsEnabled() const { return FrSky_FBUS_Telemetry.gpsEnabled(); }
void FrSky_FBUSClass::setGpsFix(bool fix, uint8_t sats) { FrSky_FBUS_Telemetry.setGpsFix(fix, sats); }
void FrSky_FBUSClass::setGps(uint8_t sats, int32_t latE7, int32_t lonE7, int32_t altCm, uint32_t speedCms, uint16_t courseDeg100) { FrSky_FBUS_Telemetry.setGps(sats, latE7, lonE7, altCm, speedCms, courseDeg100); }
const FrSkyFbusGpsData &FrSky_FBUSClass::gps() const { return FrSky_FBUS_Telemetry.gps(); }
bool FrSky_FBUSClass::gpsProcessNMEA(Stream &gpsSerial) { return FrSky_FBUS_Telemetry.gpsProcessNMEA(gpsSerial); }
bool FrSky_FBUSClass::gpsProcessUBlox(Stream &gpsSerial) { return FrSky_FBUS_Telemetry.gpsProcessUBlox(gpsSerial); }
bool FrSky_FBUSClass::gpsProcessNmeaChar(char c) { return FrSky_FBUS_Telemetry.gpsProcessNmeaChar(c); }
bool FrSky_FBUSClass::gpsProcessUbxByte(uint8_t b) { return FrSky_FBUS_Telemetry.gpsProcessUbxByte(b); }
uint32_t FrSky_FBUSClass::telemetryRequests() const { return FrSky_FBUS_Telemetry.requests(); }
uint32_t FrSky_FBUSClass::telemetryResponses() const { return FrSky_FBUS_Telemetry.responses(); }

uint8_t FrSky_FBUSClass::channelCount() const { return _channelCount; }

uint16_t FrSky_FBUSClass::channelRaw(uint8_t index) const
{
  if (index >= _channelCount || index >= MAX_FBUS_CHANNELS) return 0;
  return _channels[index];
}

uint16_t FrSky_FBUSClass::channelUs(uint8_t index) const
{
  if (index >= _channelCount || index >= MAX_FBUS_CHANNELS) return 0;
  return rawToUs(_channels[index]);
}

bool FrSky_FBUSClass::failsafe() const { return _failsafe; }
bool FrSky_FBUSClass::frameLost() const { return _frameLost; }
uint8_t FrSky_FBUSClass::rssi() const { return _rssi; }
FrSky_FBUSClass::Status FrSky_FBUSClass::lastStatus() const { return _lastStatus; }
uint32_t FrSky_FBUSClass::goodFrames() const { return _goodFrames; }
uint32_t FrSky_FBUSClass::crcErrors() const { return _crcErrors; }
uint32_t FrSky_FBUSClass::lengthErrors() const { return _lengthErrors; }
uint32_t FrSky_FBUSClass::typeErrors() const { return _typeErrors; }
const uint8_t *FrSky_FBUSClass::lastRawFrame() const { return _lastRaw; }
uint8_t FrSky_FBUSClass::lastRawFrameLength() const { return _lastRawLen; }

uint8_t FrSky_FBUSClass::isSynchro(uint8_t synchroClientIdx)
{
  const uint8_t ret = !!(_synchro & RCUL_CLIENT_MASK(synchroClientIdx));
  if (ret) {
    _synchro &= ~RCUL_CLIENT_MASK(synchroClientIdx);
  }
  return ret;
}

uint8_t FrSky_FBUSClass::RculIsSynchro(uint8_t ClientIdx)
{
  return isSynchro(ClientIdx);
}

uint16_t FrSky_FBUSClass::RculGetWidth_us(uint8_t Ch)
{
  // Rcul/SBusRx convention: channels are 1..N.
  if (Ch >= 1 && Ch <= _channelCount) {
    return channelUs((uint8_t)(Ch - 1));
  }
  return 1500;
}

void FrSky_FBUSClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}

FrSky_FBUSClass FrSky_FBUS; /* Object externalisation */
