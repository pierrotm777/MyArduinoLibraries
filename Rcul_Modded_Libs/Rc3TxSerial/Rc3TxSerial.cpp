#include "Rc3TxSerial.h"
#include <string.h>

Rc3TxSerial *Rc3TxSerial::last = NULL;

Rc3TxSerial::Rc3TxSerial(Rcul *Rcul,
                         uint8_t RepeatNb,
                         uint8_t TxFifoSize,
                         uint8_t Ch)
{
  _Rcul = Rcul;
  _Ch = Ch;
  _RepeatNb = RepeatNb;
  if(_RepeatNb > RC3_TX_SERIAL_REPEAT4) _RepeatNb = RC3_TX_SERIAL_REPEAT4;

  if(TxFifoSize < 2) TxFifoSize = 2;
  if(TxFifoSize > RC3_TX_SERIAL_FIFO_MAX) TxFifoSize = RC3_TX_SERIAL_FIFO_MAX;
  _TxFifoSize = TxFifoSize;
  _TxFifoTail = 0;
  _TxFifoHead = 0;

  _TritWidthUs[0] = 1000;
  _TritWidthUs[1] = 1500;
  _TritWidthUs[2] = 2000;

  _FrameLen = 0;
  _FramePos = 0;
  _FrameActive = 0;
  _IdleLeft = 0;
  _Sequence = 0;

  _CurrentTrit = 1;
  _CurrentTritValid = 0;
  _CurrentTritSentCnt = 0;

  _LearnActive = 0;
  _LearnTrit = 0;
  _LearnSamplesPerLevel = RC3_TX_SERIAL_LEARN_SAMPLES_PER_LEVEL;
  _LearnSamplesLeft = _LearnSamplesPerLevel;

  prev = last;
  last = this;
}

void Rc3TxSerial::reassignRculDst(Rcul *Rcul)
{
  _Rcul = Rcul;
}

void Rc3TxSerial::setCh(uint8_t Ch)
{
  _Ch = Ch;
}

uint8_t Rc3TxSerial::getCh(void) const
{
  return _Ch;
}

void Rc3TxSerial::setRepeatNb(uint8_t RepeatNb)
{
  if(RepeatNb > RC3_TX_SERIAL_REPEAT4) RepeatNb = RC3_TX_SERIAL_REPEAT4;
  _RepeatNb = RepeatNb;
}

uint8_t Rc3TxSerial::getRepeatNb(void) const
{
  return _RepeatNb;
}

void Rc3TxSerial::setTritWidths(uint16_t Trit0Us, uint16_t Trit1Us, uint16_t Trit2Us)
{
  _TritWidthUs[0] = Trit0Us;
  _TritWidthUs[1] = Trit1Us;
  _TritWidthUs[2] = Trit2Us;
}

uint16_t Rc3TxSerial::getTritWidth_us(uint8_t Trit) const
{
  if(Trit > 2) Trit = 1;
  return _TritWidthUs[Trit];
}


uint8_t Rc3TxSerial::startLearn(uint8_t SamplesPerLevel)
{
  /*
    Learn may interrupt the harmless idle physical trit already being
    repeated, but never an RC3 frame and never queued Stream data.
  */
  if(_FrameActive || _IdleLeft || !fifoEmpty() || _LearnActive)
    return 0;

  if(SamplesPerLevel < 3U)
    SamplesPerLevel = 3U;

  _LearnSamplesPerLevel = SamplesPerLevel;
  _LearnSamplesLeft = _LearnSamplesPerLevel;
  _LearnTrit = 0;
  _LearnActive = 1;

  /* Apply the first Learn level on the very next Rcul synchro. */
  _CurrentTritValid = 0;
  _CurrentTritSentCnt = 0;

  return 1;
}

void Rc3TxSerial::stopLearn(void)
{
  if(!_LearnActive)
    return;

  _LearnActive = 0;
  _LearnTrit = 0;
  _LearnSamplesLeft = _LearnSamplesPerLevel;

  /*
    Force a clean idle=1 separator before the next RC3 SYNC.  This also
    guarantees that AUTO Repeat detection on Rc3RxSerial sees an isolated
    physical 222 run.
  */
  _CurrentTritValid = 0;
  _CurrentTritSentCnt = 0;
  _IdleLeft = RC3_TX_SERIAL_IDLE_TRITS;
}

uint8_t Rc3TxSerial::learnActive(void) const
{
  return _LearnActive;
}


void Rc3TxSerial::printInfo(Stream &Out) const
{
  Out.println(F("----------------------------------------"));
  Out.print(F("Rc3TxSerial V"));
  Out.println(version());

  Out.print(F("Protocol      : RC3 v0."));
  Out.println(RC3_TX_SERIAL_PROTOCOL_VERSION);

  Out.print(F("Channel       : "));
  if(_Ch == RCUL_NO_CH)
    Out.println(F("RCUL_NO_CH"));
  else
    Out.println(_Ch);

  Out.print(F("RepeatNb      : "));
  Out.print(_RepeatNb);
  Out.print(F(" ("));
  Out.print((uint8_t)(_RepeatNb + 1U));
  Out.println(F(" physical samples/trit)"));

  Out.print(F("Trit widths   : "));
  Out.print(_TritWidthUs[0]);
  Out.print(F(" / "));
  Out.print(_TritWidthUs[1]);
  Out.print(F(" / "));
  Out.print(_TritWidthUs[2]);
  Out.println(F(" us"));

  Out.println(F("Pattern use   : RX 3-level calibration"));

  Out.print(F("Pattern mode  : "));
  Out.println(_LearnActive ? F("ON") : F("OFF"));

  Out.print(F("Pattern       : 0 -> 1 -> 2, "));
  Out.print(_LearnSamplesPerLevel);
  Out.println(F(" physical frames/level"));

  if(_LearnActive)
  {
    Out.print(F("Pattern level : "));
    Out.print(_LearnTrit);
    Out.print(F(" ("));
    Out.print(_LearnSamplesLeft);
    Out.println(F(" frames left)"));
  }

  Out.println(F("TX feedback   : NONE"));
  Out.println(F("----------------------------------------"));
}

uint8_t Rc3TxSerial::fifoEmpty(void) const
{
  return (_TxFifoHead == _TxFifoTail);
}

uint8_t Rc3TxSerial::TxFifoRead(uint8_t *TxChar)
{
  if(fifoEmpty()) return 0;

  *TxChar = _TxFifo[_TxFifoHead];
  _TxFifoHead = (uint8_t)((_TxFifoHead + 1) % _TxFifoSize);
  return 1;
}

size_t Rc3TxSerial::write(uint8_t b)
{
  if(_LearnActive) return 0;

  const uint8_t Next = (uint8_t)((_TxFifoTail + 1) % _TxFifoSize);
  if(Next == _TxFifoHead) return 0;

  _TxFifo[_TxFifoTail] = b;
  _TxFifoTail = Next;
  return 1;
}

int Rc3TxSerial::read()
{
  return -1;
}

int Rc3TxSerial::available()
{
  return 0;
}

void Rc3TxSerial::flush()
{
  _TxFifoHead = _TxFifoTail = 0;
}

int Rc3TxSerial::peek()
{
  if(fifoEmpty()) return -1;
  return _TxFifo[_TxFifoHead];
}

uint8_t Rc3TxSerial::isReadyForTx(void) const
{
  return (uint8_t)(!_LearnActive &&
                   !_FrameActive &&
                   !_IdleLeft &&
                   fifoEmpty() &&
                   !_CurrentTritValid);
}

uint8_t Rc3TxSerial::crc4PushBit(uint8_t Crc, uint8_t Bit) const
{
  const uint8_t Feedback = (uint8_t)(((Crc >> 3) & 0x01) ^ (Bit & 0x01));
  Crc = (uint8_t)((Crc << 1) & 0x0F);
  if(Feedback) Crc ^= 0x03;
  return Crc;
}

uint8_t Rc3TxSerial::computeCrc4(uint8_t Header, const uint8_t *Data, uint8_t Len) const
{
  uint8_t Crc = 0x0F;

  for(int8_t Bit = 2; Bit >= 0; --Bit)
    Crc = crc4PushBit(Crc, (Header >> Bit) & 0x01);

  for(uint8_t Idx = 0; Idx < Len; ++Idx)
    for(int8_t Bit = 7; Bit >= 0; --Bit)
      Crc = crc4PushBit(Crc, (Data[Idx] >> Bit) & 0x01);

  return (uint8_t)(Crc & 0x0F);
}

void Rc3TxSerial::appendTrit(uint8_t Trit)
{
  if(_FrameLen < RC3_TX_SERIAL_MAX_FRAME_TRITS)
    _FrameTrits[_FrameLen++] = Trit;
}

void Rc3TxSerial::append3Bits(uint8_t Value)
{
  appendTrit((uint8_t)(Value / 3));
  appendTrit((uint8_t)(Value % 3));
}

void Rc3TxSerial::packBit(uint8_t Bit, uint8_t *Acc, uint8_t *Count)
{
  *Acc = (uint8_t)((*Acc << 1) | (Bit & 0x01));
  (*Count)++;

  if(*Count == 3)
  {
    append3Bits(*Acc);
    *Acc = 0;
    *Count = 0;
  }
}

void Rc3TxSerial::buildFrame(const uint8_t *Data, uint8_t Len)
{
  if(Len < 1) Len = 1;
  if(Len > RC3_TX_SERIAL_MAX_PAYLOAD) Len = RC3_TX_SERIAL_MAX_PAYLOAD;

  _FrameLen = 0;
  _FramePos = 0;

  /* SYNC 2220 */
  appendTrit(2);
  appendTrit(2);
  appendTrit(2);
  appendTrit(0);

  const uint8_t Header = (uint8_t)(((_Sequence & 0x01) << 2) | ((Len - 1) & 0x03));
  append3Bits(Header);

  const uint8_t Crc = computeCrc4(Header, Data, Len);
  uint8_t Acc = 0;
  uint8_t Count = 0;

  for(uint8_t Idx = 0; Idx < Len; ++Idx)
    for(int8_t Bit = 7; Bit >= 0; --Bit)
      packBit((Data[Idx] >> Bit) & 0x01, &Acc, &Count);

  for(int8_t Bit = 3; Bit >= 0; --Bit)
    packBit((Crc >> Bit) & 0x01, &Acc, &Count);

  if(Count)
  {
    Acc <<= (3 - Count);
    append3Bits(Acc);
  }

  _FrameActive = 1;
  _IdleLeft = 0;
  _CurrentTritValid = 0;
}

uint8_t Rc3TxSerial::sendMsg(const uint8_t *Msg, uint8_t Len)
{
  if(!Msg || Len < 1 || Len > RC3_TX_SERIAL_MAX_PAYLOAD) return 0;
  if(!isReadyForTx()) return 0;

  buildFrame(Msg, Len);
  return 1;
}

uint8_t Rc3TxSerial::sendNibbleMsg(const uint8_t *NibbleMsg,
                                   uint8_t NibbleNb,
                                   uint8_t AddChecksum)
{
  if(!NibbleMsg ||
     (NibbleNb < 1) ||
     (NibbleNb > RC3_TX_SERIAL_COMPAT_MAX_NIBBLES) ||
     !isReadyForTx())
  {
    return 0;
  }

  const uint8_t ByteNb = (uint8_t)((NibbleNb + 1U) / 2U);
  uint8_t Rc3Msg[RC3_TX_SERIAL_MAX_PAYLOAD];

  Rc3Msg[0] =
      (uint8_t)(
          RC3_TX_SERIAL_COMPAT_MAGIC |
          (AddChecksum ? RC3_TX_SERIAL_COMPAT_CHECKSUM_BIT : 0U) |
          (NibbleNb & RC3_TX_SERIAL_COMPAT_NIBBLE_MASK));

  memcpy(&Rc3Msg[1], NibbleMsg, ByteNb);

  /*
    In an odd legacy message the low nibble of the last application byte is
    not useful payload.  Clear it so that the compatibility transport is
    canonical and deterministic.
  */
  if(NibbleNb & 1U)
  {
    Rc3Msg[ByteNb] &= 0xF0U;
  }

  return sendMsg(Rc3Msg, (uint8_t)(ByteNb + 1U));
}

uint8_t Rc3TxSerial::startFrameFromFifo(void)
{
  if(fifoEmpty()) return 0;

  uint8_t Msg[RC3_TX_SERIAL_MAX_PAYLOAD];
  uint8_t Len = 0;

  while(Len < RC3_TX_SERIAL_MAX_PAYLOAD && TxFifoRead(&Msg[Len]))
    ++Len;

  if(!Len) return 0;
  buildFrame(Msg, Len);
  return 1;
}

uint8_t Rc3TxSerial::nextLogicalTrit(void)
{
  if(_FrameActive && _FramePos < _FrameLen)
    return _FrameTrits[_FramePos++];

  if(_FrameActive)
  {
    _FrameActive = 0;
    _Sequence ^= 1;
    _IdleLeft = RC3_TX_SERIAL_IDLE_TRITS;
  }

  if(_IdleLeft)
  {
    --_IdleLeft;
    return 1;
  }

  if(startFrameFromFifo())
    return _FrameTrits[_FramePos++];

  return 1;
}

uint8_t Rc3TxSerial::nextPhysicalTrit(void)
{
  /*
    LEARN bypasses normal RC3 framing and RepeatNb on purpose.  The receiver
    simply needs many real physical observations of each of the three levels.
    Using a fixed PHYSICAL sample count keeps Learn timing independent of the
    configured repetition.
  */
  if(_LearnActive)
  {
    const uint8_t Result = _LearnTrit;

    if(_LearnSamplesLeft > 0U)
      --_LearnSamplesLeft;

    if(_LearnSamplesLeft == 0U)
    {
      _LearnTrit = (uint8_t)((_LearnTrit + 1U) % 3U);
      _LearnSamplesLeft = _LearnSamplesPerLevel;
    }

    return Result;
  }

  if(!_CurrentTritValid)
  {
    _CurrentTrit = nextLogicalTrit();
    _CurrentTritSentCnt = 0;
    _CurrentTritValid = 1;
  }

  const uint8_t Result = _CurrentTrit;
  ++_CurrentTritSentCnt;

  if(_CurrentTritSentCnt >= (uint8_t)(_RepeatNb + 1))
    _CurrentTritValid = 0;

  return Result;
}

uint8_t Rc3TxSerial::serviceOneSync(void)
{
  if(!_Rcul) return 0;

  const uint8_t Trit = nextPhysicalTrit();
  _Rcul->RculSetWidth_us(_TritWidthUs[Trit], _Ch);
  return 1;
}

uint8_t Rc3TxSerial::process(void)
{
  uint8_t Ret = 0;
  uint8_t Idx = 0;

  for(Rc3TxSerial *t = last; t != NULL; t = t->prev)
  {
    if(t->_Rcul && t->_Rcul->RculIsSynchro(Idx++))
      Ret |= t->serviceOneSync();
  }

  return Ret;
}
