#ifndef Rc3RxSerial_h
#define Rc3RxSerial_h

/*
  Rc3RxSerial
  ===========
  Receiver companion for Rc3TxSerial.

  Public class layout intentionally follows RcRxSerial:
    - Rcul source + channel + client index
    - available(), read(), msgAvailable()
    - reassignRculSrc()
    - lastWidth_us()

  RC3 v0.4:
    - 3 PWM levels -> trits 0/1/2
    - each logical trit is transmitted RepeatNb+1 times
    - majority vote reconstructs one logical trit
    - SYNC 2220
    - 1..4 byte payload
    - CRC4 x^4+x+1, init 0xF

  No malloc / no STL.
*/

#include "Arduino.h"
#include <Rcul.h>
#include <inttypes.h>

#define RC3_RX_SERIAL_VERSION          1
#define RC3_RX_SERIAL_REVISION         1
#define RC3_RX_SERIAL_PROTOCOL_VERSION 4
#define RC3_RX_SERIAL_MAX_PAYLOAD      4

enum
{
  RC3_RX_SERIAL_NO_REPEAT = 0,
  RC3_RX_SERIAL_REPEAT0   = 0,
  RC3_RX_SERIAL_REPEAT1,
  RC3_RX_SERIAL_REPEAT2,
  RC3_RX_SERIAL_REPEAT3,
  RC3_RX_SERIAL_REPEAT4,
  RC3_RX_SERIAL_REPEAT_AUTO = 0xFF
};

typedef struct
{
  uint8_t Ch:5;
  uint8_t ClientIdx:3;
} Rc3SrcSt_t;

class Rc3RxSerial
{
  private:
    Rc3SrcSt_t _rcSrc;
    Rcul      *_Rcul;
    uint8_t    _RepeatNb;
    uint8_t    _AutoRepeat;
    uint8_t    _RepeatDetected;
    uint8_t    _SamplesPerTrit;
    uint8_t    _MajorityNeeded;
    uint8_t    _SyncMin2Run;

    uint16_t   _PulseMinUs[3];
    uint16_t   _PulseMaxUs[3];
    uint16_t   _LastWidth_us;
    uint8_t    _TritAvailable;

    uint8_t    _PhyState;
    uint8_t    _Sync2Run;
    uint8_t    _SyncZeroSamples;
    uint8_t    _SyncZeroCount;
    uint8_t    _GroupSamples;
    uint8_t    _GroupCount[3];

    uint8_t    _RxState;
    uint8_t    _FirstTrit;
    uint8_t    _RxSeq;
    uint8_t    _RxLen;
    uint8_t    _RxData[RC3_RX_SERIAL_MAX_PAYLOAD];
    uint8_t    _RxByteIdx;
    uint8_t    _RxBitInByte;
    uint8_t    _RxCrcCalc;
    uint8_t    _RxCrcRecv;
    uint8_t    _RxUsefulBitsDone;
    uint8_t    _RxUsefulBitsTotal;

    uint8_t    _LastValid;
    uint8_t    _LastSeq;
    uint8_t    _LastLen;
    uint8_t    _LastData[RC3_RX_SERIAL_MAX_PAYLOAD];

    uint8_t    _MessageReady;
    uint8_t    _MessageLen;
    uint8_t    _MessageData[RC3_RX_SERIAL_MAX_PAYLOAD];
    uint8_t    _ReadPos;

    uint32_t   _GoodFrames;
    uint32_t   _DuplicateFrames;
    uint32_t   _CrcErrors;
    uint32_t   _SymbolErrors;
    uint32_t   _MajorityCorrected;

    uint8_t    crc4PushBit(uint8_t Crc, uint8_t Bit) const;
    int8_t     pulseToTrit(uint16_t WidthUs) const;
    int8_t     tritPairTo3Bits(uint8_t A, uint8_t B) const;
    void       updateRepeatDerived(void);
    int8_t     repeatFromSyncRun(uint8_t Run2) const;
    void       applyDetectedRepeat(uint8_t RepeatNb);
    void       startSyncZeroGroup(void);
    void       clearPhysicalGroup(void);
    void       resetToSync(void);
    uint8_t    isDuplicate(void) const;
    void       rememberFrame(void);
    void       publishFrame(void);
    void       finishFrame(void);
    void       consumePayloadOrCrcBit(uint8_t Bit);
    void       consume3Bits(uint8_t Value);
    void       startFrameFromHeader(uint8_t Header);
    void       processLogicalTrit(uint8_t Trit);
    int8_t     groupMajority(void) const;
    void       processPhysicalSample(int8_t Trit);
    uint8_t    somethingAvailable(void);

  public:
    Rc3RxSerial(Rcul *Rcul,
                uint8_t RepeatNb = RC3_RX_SERIAL_REPEAT_AUTO,
                uint8_t Ch = RCUL_NO_CH,
                uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);

    void       reassignRculSrc(Rcul *Rcul,
                               uint8_t Ch = RCUL_NO_CH,
                               uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);

    void       setRepeatNb(uint8_t RepeatNb);
    void       setAutoRepeat(uint8_t Enable = 1);
    uint8_t    autoRepeatEnabled(void) const;
    uint8_t    repeatDetected(void) const;
    uint8_t    getRepeatNb(void) const;
    uint8_t    getSamplesPerTrit(void) const;

    void       setPulseWindows(uint16_t Trit0MinUs, uint16_t Trit0MaxUs,
                               uint16_t Trit1MinUs, uint16_t Trit1MaxUs,
                               uint16_t Trit2MinUs, uint16_t Trit2MaxUs);

    uint8_t    available(void);
    int        read(void);
    uint8_t    msgAvailable(char *RxBuf, uint8_t RxBufMaxLen);

    uint16_t   lastWidth_us(void);
    uint8_t    tritAvailable(void);

    uint32_t   goodFrames(void) const { return _GoodFrames; }
    uint32_t   duplicateFrames(void) const { return _DuplicateFrames; }
    uint32_t   crcErrors(void) const { return _CrcErrors; }
    uint32_t   symbolErrors(void) const { return _SymbolErrors; }
    uint32_t   majorityCorrected(void) const { return _MajorityCorrected; }
    void       resetStats(void);

    static const char *version(void) { return "1.1"; }
};

#endif
