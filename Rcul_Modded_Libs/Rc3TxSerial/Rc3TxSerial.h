#ifndef Rc3TxSerial_h
#define Rc3TxSerial_h

/*
  Rc3TxSerial
  ===========
  Lightweight unidirectional serial transport through one RC channel using
  only three physical PWM levels.

  Public class layout intentionally follows RcTxSerial:
    - Rcul destination
    - channel selection
    - repetition setting
    - Stream API
    - isReadyForTx()
    - static process()

  Protocol RC3 v0.4 (validated on PTR-6A):
    trit 0 / 1 / 2
    SYNC 2220
    3-bit value -> 2 trits (00..21, 22 reserved)
    1..4 payload bytes
    CRC4 x^4+x+1, init 0xF
    each physical trit repeated RepeatNb+1 times

  No malloc / no STL.
*/

#include "Arduino.h"
#include <Rcul.h>
#include <inttypes.h>
#include <Stream.h>

#define RC3_TX_SERIAL_VERSION          1
#define RC3_TX_SERIAL_REVISION         3
#define RC3_TX_SERIAL_PROTOCOL_VERSION 4

#define RC3_TX_SERIAL_MAX_PAYLOAD      4
#define RC3_TX_SERIAL_MAX_FRAME_TRITS 30

/*
  RcTxSerial compatibility envelope carried inside a native RC3 payload.

  Byte 0:
    bits 7..5 = 101 : compatibility marker
    bit 4     = legacy AddChecksum flag
    bits 3..0 = useful legacy nibble count (1..6)

  Bytes 1..3:
    legacy payload bytes, packed MSN first exactly like RcTxSerial::sendNibbleMsg().

  Maximum legacy payload = 6 nibbles = 3 bytes.
  With the metadata byte this exactly fits the 4-byte RC3 payload.
*/
#define RC3_TX_SERIAL_COMPAT_MAGIC_MASK       0xE0
#define RC3_TX_SERIAL_COMPAT_MAGIC            0xA0
#define RC3_TX_SERIAL_COMPAT_CHECKSUM_BIT     0x10
#define RC3_TX_SERIAL_COMPAT_NIBBLE_MASK      0x0F
#define RC3_TX_SERIAL_COMPAT_MAX_NIBBLES      6

#ifndef RC3_TX_SERIAL_FIFO_MAX
#define RC3_TX_SERIAL_FIFO_MAX         8
#endif

#ifndef RC3_TX_SERIAL_IDLE_TRITS
#define RC3_TX_SERIAL_IDLE_TRITS       5
#endif

/*
  RX-CALIBRATION PATTERN mode.

  Historical API names startLearn()/stopLearn()/learnActive() are intentionally
  kept for source compatibility.  Rc3TxSerial itself does NOT learn anything:
  it has no feedback measurement.  It only emits long physical blocks
  of trit 0, then trit 1, then trit 2 so an RC3 receiver can measure the
  three real pulse-width clusters through the complete radio path.

  Count is expressed in PHYSICAL RC frames, therefore Learn duration is
  independent of RepeatNb.  25 samples/level is about 0.5 s/level on a
  ~20 ms radio cycle, i.e. a full 0/1/2 Learn cycle about every 1.5 s.
*/
#ifndef RC3_TX_SERIAL_LEARN_SAMPLES_PER_LEVEL
#define RC3_TX_SERIAL_LEARN_SAMPLES_PER_LEVEL 25
#endif

enum
{
  RC3_TX_SERIAL_NO_REPEAT = 0,
  RC3_TX_SERIAL_REPEAT0   = 0,
  RC3_TX_SERIAL_REPEAT1,
  RC3_TX_SERIAL_REPEAT2,
  RC3_TX_SERIAL_REPEAT3,
  RC3_TX_SERIAL_REPEAT4
};

class Rc3TxSerial : public Stream
{
  private:
    Rcul          *_Rcul;
    uint8_t        _Ch;
    uint8_t        _RepeatNb;

    uint8_t        _TxFifoSize;
    uint8_t        _TxFifo[RC3_TX_SERIAL_FIFO_MAX];
    uint8_t        _TxFifoTail;
    uint8_t        _TxFifoHead;

    uint16_t       _TritWidthUs[3];

    uint8_t        _FrameTrits[RC3_TX_SERIAL_MAX_FRAME_TRITS];
    uint8_t        _FrameLen;
    uint8_t        _FramePos;
    uint8_t        _FrameActive;
    uint8_t        _IdleLeft;
    uint8_t        _Sequence;

    uint8_t        _CurrentTrit;
    uint8_t        _CurrentTritValid;
    uint8_t        _CurrentTritSentCnt;

    uint8_t        _LearnActive;
    uint8_t        _LearnTrit;
    uint8_t        _LearnSamplesPerLevel;
    uint8_t        _LearnSamplesLeft;

    class Rc3TxSerial *prev;
    static Rc3TxSerial *last;

    uint8_t        TxFifoRead(uint8_t *TxChar);
    uint8_t        fifoEmpty(void) const;
    uint8_t        crc4PushBit(uint8_t Crc, uint8_t Bit) const;
    uint8_t        computeCrc4(uint8_t Header, const uint8_t *Data, uint8_t Len) const;
    void           appendTrit(uint8_t Trit);
    void           append3Bits(uint8_t Value);
    void           packBit(uint8_t Bit, uint8_t *Acc, uint8_t *Count);
    void           buildFrame(const uint8_t *Data, uint8_t Len);
    uint8_t        startFrameFromFifo(void);
    uint8_t        nextLogicalTrit(void);
    uint8_t        nextPhysicalTrit(void);
    uint8_t        serviceOneSync(void);

  public:
    Rc3TxSerial(Rcul *Rcul,
                uint8_t RepeatNb = RC3_TX_SERIAL_REPEAT2,
                uint8_t TxFifoSize = RC3_TX_SERIAL_FIFO_MAX,
                uint8_t Ch = RCUL_NO_CH);

    void           reassignRculDst(Rcul *Rcul);
    void           setCh(uint8_t Ch);
    uint8_t        getCh(void) const;

    void           setRepeatNb(uint8_t RepeatNb);
    uint8_t        getRepeatNb(void) const;

    void           setTritWidths(uint16_t Trit0Us, uint16_t Trit1Us, uint16_t Trit2Us);
    uint16_t       getTritWidth_us(uint8_t Trit) const;

    /*
      RX-calibration pattern for the remote receiver.

      API names are kept as startLearn()/stopLearn()/learnActive() for backward
      compatibility. startLearn() can be called while the transport is idle.
      While active,
      normal messages are not accepted and the TX continuously emits:
          level 0 -> level 1 -> level 2 -> level 0 -> ...
      SamplesPerLevel is a number of PHYSICAL RC frames, not logical trits.

      stopLearn() returns to normal operation after the usual idle separator.
    */
    uint8_t        startLearn(uint8_t SamplesPerLevel = RC3_TX_SERIAL_LEARN_SAMPLES_PER_LEVEL);
    void           stopLearn(void);
    uint8_t        learnActive(void) const;

    /*
      Human-readable transport status.
      This reports TX state only; the TX has no feedback channel and therefore
      cannot know whether the remote receiver actually completed its calibration.
    */
    void           printInfo(Stream &Out) const;

    /* Exact RC3 message API: payload length 1..4 bytes. Returns 1 if accepted. */
    uint8_t        sendMsg(const uint8_t *Msg, uint8_t Len);

    /*
      RcTxSerial-compatible message API.

      The application keeps the same packed nibble payload and NibbleNb used
      with RcTxSerial::sendNibbleMsg().  The legacy checksum is NOT sent over
      RC3; only the AddChecksum flag is carried.  The ATTiny bridge rebuilds
      the exact historical checksum after the RC3 CRC4 has validated the
      transport.

      Supported useful payload: 1..6 nibbles.
      Returns 1 if the message was accepted.
    */
    uint8_t        sendNibbleMsg(const uint8_t *NibbleMsg,
                                 uint8_t NibbleNb,
                                 uint8_t AddChecksum = 1);

    uint8_t        isReadyForTx(void) const;

    int            peek();
    virtual size_t write(uint8_t byte);
    virtual int    read();
    virtual int    available();
    virtual void   flush();
    using Print::write;

    /* Same service pattern as RcTxSerial::process(). */
    static uint8_t process(void);

    static const char *version(void) { return "1.3.1"; }
};

#endif
