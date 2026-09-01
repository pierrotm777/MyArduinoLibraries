#include "Rc3RxSerial.h"
#include <string.h>

enum
{
  RC3_PHY_SEARCH_SYNC = 0,
  RC3_PHY_SYNC_ZERO_GROUP,
  RC3_PHY_DATA_GROUP
};

enum
{
  RC3_RX_HEADER_TRIT_1 = 0,
  RC3_RX_HEADER_TRIT_2,
  RC3_RX_DATA_TRIT_1,
  RC3_RX_DATA_TRIT_2
};

Rc3RxSerial::Rc3RxSerial(Rcul *Rcul,
                         uint8_t RepeatNb,
                         uint8_t Ch,
                         uint8_t ClientIdx)
{
  _Rcul = NULL;

  /*
    AUTO is now the default.  RepeatNb is still accepted for backward
    compatibility and for forced/manual diagnostics.

    Keep Repeat2 as the harmless initial working value until the first
    complete SYNC run has identified the real repetition.
  */
  if(RepeatNb == RC3_RX_SERIAL_REPEAT_AUTO)
  {
    _AutoRepeat = 1;
    _RepeatDetected = 0;
    _RepeatNb = RC3_RX_SERIAL_REPEAT2;
  }
  else
  {
    _AutoRepeat = 0;
    _RepeatDetected = 1;
    _RepeatNb = RepeatNb;
    if(_RepeatNb > RC3_RX_SERIAL_REPEAT4) _RepeatNb = RC3_RX_SERIAL_REPEAT4;
  }

  /* Standard RC servo defaults. */
  setPulseWindows(800, 1200, 1300, 1700, 1800, 2200);
  updateRepeatDerived();

  _LastWidth_us = 1500;
  _TritAvailable = 0;

  _LastValid = 0;
  _LastSeq = 0;
  _LastLen = 0;
  memset(_LastData, 0, sizeof(_LastData));

  _MessageReady = 0;
  _MessageLen = 0;
  _ReadPos = 0;
  memset(_MessageData, 0, sizeof(_MessageData));

  resetStats();
  reassignRculSrc(Rcul, Ch, ClientIdx);
}

void Rc3RxSerial::reassignRculSrc(Rcul *Rcul, uint8_t Ch, uint8_t ClientIdx)
{
  _Rcul = Rcul;
  _rcSrc.Ch = Ch;
  _rcSrc.ClientIdx = ClientIdx;
  _MessageReady = 0;
  _MessageLen = 0;
  _ReadPos = 0;
  resetToSync();
}

void Rc3RxSerial::setRepeatNb(uint8_t RepeatNb)
{
  if(RepeatNb == RC3_RX_SERIAL_REPEAT_AUTO)
  {
    setAutoRepeat(1);
    return;
  }

  if(RepeatNb > RC3_RX_SERIAL_REPEAT4) RepeatNb = RC3_RX_SERIAL_REPEAT4;

  _AutoRepeat = 0;
  _RepeatDetected = 1;
  _RepeatNb = RepeatNb;
  updateRepeatDerived();
  resetToSync();
}

void Rc3RxSerial::setAutoRepeat(uint8_t Enable)
{
  _AutoRepeat = !!Enable;

  if(_AutoRepeat)
  {
    /*
      The next complete SYNC 222 run determines RepeatNb.
      Do not discard the last known value: it remains useful as a diagnostic
      value until a fresh SYNC has been measured.
    */
    _RepeatDetected = 0;
  }
  else
  {
    _RepeatDetected = 1;
  }

  resetToSync();
}

uint8_t Rc3RxSerial::autoRepeatEnabled(void) const
{
  return _AutoRepeat;
}

uint8_t Rc3RxSerial::repeatDetected(void) const
{
  return _RepeatDetected;
}

uint8_t Rc3RxSerial::getRepeatNb(void) const
{
  return _RepeatNb;
}

uint8_t Rc3RxSerial::getSamplesPerTrit(void) const
{
  return _SamplesPerTrit;
}

void Rc3RxSerial::updateRepeatDerived(void)
{
  _SamplesPerTrit = (uint8_t)(_RepeatNb + 1);
  _MajorityNeeded = (uint8_t)((_SamplesPerTrit / 2) + 1);

  uint8_t MinRun = (uint8_t)(3 * _SamplesPerTrit);
  if(_SamplesPerTrit > 1)
  {
    if(MinRun > 2) MinRun = (uint8_t)(MinRun - 2);
  }
  if(MinRun < 3) MinRun = 3;
  _SyncMin2Run = MinRun;
}

/*
  AUTO repetition detection from the physical SYNC prefix.

  Rc3TxSerial inserts five logical idle trits = 1 between frames, so the
  following SYNC starts with an isolated run of exactly:

      3 * (RepeatNb + 1)

  physical trit-2 samples before the first SYNC zero.

  Expected runs are therefore 3, 6, 9, 12 or 15.  A tolerance of one sample
  accepts an occasional missed/extra observation without making adjacent
  candidates overlap.
*/
int8_t Rc3RxSerial::repeatFromSyncRun(uint8_t Run2) const
{
  int8_t Best = -1;
  uint8_t BestErr = 255;

  for(uint8_t RepeatNb = RC3_RX_SERIAL_REPEAT0;
      RepeatNb <= RC3_RX_SERIAL_REPEAT4;
      ++RepeatNb)
  {
    const uint8_t Expected = (uint8_t)(3U * (RepeatNb + 1U));
    const uint8_t Err = (Run2 > Expected) ?
                        (uint8_t)(Run2 - Expected) :
                        (uint8_t)(Expected - Run2);

    if((Err <= 1U) && (Err < BestErr))
    {
      BestErr = Err;
      Best = (int8_t)RepeatNb;
    }
  }

  return Best;
}

void Rc3RxSerial::applyDetectedRepeat(uint8_t RepeatNb)
{
  if(RepeatNb > RC3_RX_SERIAL_REPEAT4) return;

  if((_RepeatNb != RepeatNb) || !_RepeatDetected)
  {
    _RepeatNb = RepeatNb;
    updateRepeatDerived();
  }

  _RepeatDetected = 1;
}

/*
  The first zero sample has already been consumed when this is called.
  Handle SamplesPerTrit==1 immediately; the V1.0 code waited for another
  sample and therefore could not stay aligned in Repeat0 mode.
*/
void Rc3RxSerial::startSyncZeroGroup(void)
{
  _SyncZeroSamples = 1;
  _SyncZeroCount = 1;

  if(_SamplesPerTrit <= 1U)
  {
    clearPhysicalGroup();
    _RxState = RC3_RX_HEADER_TRIT_1;
    _PhyState = RC3_PHY_DATA_GROUP;
  }
  else
  {
    _PhyState = RC3_PHY_SYNC_ZERO_GROUP;
  }
}

void Rc3RxSerial::setPulseWindows(uint16_t Trit0MinUs, uint16_t Trit0MaxUs,
                                  uint16_t Trit1MinUs, uint16_t Trit1MaxUs,
                                  uint16_t Trit2MinUs, uint16_t Trit2MaxUs)
{
  _PulseMinUs[0] = Trit0MinUs;
  _PulseMaxUs[0] = Trit0MaxUs;
  _PulseMinUs[1] = Trit1MinUs;
  _PulseMaxUs[1] = Trit1MaxUs;
  _PulseMinUs[2] = Trit2MinUs;
  _PulseMaxUs[2] = Trit2MaxUs;
}

uint8_t Rc3RxSerial::crc4PushBit(uint8_t Crc, uint8_t Bit) const
{
  const uint8_t Feedback = (uint8_t)(((Crc >> 3) & 0x01) ^ (Bit & 0x01));
  Crc = (uint8_t)((Crc << 1) & 0x0F);
  if(Feedback) Crc ^= 0x03;
  return Crc;
}

int8_t Rc3RxSerial::pulseToTrit(uint16_t WidthUs) const
{
  for(uint8_t Trit = 0; Trit < 3; ++Trit)
    if(WidthUs >= _PulseMinUs[Trit] && WidthUs <= _PulseMaxUs[Trit])
      return (int8_t)Trit;
  return -1;
}

int8_t Rc3RxSerial::tritPairTo3Bits(uint8_t A, uint8_t B) const
{
  if(A > 2 || B > 2) return -1;
  if(A == 2 && B == 2) return -1;

  const uint8_t Value = (uint8_t)(A * 3 + B);
  return (Value <= 7) ? (int8_t)Value : -1;
}

void Rc3RxSerial::clearPhysicalGroup(void)
{
  _GroupSamples = 0;
  _GroupCount[0] = 0;
  _GroupCount[1] = 0;
  _GroupCount[2] = 0;
}

void Rc3RxSerial::resetToSync(void)
{
  _PhyState = RC3_PHY_SEARCH_SYNC;
  _Sync2Run = 0;
  _SyncZeroSamples = 0;
  _SyncZeroCount = 0;
  clearPhysicalGroup();

  _RxState = RC3_RX_HEADER_TRIT_1;
  _FirstTrit = 0;
  _RxSeq = 0;
  _RxLen = 0;
  memset(_RxData, 0, sizeof(_RxData));
  _RxByteIdx = 0;
  _RxBitInByte = 0;
  _RxCrcCalc = 0x0F;
  _RxCrcRecv = 0;
  _RxUsefulBitsDone = 0;
  _RxUsefulBitsTotal = 0;
}

uint8_t Rc3RxSerial::isDuplicate(void) const
{
  if(!_LastValid) return 0;
  if(_RxSeq != _LastSeq || _RxLen != _LastLen) return 0;

  for(uint8_t Idx = 0; Idx < _RxLen; ++Idx)
    if(_RxData[Idx] != _LastData[Idx]) return 0;

  return 1;
}

void Rc3RxSerial::rememberFrame(void)
{
  _LastValid = 1;
  _LastSeq = _RxSeq;
  _LastLen = _RxLen;
  memcpy(_LastData, _RxData, _RxLen);
}

void Rc3RxSerial::publishFrame(void)
{
  /* One-message mailbox: do not overwrite an unread valid message. */
  if(_MessageReady) return;

  _MessageLen = _RxLen;
  memcpy(_MessageData, _RxData, _RxLen);
  _ReadPos = 0;
  _MessageReady = 1;
}

void Rc3RxSerial::finishFrame(void)
{
  if((_RxCrcCalc & 0x0F) == (_RxCrcRecv & 0x0F))
  {
    if(isDuplicate())
    {
      ++_DuplicateFrames;
    }
    else
    {
      ++_GoodFrames;
      rememberFrame();
      publishFrame();
    }
  }
  else
  {
    ++_CrcErrors;
  }

  resetToSync();
}

void Rc3RxSerial::consumePayloadOrCrcBit(uint8_t Bit)
{
  const uint8_t PayloadBitNb = (uint8_t)(_RxLen * 8);

  if(_RxUsefulBitsDone < PayloadBitNb)
  {
    _RxData[_RxByteIdx] = (uint8_t)((_RxData[_RxByteIdx] << 1) | (Bit & 0x01));
    ++_RxBitInByte;
    _RxCrcCalc = crc4PushBit(_RxCrcCalc, Bit);

    if(_RxBitInByte == 8)
    {
      _RxBitInByte = 0;
      ++_RxByteIdx;
    }
  }
  else
  {
    _RxCrcRecv = (uint8_t)((_RxCrcRecv << 1) | (Bit & 0x01));
  }

  ++_RxUsefulBitsDone;
  if(_RxUsefulBitsDone >= _RxUsefulBitsTotal) finishFrame();
}

void Rc3RxSerial::consume3Bits(uint8_t Value)
{
  for(int8_t Bit = 2; Bit >= 0; --Bit)
  {
    if(_RxUsefulBitsDone >= _RxUsefulBitsTotal) return;
    consumePayloadOrCrcBit((Value >> Bit) & 0x01);
  }
}

void Rc3RxSerial::startFrameFromHeader(uint8_t Header)
{
  _RxSeq = (Header >> 2) & 0x01;
  _RxLen = (uint8_t)((Header & 0x03) + 1);

  if(_RxLen < 1 || _RxLen > RC3_RX_SERIAL_MAX_PAYLOAD)
  {
    ++_SymbolErrors;
    resetToSync();
    return;
  }

  memset(_RxData, 0, sizeof(_RxData));
  _RxByteIdx = 0;
  _RxBitInByte = 0;
  _RxCrcRecv = 0;
  _RxUsefulBitsDone = 0;
  _RxUsefulBitsTotal = (uint8_t)(_RxLen * 8 + 4);
  _RxCrcCalc = 0x0F;

  for(int8_t Bit = 2; Bit >= 0; --Bit)
    _RxCrcCalc = crc4PushBit(_RxCrcCalc, (Header >> Bit) & 0x01);

  _RxState = RC3_RX_DATA_TRIT_1;
}

void Rc3RxSerial::processLogicalTrit(uint8_t Trit)
{
  switch(_RxState)
  {
    case RC3_RX_HEADER_TRIT_1:
      _FirstTrit = Trit;
      _RxState = RC3_RX_HEADER_TRIT_2;
      break;

    case RC3_RX_HEADER_TRIT_2:
    {
      const int8_t Header = tritPairTo3Bits(_FirstTrit, Trit);
      if(Header < 0)
      {
        ++_SymbolErrors;
        resetToSync();
      }
      else startFrameFromHeader((uint8_t)Header);
      break;
    }

    case RC3_RX_DATA_TRIT_1:
      _FirstTrit = Trit;
      _RxState = RC3_RX_DATA_TRIT_2;
      break;

    case RC3_RX_DATA_TRIT_2:
    {
      const int8_t Value = tritPairTo3Bits(_FirstTrit, Trit);
      if(Value < 0)
      {
        ++_SymbolErrors;
        resetToSync();
      }
      else
      {
        _RxState = RC3_RX_DATA_TRIT_1;
        consume3Bits((uint8_t)Value);
      }
      break;
    }

    default:
      resetToSync();
      break;
  }
}

int8_t Rc3RxSerial::groupMajority(void) const
{
  for(uint8_t Trit = 0; Trit < 3; ++Trit)
    if(_GroupCount[Trit] >= _MajorityNeeded) return (int8_t)Trit;
  return -1;
}

void Rc3RxSerial::processPhysicalSample(int8_t Trit)
{
  switch(_PhyState)
  {
    case RC3_PHY_SEARCH_SYNC:
      if(Trit == 2)
      {
        if(_Sync2Run < 255) ++_Sync2Run;
      }
      else if(Trit == 0)
      {
        if(_AutoRepeat)
        {
          const int8_t DetectedRepeat = repeatFromSyncRun(_Sync2Run);

          if(DetectedRepeat >= 0)
          {
            applyDetectedRepeat((uint8_t)DetectedRepeat);
            startSyncZeroGroup();
          }
          else
          {
            _Sync2Run = 0;
          }
        }
        else if(_Sync2Run >= _SyncMin2Run)
        {
          startSyncZeroGroup();
        }
        else
        {
          _Sync2Run = 0;
        }
      }
      else
      {
        _Sync2Run = 0;
      }
      break;

    case RC3_PHY_SYNC_ZERO_GROUP:
      ++_SyncZeroSamples;
      if(Trit == 0) ++_SyncZeroCount;

      if(_SyncZeroSamples >= _SamplesPerTrit)
      {
        if(_SyncZeroCount >= _MajorityNeeded)
        {
          clearPhysicalGroup();
          _RxState = RC3_RX_HEADER_TRIT_1;
          _PhyState = RC3_PHY_DATA_GROUP;
        }
        else
        {
          ++_SymbolErrors;
          resetToSync();
        }
      }
      break;

    case RC3_PHY_DATA_GROUP:
      ++_GroupSamples;
      if(Trit >= 0 && Trit <= 2) ++_GroupCount[(uint8_t)Trit];

      if(_GroupSamples >= _SamplesPerTrit)
      {
        const int8_t LogicalTrit = groupMajority();
        if(LogicalTrit < 0)
        {
          ++_SymbolErrors;
          resetToSync();
        }
        else
        {
          if(_GroupCount[(uint8_t)LogicalTrit] != _SamplesPerTrit)
            ++_MajorityCorrected;

          clearPhysicalGroup();
          processLogicalTrit((uint8_t)LogicalTrit);
        }
      }
      break;

    default:
      resetToSync();
      break;
  }
}

uint8_t Rc3RxSerial::somethingAvailable(void)
{
  if(!_Rcul) return _MessageReady;

  if(_Rcul->RculIsSynchro(_rcSrc.ClientIdx))
  {
    _LastWidth_us = _Rcul->RculGetWidth_us(_rcSrc.Ch);
    _TritAvailable = 1;
    processPhysicalSample(pulseToTrit(_LastWidth_us));
  }

  return _MessageReady;
}

uint8_t Rc3RxSerial::available(void)
{
  somethingAvailable();
  if(!_MessageReady) return 0;
  return (uint8_t)(_MessageLen - _ReadPos);
}

int Rc3RxSerial::read(void)
{
  if(!available()) return -1;

  const uint8_t Value = _MessageData[_ReadPos++];
  if(_ReadPos >= _MessageLen)
  {
    _MessageReady = 0;
    _MessageLen = 0;
    _ReadPos = 0;
  }
  return Value;
}

uint8_t Rc3RxSerial::msgAvailable(char *RxBuf, uint8_t RxBufMaxLen)
{
  somethingAvailable();
  if(!_MessageReady || !RxBuf || !RxBufMaxLen) return 0;

  if(_MessageLen > RxBufMaxLen)
  {
    _MessageReady = 0;
    _MessageLen = 0;
    _ReadPos = 0;
    return 0;
  }

  const uint8_t Len = _MessageLen;
  memcpy(RxBuf, _MessageData, Len);
  _MessageReady = 0;
  _MessageLen = 0;
  _ReadPos = 0;
  return Len;
}

uint16_t Rc3RxSerial::lastWidth_us(void)
{
  _TritAvailable = 0;
  return _LastWidth_us;
}

uint8_t Rc3RxSerial::tritAvailable(void)
{
  return _TritAvailable;
}

void Rc3RxSerial::resetStats(void)
{
  _GoodFrames = 0;
  _DuplicateFrames = 0;
  _CrcErrors = 0;
  _SymbolErrors = 0;
  _MajorityCorrected = 0;
}
