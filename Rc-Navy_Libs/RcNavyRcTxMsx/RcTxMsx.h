#ifndef RcTxMsx_h
#define RcTxMsx_h

/* Libray Version/Revision */
#define RC_TX_MSX_VER    1
#define RC_TX_MSX_REV    0

/*
 English: by RC Navy (2018)
 =======
 <RcTxMsx>: a library to send pulse frame for the following Multi-Switch decoders:
 - MS8       Futaba       (Ref:  1513) (8  outputs)
 - MS16      Robbe-Futaba (Ref:  8369) (16 outputs)
 - MS12+2    Robbe-Futaba (Ref:  8370) (14 outputs: 12 + 2 Prop  Pos0/Stop/Pos1)
 - Multinaut Multiplex    (Ref: 75882) (14 outputs: 12 + 2 Eng   Rear/Stop/Fwd)
 - Nautic    Graupner     (Ref:  4159) (16 outputs)
 http://p.loussouarn.free.fr

 Francais: par RC Navy (2018)
 ========
 <RcTxMsx>: une bibliotheque pour emettre un train d'impulsion pour les decodeurs Multi-Switches suivants:
 - MS8       Futaba       (Ref:  1513) (8  sorties)
 - MS16      Robbe-Futaba (Ref:  8369) (16 sorties)
 - MS12+2    Robbe-Futaba (Ref:  8370) (14 sorties: 12 + 2 Prop  Pos0/Stop/Pos1)
 - Multinaut Multiplex    (Ref: 75882) (14 sorties: 12 + 2 Mot     Ar/Stop/Av)
 - Nautic    Graupner     (Ref:  4159) (16 sorties)
 http://p.loussouarn.free.fr
*/

#include "Arduino.h"
#include <Rcul.h>

#include <inttypes.h>
#include <avr/eeprom.h>

/* RcTxMsx Debug Management (at compilation time) */
#define RC_TX_MSX_DBG_FUT   1 /* Set this to 0 to cancel debug messages for Robbe/Futaba */
#define RC_TX_MSX_DBG_GRP   0 /* Set this to 0 to cancel debug messages for Graupner     */
#define RC_TX_MSX_DBG_MPX   0 /* Set this to 0 to cancel debug messages for Multiplex    */

#define RC_TX_MSX_DBG       (RC_TX_MSX_DBG_FUT + RC_TX_MSX_DBG_GRP + RC_TX_MSX_DBG_MPX)

#if (RC_TX_MSX_DBG > 0)
#warning DEBUG enabled in RcTxMsx.h !
#endif

enum {RC_TX_MSX_MS16_ROBBE_8369 = 0, RC_TX_MSX_MS12PROP2_ROBBE_8370, RC_TX_MSX_MS8_ROBBE_FUTABA_1513, RC_TX_MSX_MULTINAUT_MPX_75882, RC_TX_MSX_NAUTIC_GRP_4159, RC_TX_MSX_NB};

class RcTxMsx
{
  private:
    // static data
    Rcul            *_TxRcul;
    uint8_t         *_Data;
    uint8_t         _InIdx;
    uint8_t         _MsxMode;
    uint8_t         _MsxState;
    uint8_t         _EepGrpIdx;
    uint8_t         _Ch;
    uint16_t        scheduleFutMsxFrame(uint8_t FutMode);
    uint16_t        scheduleMpxMsxFrame(uint8_t MpxMode);
    uint16_t        scheduleGrpMsxFrame(uint8_t GrpMode);
  public:
    RcTxMsx(Rcul *TxRcul, uint8_t EepGrpIdx = 0, uint8_t Ch = 255);
    void            setMsxMode(uint8_t EepGrpIdx, uint8_t MsxMode, uint8_t *Data);
    void            scheduleMsxFrame(void);
    static void     setEepBaseAddrAndGroupNb(uint16_t EepBaseAddr, uint8_t  GroupNb);
    static uint16_t getEepTotalSize(void);
    static uint16_t getEepWord(uint8_t GoupIdx, uint8_t MsxIdx, uint8_t PropIdx, uint8_t WordIdx);
    static void     updateEepWord(uint8_t GoupIdx, uint8_t MsxIdx, uint8_t PropIdx, uint8_t WordIdx, uint16_t WordValue);
#if (RC_TX_MSX_DBG > 0)
    static void     debugProtocol(uint8_t OffOn);
#endif
};

#endif
