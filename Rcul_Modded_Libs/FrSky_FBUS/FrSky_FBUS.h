#pragma once

#include <Arduino.h>
#include <Rcul.h>
#include "FrSky_FBUS_Telemetry.h"

class FrSky_FBUSClass : public Rcul {
public:
  static constexpr uint32_t FBUS_BAUD = 460800;
  static constexpr uint8_t  MAX_CHANNELS = 24;
  static constexpr uint8_t  DEFAULT_CHANNELS = 16;

  enum CpuType : uint8_t {
    CPU_UNKNOWN = 0,
    CPU_AVR,
    CPU_TEENSY_40,
    CPU_TEENSY_LC,
    CPU_RP2040,
    CPU_ESP32,
    CPU_ESP32_S2,
    CPU_ESP32_S3,
    CPU_ESP32_C3,
    CPU_ESP32_C6,
    CPU_ESP32_H2
  };

  // F.Port2 / FBUS frame constants observed in INAV fport2.c
  static constexpr uint8_t FRAME_TYPE_RC = 0xFF;
  static constexpr uint8_t CONTROL_LEN_16CH = 24; // payload: 22 packed bytes + flags + rssi
  static constexpr uint8_t CONTROL_LEN_24CH = 35; // experimental: 33 packed bytes + flags + rssi

  enum Status : uint8_t {
    OK = 0,
    NO_FRAME,
    BAD_LENGTH,
    BAD_TYPE,
    BAD_CRC,
    TIMEOUT,
    OVERSIZE
  };

  FrSky_FBUSClass(void);

  // Attach a serial port manually, RCUL/RcRxSerial style.
  void serialAttach(Stream *RxStream);

  // Preferred for FBUS: attach + configure the UART at 460800, 8N1, non-inverted.
  void begin(HardwareSerial &serial, uint32_t baud = FBUS_BAUD);

#if defined(ARDUINO_ARCH_ESP32)
  // ESP32 helper: configure the UART with explicit RX/TX pins.
  // FBUS is non-inverted, 460800 baud, 8N1. For RX-only, txPin can be -1.
  void begin(HardwareSerial &serial, int8_t rxPin, int8_t txPin, uint32_t baud = FBUS_BAUD, uint32_t rxBufferSize = 512);
#endif

  // Start using the already attached serial port.
  void begin(uint32_t baud = FBUS_BAUD);
  void end();

  // Call as often as possible from loop(). Returns true when a valid RC frame was decoded.
  bool read();

  // Compile-time CPU/board family helper. Useful for examples and debug prints.
  static CpuType cpuType();
  static const char *cpuName();
  static bool isEsp32Cpu();

  // Optional tuning/debug.
  void setChecksumRequired(bool enabled);
  void setInterByteTimeoutUs(uint32_t timeoutUs);
  void setExpectedChannels(uint8_t count); // 16 or 24. Auto still accepts both.

  // Telemetry is now implemented in FrSky_FBUS_Telemetry.h/.cpp.
  // These wrappers are kept for compatibility with v0.5/v0.6/v0.7 sketches.
  void setTelemetryEnabled(bool enabled);
  bool telemetryEnabled() const;
  void setTelemetryResponseDelayUs(uint16_t delayUs);
  void setVoltage_V(float volts);
  void setCurrent_A(float amps);
  void setVoltageRaw_cV(uint32_t centiVolts);
  void setCurrentRaw_dA(uint32_t deciAmps);
  void setTemperature1_C(float celsius);
  void setTemperature2_C(float celsius);
  void setTemperature1Raw_C(int32_t celsius);
  void setTemperature2Raw_C(int32_t celsius);
  void setRpm(uint32_t rpm);
  void setGpsEnabled(bool enabled);
  bool gpsEnabled() const;
  void setGpsFix(bool fix, uint8_t sats = 0);
  void setGps(uint8_t sats, int32_t latE7, int32_t lonE7, int32_t altCm, uint32_t speedCms, uint16_t courseDeg100);
  const FrSkyFbusGpsData &gps() const;
  bool gpsProcessNMEA(Stream &gpsSerial);
  bool gpsProcessUBlox(Stream &gpsSerial);
  bool gpsProcessNmeaChar(char c);
  bool gpsProcessUbxByte(uint8_t b);
  bool processTelemetry();
  uint32_t telemetryRequests() const;
  uint32_t telemetryResponses() const;

  uint8_t channelCount() const;
  uint16_t channelRaw(uint8_t index) const; // 11-bit raw value, usually around 172..1811
  uint16_t channelUs(uint8_t index) const;  // mapped to 1000..2000 us

  bool failsafe() const;
  bool frameLost() const;
  uint8_t rssi() const;                     // FBUS frame RSSI byte when present

  Status lastStatus() const;
  uint32_t goodFrames() const;
  uint32_t crcErrors() const;
  uint32_t lengthErrors() const;
  uint32_t typeErrors() const;

  const uint8_t *lastRawFrame() const;
  uint8_t lastRawFrameLength() const;

  // Rcul support: same philosophy as RC Navy SBusRx.
  uint8_t isSynchro(uint8_t synchroClientIdx = RCUL_DEFAULT_CLIENT_IDX);
  virtual uint8_t RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
  virtual uint16_t RculGetWidth_us(uint8_t Ch);
  virtual void RculSetWidth_us(uint16_t Width_us, uint8_t Ch = RCUL_NO_CH);

private:
  Stream *_serial;
  HardwareSerial *_hwSerial;

  uint16_t _channels[MAX_CHANNELS];
  uint8_t _channelCount;
  uint8_t _expectedChannels;
  bool _failsafe;
  bool _frameLost;
  uint8_t _rssi;
  uint8_t _synchro;

  bool _checksumRequired;
  uint32_t _interByteTimeoutUs;
  uint32_t _lastByteUs;

  enum ParseState : uint8_t {
    WAIT_LEN,
    WAIT_TYPE,
    WAIT_DATA
  };

  ParseState _state;
  uint8_t _lengthByte;
  uint8_t _typeByte;
  uint8_t _payloadAndCrc[40];
  uint8_t _payloadIndex;
  uint8_t _payloadToRead;

  uint8_t _lastRaw[48];
  uint8_t _lastRawLen;

  Status _lastStatus;
  uint32_t _goodFrames;
  uint32_t _crcErrors;
  uint32_t _lengthErrors;
  uint32_t _typeErrors;

  void resetParser(Status status);
  bool acceptFrame();
  bool decodeRcFrame(const uint8_t *payload, uint8_t payloadLen);
  static uint16_t decode11Bit(const uint8_t *data, uint16_t bitIndex);
  static uint16_t rawToUs(uint16_t raw);
  static bool frskyChecksumIsGood(const uint8_t *data, uint8_t len);
};

extern FrSky_FBUSClass FrSky_FBUS; /* Object externalisation */
