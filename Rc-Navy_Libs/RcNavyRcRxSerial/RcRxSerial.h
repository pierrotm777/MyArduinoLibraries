#ifndef RcRxSerial_h
#define RcRxSerial_h

/*
 English: by RC Navy (2012)
 =======
 <RcRxSerial>: a library to build an unidirectionnal serial port through RC Transmitter/Receiver.
 http://p.loussouarn.free.fr
 V1.0: (07/11/2022) initial release
 
 Francais: par RC Navy (2012)
 ========
 <RcRxSerial>: une bibliotheque pour construire un port serie unidirectionnel a travers un Emetteur/Recepteur RC.
 http://p.loussouarn.free.fr
 V1.0: (07/11/2022) release initiale
*/

#include "Arduino.h"
#include <Rcul.h>

#define RC_RX_SERIAL_VERSION      1
#define RC_RX_SERIAL_REVISION     0

enum {RC_RX_SERIAL_NO_FILTER = 0, RC_RX_SERIAL_FILTER0 = 0, RC_RX_SERIAL_FILTER1, RC_RX_SERIAL_FILTER2, RC_RX_SERIAL_FILTER3};
 //Note: and RC_RX_SERIAL_FILTER0 are equivalent RC_RX_SERIAL_NO_FILTER
#define RC_RX_SERIAL_PENDING_NIBBLE_INDICATOR  (1 << 7)

typedef struct {
    uint16_t
            Filter:     2, /* If No Filter (Filter = 0) 1 occurence of nibble validates the nibble, otherwise (1 + Filter) occurences are expected */
            MsnPending: 1,
            Phase:      1,
            Available:  1,
            PrevValid:  5,
            Itself:     5;
    uint8_t 
            PrevIdx:    5, /* Prev Nibble to compare to the following one */
            SameCnt:    3; /* Current valid Nibble */
}RxNibbleSt_t;

typedef struct {
    uint8_t 
            Ch:         5, /* Default = 31 (Max) */
            ClientIdx:  3; /* 0 to 6 */
}RcSrcSt_t;

class RcRxSerial
{
  private:
    RcSrcSt_t      _rcSrc;
    Rcul           *_Rcul;
    uint8_t        _Ch;
    char           _Char;
    uint8_t        _MsgLen;
    uint16_t       _LastWidth_us;
    uint8_t        _available;
    RxNibbleSt_t   _Nibble;
    uint8_t        somethingAvailable(void);
  public:
    RcRxSerial(Rcul *Rcul, uint8_t Filter = RC_RX_SERIAL_NO_FILTER, uint8_t Ch = RCUL_NO_CH, uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    void           reassignRculSrc(Rcul *Rcul, uint8_t Ch = RCUL_NO_CH, uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX); /* Marginal use (do not use it, if you do not know what is it for) */
    void           setFilter(uint8_t Filter);
    uint8_t        available();
    uint8_t        read();
    uint8_t        msgAvailable(char *RxBuf,   uint8_t RxBufMaxLen);
    static uint8_t msgChecksumIsValid(char *Msg, uint8_t MsgAttr);
    uint16_t       lastWidth_us();     /* Only for calibration purpose */
    uint8_t        nibbleAvailable();  /* Only for calibration purpose */
};

#endif

