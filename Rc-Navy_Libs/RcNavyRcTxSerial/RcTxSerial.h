#ifndef RcTxSerial_h
#define RcTxSerial_h

/*
 English: by RC Navy (2012)
 =======
 <RcTxSerial>: a library to build an unidirectionnal serial port through RC Transmitter/Receiver.
 http://p.loussouarn.free.fr
 V1.0: (07/11/2022) initial release
 V1.1: (25/06/2023) When sendNibbleMsg() is used with checksum, take care of the final ^ 0x55
 V1.2: (29/12/2023) Troncated message when using nibble mode with at least one repetition and odd nibble number

 Francais: par RC Navy (2012)
 ========
 <RcTxSerial>: une bibliotheque pour construire un port serie unidirectionnel a travers un Emetteur/Recepteur RC.
 http://p.loussouarn.free.fr
 V1.0: (07/11/2022) release initiale
 V1.1: (25/06/2023) Quand sendNibbleMsg() est utilise avec checksum, tient compte du ^ 0x55 final
 V1.2: (29/12/2023) Message tronque quand nibble mode utilise avec au moins une repetition et nombre de nibble impair
*/

#include "Arduino.h"
#include <Rcul.h>

#include <inttypes.h>
#include <Stream.h>

#define RC_TX_SERIAL_VERSION      1
#define RC_TX_SERIAL_REVISION     2

#define RC_TX_BYTE_OPT_TIME_MS(PpmPeriodUs, RepeatNb, MsgByteNb)        ( ( ( (2 * ((MsgByteNb) + 1)) + 1) * ((RepeatNb) + 1) ) * ( (PpmPeriodUs) / 1000) )
#define RC_TX_NIBBLE_OPT_TIME_MS(PpmPeriodUs, RepeatNb, MsgNibbleNb)    ( ( ( ( (MsgNibbleNb) + 2) + 1) * ((RepeatNb) + 1) ) * ( (PpmPeriodUs) / 1000) )

/* MODULE CONFIGURATION */
#define PPM_TX_SERIAL_USES_POWER_OF_2_AUTO_MALLOC /* Comment this if you set fifo size to a power of 2: this allows saving some bytes of program memory */

enum {RC_TX_BIN_BYTE = 0, RC_TX_BIN_NIBBLE};

enum {RC_TX_SERIAL_NO_REPEAT = 0, RC_TX_SERIAL_REPEAT0 = 0, RC_TX_SERIAL_REPEAT1, RC_TX_SERIAL_REPEAT2, RC_TX_SERIAL_REPEAT3, RC_TX_SERIAL_REPEAT4};
// Note: RC_TX_SERIAL_NO_REPEAT and RC_TX_SERIAL_REPEAT0 are equivalent

typedef struct {
    uint8_t
            TxMode:           1, /* RC_TX_BIN_BYTE, RC_TX_BIN_NIBBLE */
            TxInProgress:     1, /*  */
            TxCharInProgress: 1,
            TxFifoEmpty:      1,
            SweepTest:        1,
            SweepDec:         1;
    uint8_t TxNb;
    uint16_t
            NbToSend:         3,
            SentCnt:          3,
            CurIdx:           5, /* Prev Nibble to compare to the following one */
            PrevIdx:          5; /* Prev Nibble to compare to the following one */
}TxNibbleSt_t;

class RcTxSerial : public Stream
{
  private:
    // static data
    Rcul          *_Rcul; /* Each RcTxSerial may have its own destination Rcul */
    uint8_t        _Ch;
    uint8_t        _TxFifoSize;
    char          *_TxFifo;
    char           _TxChar;
    uint8_t        _TxFifoTail;
    uint8_t        _TxFifoHead;
    TxNibbleSt_t   _Nibble;
    uint8_t        _ChecksumForByteMsg;
    class          RcTxSerial *prev;
    static         RcTxSerial *last;
    uint8_t        TxFifoRead(char *TxChar);
  public:
    RcTxSerial(Rcul *Rcul, uint8_t RepeatNb, uint8_t TxFifoSize, uint8_t Ch = RCUL_NO_CH);
    void           reassignRculDst(Rcul *Rcul);
    void           setCh(uint8_t Ch);
    uint8_t        getCh(void);
    void           setTxMode(uint8_t TxMode);
    void           setRepeatNb(uint8_t RepeatNb);
    uint8_t        isReadyForTx(void);
    void           sendNibbleMsg(uint8_t *NibbleMsg, uint8_t NibbleNb, uint8_t AddChecksum = 1);
    void           addChecksumToByteMsg(void); /* If checksum needed, use it just after TxSerial.print() or TxSerial.write() */
    void           setSweepTest(uint8_t OffOn);
    uint8_t        getSweepTest(void);
    int peek();
    virtual size_t write(uint8_t byte);
    virtual int    read();
    virtual int    available();
    virtual void   flush();
    using Print::write;
    static uint8_t process(); /* Send half character synchronized with every CPPM/SBUS/SRXL/IBUS/SUMD frame */
};

#endif
