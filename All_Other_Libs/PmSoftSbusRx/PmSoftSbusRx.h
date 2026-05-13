#ifndef PM_SOFT_SBUS_RX_H
#define PM_SOFT_SBUS_RX_H

#include <Arduino.h>
#include <Rcul.h>

#ifndef PM_SOFT_SBUS_RX_ATMEGA328P_ONLY
#define PM_SOFT_SBUS_RX_ATMEGA328P_ONLY 1
#endif

class PmSoftSbusRxClass : public Rcul
{
public:
  PmSoftSbusRxClass();

  void begin(uint8_t pin, uint8_t inverted = 1, int8_t tune = -4);
  void process();

  // RCUL compatibility: signatures MUST match Rcul.h pure virtual methods.
  uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
  uint16_t RculGetWidth_us(uint8_t Ch);
  void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = RCUL_NO_CH);

  uint16_t width_us(uint8_t ch) const;
  bool isSynchro() const { return (_validCount != 0) && !_failsafe; }
  bool failsafe() const { return _failsafe; }

  uint16_t raw(uint8_t ch) const;
  uint32_t frameCount() const { return _validCount; }
  uint32_t validCount() const { return _validCount; }
  uint32_t rejectCount() const { return _rejectCount; }
  uint32_t byteCount() const { return _byteCount; }
  uint32_t edgeCount() const { return _edgeCount; }

  void printFrameHex(Stream &s) const;

  // Diagnostic: 0 = auto, 22 = accept early next 0x0F at byte 22, 25 = standard SBUS.
  void setShortFrameMode(uint8_t mode) { _shortFrameMode = mode; }

private:
  static const uint8_t CH_NB = 16;
  static const uint8_t FRAME_NB = 25;
  static const uint8_t RX_BUF_NB = 96;

  volatile uint8_t _rxBuf[RX_BUF_NB];
  volatile uint8_t _rxHead;
  volatile uint8_t _rxTail;
  volatile uint32_t _edgeCount;
  volatile uint32_t _byteCount;

  uint8_t _frame[FRAME_NB];
  uint8_t _lastFrame[FRAME_NB];
  uint8_t _pos;
  uint16_t _raw[CH_NB];
  uint16_t _us[CH_NB];
  bool _failsafe;
  uint32_t _validCount;
  uint32_t _rejectCount;
  uint8_t _shortFrameMode;

  uint8_t _pin;
  uint8_t _inverted;
  int8_t _tune;

  void pushByteFromISR(uint8_t b);
  bool popByte(uint8_t &b);
  bool validateAndDecode(uint8_t len);
  void decodeFrame(uint8_t len, uint8_t flagsIndex);
  static uint16_t rawToUs(uint16_t raw);

  friend void pmSoftSbusRxHandlePCINT0();
};

extern PmSoftSbusRxClass PmSoftSbusRx;

#endif
