#include "PmSoftSbusRx.h"

#if !defined(__AVR_ATmega328P__)
#error "PmSoftSbusRx v2.5 is intentionally limited to ATmega328P / Arduino Nano / D11 (PB3)."
#endif

PmSoftSbusRxClass PmSoftSbusRx;

// Compatibility shim for RcNavyRcul versions where RculSetWidth_us()
// is declared virtual in Rcul.h but not implemented in the RcNavyRcul library.
// PmSoftSbusRx is receiver-only, so this intentionally does nothing.
void Rcul::RculSetWidth_us(uint16_t Width_us, uint8_t Ch)
{
  (void)Width_us;
  (void)Ch;
}

static PmSoftSbusRxClass *g_sbus = NULL;
static volatile uint8_t g_busy = 0;

static inline uint8_t pmReadRawD11()
{
  return (PINB & _BV(PB3)) ? 1 : 0; // D11 = PB3 on ATmega328P/Nano
}

static inline uint8_t pmReadLogical(uint8_t inverted)
{
  uint8_t v = pmReadRawD11();
  return inverted ? !v : v;
}

void pmSoftSbusRxHandlePCINT0()
{
  if(!g_sbus || g_busy) return;

  g_sbus->_edgeCount++;

  // SBUS after normalization is UART-like: idle=1, start=0.
  if(pmReadLogical(g_sbus->_inverted) != 0) return;

  g_busy = 1;

  int16_t firstDelay = 15 + g_sbus->_tune; // center of first data bit
  if(firstDelay < 8) firstDelay = 8;
  if(firstDelay > 22) firstDelay = 22;
  delayMicroseconds((uint8_t)firstDelay);

  uint8_t b = 0;
  for(uint8_t i = 0; i < 8; i++)
  {
    if(pmReadLogical(g_sbus->_inverted)) b |= (1 << i); // LSB first
    delayMicroseconds(10);
  }

  // Ignore parity + 2 stop bits. Do not wait too long: next byte start comes soon.
  delayMicroseconds(12);

  g_sbus->pushByteFromISR(b);
  g_busy = 0;
}

ISR(PCINT0_vect)
{
  pmSoftSbusRxHandlePCINT0();
}

PmSoftSbusRxClass::PmSoftSbusRxClass()
: _rxHead(0), _rxTail(0), _edgeCount(0), _byteCount(0), _pos(0),
  _failsafe(true), _validCount(0), _rejectCount(0), _shortFrameMode(0),
  _pin(11), _inverted(1), _tune(-4)
{
  memset(_frame, 0, sizeof(_frame));
  memset(_lastFrame, 0, sizeof(_lastFrame));
  memset(_raw, 0, sizeof(_raw));
  memset(_us, 0, sizeof(_us));
}

void PmSoftSbusRxClass::begin(uint8_t pin, uint8_t inverted, int8_t tune)
{
  _pin = pin;
  _inverted = inverted ? 1 : 0;
  _tune = tune;
  _rxHead = _rxTail = 0;
  _edgeCount = _byteCount = 0;
  _pos = 0;
  _validCount = _rejectCount = 0;
  _failsafe = true;
  memset(_frame, 0, sizeof(_frame));
  memset(_lastFrame, 0, sizeof(_lastFrame));
  memset(_raw, 0, sizeof(_raw));
  memset(_us, 0, sizeof(_us));

  // Restrictive by design: D11 only.
  pinMode(11, INPUT);
  g_sbus = this;

  cli();
  PCICR  |= _BV(PCIE0);    // enable PCINT for port B
  PCMSK0 |= _BV(PCINT3);   // D11/PB3
  sei();
}

void PmSoftSbusRxClass::pushByteFromISR(uint8_t b)
{
  uint8_t n = (uint8_t)(_rxHead + 1);
  if(n >= RX_BUF_NB) n = 0;
  if(n != _rxTail)
  {
    _rxBuf[_rxHead] = b;
    _rxHead = n;
    _byteCount++;
  }
}

bool PmSoftSbusRxClass::popByte(uint8_t &b)
{
  if(_rxTail == _rxHead) return false;
  b = _rxBuf[_rxTail];
  uint8_t n = (uint8_t)(_rxTail + 1);
  if(n >= RX_BUF_NB) n = 0;
  _rxTail = n;
  return true;
}

void PmSoftSbusRxClass::process()
{
  uint8_t b;
  while(popByte(b))
  {
    if(_pos == 0)
    {
      if(b == 0x0F)
      {
        _frame[0] = b;
        _pos = 1;
      }
      continue;
    }

    // If a new SBUS start appears early, try to validate the short frame first.
    // Your logs often show: 0F + 21 bytes + next 0F.
    if(b == 0x0F && _pos >= 20 && _pos <= 24)
    {
      bool ok = false;
      if(_shortFrameMode == 0 || _shortFrameMode == 22)
      {
        if(_pos == 22) ok = validateAndDecode(22);
      }
      if(!ok) _rejectCount++;

      // Start a new candidate frame with current 0x0F.
      _frame[0] = 0x0F;
      _pos = 1;
      continue;
    }

    if(_pos < FRAME_NB) _frame[_pos++] = b;

    if(_pos >= FRAME_NB)
    {
      if(!validateAndDecode(FRAME_NB)) _rejectCount++;
      _pos = 0;
    }
  }
}

bool PmSoftSbusRxClass::validateAndDecode(uint8_t len)
{
  if(_frame[0] != 0x0F) return false;

  uint8_t flagsIndex = 0;

  if(len == 25)
  {
    uint8_t footer = _frame[24];
    if(!(footer == 0x00 || footer == 0x04 || footer == 0x14 || footer == 0x24 || footer == 0x34)) return false;
    flagsIndex = 23;
  }
  else if(len == 22)
  {
    // Compatibility with the current soft capture: the next 0x0F often arrives early.
    // bytes 20/21 are then treated as flags/footer. Only lower channels are reliable.
    uint8_t footer = _frame[21];
    if(!(footer == 0x00 || footer == 0x04 || footer == 0x14 || footer == 0x24 || footer == 0x34)) return false;
    flagsIndex = 20;
  }
  else
  {
    return false;
  }

  // Decode to temporary arrays first. Do NOT update the public channels if the
  // candidate frame is not physically plausible. This avoids jumps like raw=100.
  uint16_t tmpRaw[CH_NB];
  uint16_t tmpUs[CH_NB];
  for(uint8_t i = 0; i < CH_NB; i++)
  {
    tmpRaw[i] = _raw[i];
    tmpUs[i]  = _us[i];
  }

  uint32_t bitIndex = 0;
  for(uint8_t ch = 0; ch < CH_NB; ch++)
  {
    // Need all bits of this channel before the flags byte.
    uint16_t lastByteIndex = 1 + ((bitIndex + 10) >> 3);
    if(lastByteIndex >= flagsIndex) break;

    uint16_t v = 0;
    for(uint8_t bit = 0; bit < 11; bit++)
    {
      uint16_t byteIndex = 1 + ((bitIndex + bit) >> 3);
      uint8_t bitPos = (uint8_t)((bitIndex + bit) & 7);
      if(_frame[byteIndex] & (1 << bitPos)) v |= (1 << bit);
    }
    tmpRaw[ch] = v;
    tmpUs[ch] = rawToUs(v);
    bitIndex += 11;
  }

  // SBUS channels should normally stay in the 172..1811 range.
  // The soft decoder may occasionally accept a shifted frame; reject it before
  // it corrupts the channels. Check first 8 channels, enough for MS8/X-Any tests.
  for(uint8_t ch = 0; ch < 8; ch++)
  {
    if(tmpRaw[ch] < 172 || tmpRaw[ch] > 1811) return false;
  }

  memcpy(_lastFrame, _frame, len);
  if(len < FRAME_NB) memset(_lastFrame + len, 0, FRAME_NB - len);

  uint8_t flags = _frame[flagsIndex];
  _failsafe = (flags & 0x08) ? true : false;

  for(uint8_t ch = 0; ch < CH_NB; ch++)
  {
    _raw[ch] = tmpRaw[ch];
    _us[ch] = tmpUs[ch];
  }

  _validCount++;
  return true;
}

void PmSoftSbusRxClass::decodeFrame(uint8_t len, uint8_t flagsIndex)
{
  memcpy(_lastFrame, _frame, len);
  if(len < FRAME_NB) memset(_lastFrame + len, 0, FRAME_NB - len);

  uint8_t flags = _frame[flagsIndex];
  _failsafe = (flags & 0x08) ? true : false;

  // Decode 16 channels, little endian 11-bit SBUS, from data bytes 1..22.
  uint32_t bitIndex = 0;
  for(uint8_t ch = 0; ch < CH_NB; ch++)
  {
    uint16_t v = 0;
    for(uint8_t bit = 0; bit < 11; bit++)
    {
      uint16_t byteIndex = 1 + ((bitIndex + bit) >> 3);
      uint8_t bitPos = (uint8_t)((bitIndex + bit) & 7);
      if(byteIndex < len && (_frame[byteIndex] & (1 << bitPos))) v |= (1 << bit);
    }
    _raw[ch] = v;
    _us[ch] = rawToUs(v);
    bitIndex += 11;
  }

  _validCount++;
}


uint8_t PmSoftSbusRxClass::RculIsSynchro(uint8_t ClientIdx)
{
  (void)ClientIdx;
  return ((_validCount != 0) && !_failsafe) ? 1 : 0;
}

uint16_t PmSoftSbusRxClass::RculGetWidth_us(uint8_t Ch)
{
  return width_us(Ch);
}

void PmSoftSbusRxClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch)
{
  // Receiver-only RCUL source: nothing to set.
  (void)Width_us;
  (void)Ch;
}

uint16_t PmSoftSbusRxClass::rawToUs(uint16_t raw)
{
  // Common SBUS range. Clamp hard to keep RCUL safe.
  if(raw < 172) raw = 172;
  if(raw > 1811) raw = 1811;
  return 1000 + ((uint32_t)(raw - 172) * 1000UL) / (1811 - 172);
}

uint16_t PmSoftSbusRxClass::width_us(uint8_t ch) const
{
  if(ch < 1 || ch > CH_NB) return 1500;
  if(_raw[ch - 1] == 0) return 1500;
  return _us[ch - 1];
}

uint16_t PmSoftSbusRxClass::raw(uint8_t ch) const
{
  if(ch < 1 || ch > CH_NB) return 0;
  return _raw[ch - 1];
}

void PmSoftSbusRxClass::printFrameHex(Stream &s) const
{
  s.print(F("FR="));
  for(uint8_t i = 0; i < FRAME_NB; i++)
  {
    s.print(' ');
    if(_lastFrame[i] < 16) s.print('0');
    s.print(_lastFrame[i], HEX);
  }
  s.println();
}
