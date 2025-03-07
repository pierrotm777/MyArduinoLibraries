#include <RcTxMsx.h>

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

/* ROBBE - FUTABA */
#define SYNCHRO_PULSE_WIDTH_FUTABA       950
#define      ON_PULSE_WIDTH_FUTABA      1250
#define     OFF_PULSE_WIDTH_FUTABA      1750
#define     MID_PULSE_WIDTH_FUTABA         0 /* Not used */

/* MULTIPLEX */
/*
 .======+=========+======+======+======.
 |OutIdx|  Info   |Bottom|Middle| Top  |
 +======+=========+======+======+======+
 |   0  | Inter#1 |      |      |      |
 +------+---------+      |      |      |
 |   1  | Inter#2 |      |      |      |
 +------+---------+ 1700 | 1900 | 2100 |
 |   2  | Inter#3 |      |      |      |
 +------+---------+      |      |      |
 |   3  | Inter#4 |      |      |      |
 +======+=========+======+======+======+
 |   4  | Inter#5 |      |      |      |
 +------+---------+ 1100 | 1300 | 1500 |
 |   5  | Inter#6 |      |      |      |
 +------+---------+------+------+------+
 |   6  |  POT#1  |      |      |      |
 +------+---------+ 1100 | 1300 | 1500 |
 |   7  |  POT#2  |      |      |      |
 '======+=========+======+======+======'
 Note: Synchro if Pulse Width < 1500us and next Pulse Width > 1500us -> First channel (OutIdx = 0)
*/
#define SYNCHRO_PULSE_WIDTH_MULTIPLEX              0 /* Not used */
#define      ON_PULSE_WIDTH_MULTIPLEX           1500 /* + 600 if OutIdx < 4 */
#define     OFF_PULSE_WIDTH_MULTIPLEX           1100 /* + 600 if OutIdx < 4 */
#define     MID_PULSE_WIDTH_MULTIPLEX           1300 /* + 600 if OutIdx < 4 */
#define     HIGH_OFFSET_PULSE_WIDTH_MULTIPLEX    400
#define     GROUP_OFFSET_PULSE_WIDTH_MULTIPLEX   600

/* GRAUPNER */
#define SYNCHRO_PULSE_WIDTH_GRAUPNER            2100
#define      ON_PULSE_WIDTH_GRAUPNER            1900
#define     OFF_PULSE_WIDTH_GRAUPNER            1150
#define     MID_PULSE_WIDTH_GRAUPNER            1500

enum {PULSE_WIDTH_US_OFF = 0, PULSE_WIDTH_US_ON, PULSE_WIDTH_US_MID, PULSE_WIDTH_US_SYNCHRO, PULSE_WIDTH_US_NB};

typedef struct{
    uint16_t  Width_us[PULSE_WIDTH_US_NB];
}MsxPulseWidthSt_t;

typedef struct{
    uint8_t           PulseNb;
    uint8_t           EepPropWordNb;
    MsxPulseWidthSt_t Pulse;
}MsxSt_t;

const MsxSt_t MsxAttr[RC_TX_MSX_NB] PROGMEM = {
    /* PulseNb   EepPropWordNb         OFF(us)     ,            ON(us)       ,          MID(us)         ,        SYNCHRO(us)           */
    {1 + 16    , (0)    , {OFF_PULSE_WIDTH_FUTABA,    ON_PULSE_WIDTH_FUTABA,    MID_PULSE_WIDTH_FUTABA,    SYNCHRO_PULSE_WIDTH_FUTABA}},
    {1 + 12 + 2, (2 * 3), {OFF_PULSE_WIDTH_FUTABA,    ON_PULSE_WIDTH_FUTABA,    MID_PULSE_WIDTH_FUTABA,    SYNCHRO_PULSE_WIDTH_FUTABA}},
    {1 + 8     , (0)    , {OFF_PULSE_WIDTH_FUTABA,    ON_PULSE_WIDTH_FUTABA,    MID_PULSE_WIDTH_FUTABA,    SYNCHRO_PULSE_WIDTH_FUTABA}},
    {0 + 8     , (2 * 3), {OFF_PULSE_WIDTH_MULTIPLEX, ON_PULSE_WIDTH_MULTIPLEX, MID_PULSE_WIDTH_MULTIPLEX, SYNCHRO_PULSE_WIDTH_MULTIPLEX}},
    {2 + 8     , (0)    , {OFF_PULSE_WIDTH_GRAUPNER,  ON_PULSE_WIDTH_GRAUPNER,  MID_PULSE_WIDTH_GRAUPNER,  SYNCHRO_PULSE_WIDTH_GRAUPNER}},
                                  };
/*
EEPROM STRUCTURE (one per RcTxMsx object)
RC_TX_MSX_MS8_ROBBE_FUTABA_1513  -> Nothing used
RC_TX_MSX_MS16_ROBBE_8369        -> Nothing used
RC_TX_MSX_MS12PROP2_ROBBE_8370   [BB:Stop,BB:Fwd,BB:Rear][BB:Stop,BB:Fwd,BB:Rear] // Stop/Fwd/Rear
RC_TX_MSX_MULTINAUT_MPX_75882    [BB:Stop,BB:Fwd,BB:Rear][BB:Stop,BB:Fwd,BB:Rear] // Stop/Fwd/Rear
RC_TX_MSX_NAUTIC_GRP_4159        -> Nothing used
*/
#define GET_PULSE_NB(MsxIdx)               (uint8_t)pgm_read_byte(&MsxAttr[(MsxIdx)].PulseNb)
#define GET_PROP_WORD_NB_IN_EEP(MsxIdx)    (uint8_t)pgm_read_byte(&MsxAttr[(MsxIdx)].EepPropWordNb)
#define GET_PULSE_WIDTH(MsxIdx, PulseIdx)  (uint16_t)pgm_read_word(&MsxAttr[(MsxIdx)].Pulse.Width_us[(PulseIdx)])

/*************************************************************************
                           GLOBAL VARIABLES
*************************************************************************/
static uint16_t GlobEepBaseAddr = 0;
static uint8_t  GlobGroupNb     = 0;
#if (RC_TX_MSX_DBG > 0)
static uint8_t  ProtoDbg        = 0;
#endif

/*************************************************************************
                           PRIVATE FUNCTION PROTOTYPE
*************************************************************************/
static uint8_t getEepRecordSize(void);

/*************************************************************************
                           METHODS
*************************************************************************/
/* Constructor */
RcTxMsx::RcTxMsx(Rcul *TxRcul, uint8_t EepGrpIdx/* = 0*/, uint8_t Ch/* = 255*/)
{
    _TxRcul    = TxRcul;
    _EepGrpIdx = EepGrpIdx;
    _Ch        = Ch;
    _MsxMode   = RC_TX_MSX_NB; /* No Mode at startup */
}

void RcTxMsx::setMsxMode(uint8_t EepGrpIdx, uint8_t MsxMode, uint8_t *Data)
{
    _MsxMode   = MsxMode;
    _Data      = Data;
    _EepGrpIdx = EepGrpIdx;
    _InIdx     = 0;    
}

void RcTxMsx::setEepBaseAddrAndGroupNb(uint16_t EepBaseAddr, uint8_t  GroupNb)
{
    GlobEepBaseAddr = EepBaseAddr;
    GlobGroupNb     = GroupNb; /* To Compute the Total amount of eeporm used by RcTxMsx */
}

uint16_t RcTxMsx::getEepTotalSize(void)
{
    return(getEepRecordSize() * GlobGroupNb);
}

uint16_t RcTxMsx::getEepWord(uint8_t EepGrpIdx, uint8_t MsxIdx, uint8_t PropIdx, uint8_t WordIdx)
{
    uint8_t  EepRecordSize;
    uint16_t EepAddr, EepWord;
    
    EepRecordSize = getEepRecordSize();
    EepAddr       = GlobEepBaseAddr + (EepGrpIdx * EepRecordSize) + (MsxIdx * sizeof(MsxSt_t)) + (PropIdx * 3 * 2) + (WordIdx * 2);
    EepWord = (uint16_t)eeprom_read_word((const uint16_t *)(uint16_t)(EepAddr));
    
    return(EepWord);
}

void RcTxMsx::updateEepWord(uint8_t EepGrpIdx, uint8_t MsxIdx, uint8_t PropIdx, uint8_t WordIdx, uint16_t WordValue)
{
    uint8_t  EepRecordSize;
    uint16_t EepAddr;
    
    EepRecordSize = getEepRecordSize();
    EepAddr       = GlobEepBaseAddr + (EepGrpIdx * EepRecordSize) + (MsxIdx * sizeof(MsxSt_t)) + (PropIdx * 3 * 2) + (WordIdx * 2);
    eeprom_update_word((uint16_t *)(uint16_t)(EepAddr), WordValue);
}

void RcTxMsx::scheduleMsxFrame(void)
{
    uint16_t Width_us;
    
    switch(_MsxMode)
    {
        case RC_TX_MSX_MS8_ROBBE_FUTABA_1513: /* No break: normal */
        case RC_TX_MSX_MS16_ROBBE_8369:       /* No break: normal */
        case RC_TX_MSX_MS12PROP2_ROBBE_8370:
        Width_us = scheduleFutMsxFrame(_MsxMode);
        break;
        
        case RC_TX_MSX_MULTINAUT_MPX_75882:
        Width_us = scheduleMpxMsxFrame(_MsxMode);
        break;
        
        case RC_TX_MSX_NAUTIC_GRP_4159:
        Width_us = scheduleGrpMsxFrame(_MsxMode);
        break;
        
        default:
        Width_us = 1500;
        break;
    }
    _TxRcul->RculSetWidth_us(Width_us, _Ch);
}

//========================================================================================================================
// PRIVATE FUNCTIONS
//========================================================================================================================
uint16_t RcTxMsx::scheduleFutMsxFrame(uint8_t FutMode)
{
    uint16_t PulseWidthUs;
    uint8_t  MaxByteIdx = 0, Fwd, Rear, ValIdx;
    
    /*
    ByteIdx          [0]                      [1]
    BitIdx 15 14 13 12 11 10  9  8   7  6  5  4  3  2  1  0  <- Bit Order provided by X-Any
    MS8:  [        Not used       ][ 0, 1, 2, 3, 4, 5, 6, 7] <- For 1st MS8 (if 2 MS8 are declared)
    MS16: [ 0, 1, 2, 3, 4, 5, 6, 7][ 8, 9,10,11,12,13,14,15]
    */
    /* IMPORTANT: Robbe/Futaba sends the last bit first! */
    if(_InIdx) _InIdx--;
    else       _InIdx = GET_PULSE_NB(FutMode) - 1;
    if(_InIdx == (GET_PULSE_NB(FutMode) - 1))
    {
#if 0
    Serial.print(F("_D[0]="));Serial.print(_Data[0], HEX);
    if(FutMode != RC_TX_MSX_MS8_ROBBE_FUTABA_1513)
    {
        Serial.print(F("_D[1]="));Serial.print(_Data[1], HEX);
    }
    Serial.println();
#endif
        PulseWidthUs = GET_PULSE_WIDTH(FutMode, PULSE_WIDTH_US_SYNCHRO);
    }
    else
    {
        if(FutMode != RC_TX_MSX_MS8_ROBBE_FUTABA_1513)
        {
            /* Robbe/Futaba protocol needs 16 inputs -> 2 bytes (except for Futaba MS8) */
            MaxByteIdx = 1; /* MS protocol needs 2 bytes */
        }
        switch(FutMode)
        {
          case RC_TX_MSX_MS16_ROBBE_8369:
          case RC_TX_MSX_MS8_ROBBE_FUTABA_1513:
          PulseWidthUs = GET_PULSE_WIDTH(FutMode, bitRead(_Data[(_InIdx / 8) ^ MaxByteIdx], _InIdx % 8));
          break;
          
          case RC_TX_MSX_MS12PROP2_ROBBE_8370:
          if(_InIdx >= 12)
          {
            uint8_t BitIdx;
            BitIdx = (_InIdx * 2) - 12; /* First pass -> BitIdx = 12, second pass -> BitIdx = 14 */
            Rear   = bitRead(_Data[((BitIdx + 0) / 8) ^ MaxByteIdx], (BitIdx + 0) % 8);
            Fwd    = bitRead(_Data[((BitIdx + 1) / 8) ^ MaxByteIdx], (BitIdx + 1) % 8);
            ValIdx = (Fwd ^ Rear);
            if(ValIdx)
            {
              if(Rear) ValIdx++;
            }
            PulseWidthUs = getEepWord(_EepGrpIdx, FutMode, (BitIdx > 12), ValIdx); /* 0: Stop, 1: Fwd, 2: Rear */
          }
          else
          {
            /* For Bit 0 to Bit 11 */
            PulseWidthUs = GET_PULSE_WIDTH(FutMode, bitRead(_Data[(_InIdx / 8) ^ MaxByteIdx], _InIdx % 8));
          }
          break;
        }
    }
#if defined(__AVR_ATmega328P__) && (RC_TX_MSX_DBG_FUT == 1)
    if(ProtoDbg & ((1 << RC_TX_MSX_MS16_ROBBE_8369) | (1 << RC_TX_MSX_MS12PROP2_ROBBE_8370) | (1 << RC_TX_MSX_MS8_ROBBE_FUTABA_1513)))
    {
        uint8_t Ms = millis() % 100;
        if(Ms < 10) Serial.print(F("0"));
        Serial.print(Ms);Serial.print(F("R/F["));
        if(_InIdx < 10) Serial.print(F("0"));
        Serial.print(_InIdx);Serial.print(F("]="));Serial.println(PulseWidthUs);
    }
#endif
    return(PulseWidthUs);
}

uint16_t RcTxMsx::scheduleMpxMsxFrame(uint8_t MpxMode)
{
    uint8_t  OutIdx = _InIdx / 2, ValTop, ValBottom, ValIdx;
    uint16_t PulseWidthUs;
    
    /*
    ByteIdx          [0]                      [1]
    BitIdx 15 14 13 12 11 10  9  8   7  6  5  4  3  2  1  0  <- Bit Order provided by X-Any
    */
    /* Multiplex protocol needs 16 inputs -> 2 bytes */
    ValBottom = bitRead(_Data[((_InIdx + 0) / 8) ^ 1], ((_InIdx + 0) % 8));
    ValTop    = bitRead(_Data[((_InIdx + 1) / 8) ^ 1], ((_InIdx + 1) % 8));
    
    ValIdx = (ValBottom ^ ValTop);
    if(ValIdx)
    {
        if(ValBottom) ValIdx = 0;
        else          ValIdx = 1;
    }
    else ValIdx = 2;
    PulseWidthUs = GET_PULSE_WIDTH(MpxMode, ValIdx); /* ValIdx: 0 -> ValBottom, 1 -> ValTop, 2 -> Middle */
    if(OutIdx < 4)
    {
        PulseWidthUs += GROUP_OFFSET_PULSE_WIDTH_MULTIPLEX;
    }
    if(OutIdx >= 6)
    {
        /* 2 x Pseudo-Prop Channels: change index from 0 1 2 to 2 1 0 */
        if(!(ValIdx & 1))
        {
            if(!ValIdx) ValIdx = 2;
            else        ValIdx = 0;
        }
        PulseWidthUs = getEepWord(_EepGrpIdx, MpxMode, (OutIdx > 6), ValIdx); /* 0: Stop, 1: Fwd, 2: Rear */
    }
#if defined(__AVR_ATmega328P__) && (RC_TX_MSX_DBG_MPX == 1)
    if(ProtoDbg & (1 << RC_TX_MSX_MULTINAUT_MPX_75882))
    {
        uint8_t Ms = millis() % 100;
        if(Ms < 10) Serial.print(F("0"));
        Serial.print(Ms);Serial.print(F("M["));
        if(OutIdx < 10) Serial.print(F("0"));
        Serial.print(OutIdx);Serial.print(F("]="));Serial.println(PulseWidthUs);
    }
#endif
    _InIdx += 2;
    OutIdx ++;
    if(OutIdx >= GET_PULSE_NB(MpxMode)) _InIdx = 0;
    
    return(PulseWidthUs);
}

uint16_t RcTxMsx::scheduleGrpMsxFrame(uint8_t GrpMode)
{
    uint16_t PulseWidthUs;
    uint8_t  BitIdx, ValTop, ValBottom, ValIdx;
    
    /*
    ByteIdx          [0]                      [1]
    BitIdx 15 14 13 12 11 10  9  8   7  6  5  4  3  2  1  0  <- Bit Order provided by X-Any
    */
    /* Graupner protocol needs 16 inputs -> 2 bytes */
    if(_InIdx < 2)
    {
        /* 2 synchro pulses */
        PulseWidthUs = GET_PULSE_WIDTH(GrpMode, PULSE_WIDTH_US_SYNCHRO);
    }
    else
    {
        BitIdx    = (_InIdx * 2) - 4;
        ValTop    = bitRead(_Data[((BitIdx + 0) / 8) ^ 1], ((BitIdx + 0) % 8));
        ValBottom = bitRead(_Data[((BitIdx + 1) / 8) ^ 1], ((BitIdx + 1) % 8));
        
        if(ValTop ^ ValBottom)
        {
            ValIdx = ValBottom;
        }
        else
        {
            ValIdx = 2;
        }
        PulseWidthUs = GET_PULSE_WIDTH(GrpMode, ValIdx);
    }
#if defined(__AVR_ATmega328P__) && (RC_TX_MSX_DBG_GRP == 1)
    uint8_t DispIdx;
    if(ProtoDbg & (1 << RC_TX_MSX_NAUTIC_GRP_4159))
    {
        uint8_t Ms = millis() % 100;
        if(Ms < 10) Serial.print(F("0"));
        Serial.print(Ms);Serial.print(F("G["));
        DispIdx = (_InIdx < 2)? 8 + _InIdx: _InIdx - 2;
        if(DispIdx < 10) Serial.print(F("0"));
        Serial.print(DispIdx);Serial.print(F("]="));Serial.println(PulseWidthUs);
    }
#endif
    _InIdx ++;
    if(_InIdx >= GET_PULSE_NB(GrpMode)) _InIdx = 0;
    
    return(PulseWidthUs);
}

#if (RC_TX_MSX_DBG > 0)
void RcTxMsx::debugProtocol(uint8_t GlobalOffOn)
{
    ProtoDbg =  GlobalOffOn;
}
#endif

static uint8_t getEepRecordSize(void)
{
    uint8_t EepRecordSize = 0;
    
    for(uint8_t Idx = 0; Idx < RC_TX_MSX_NB; Idx++)
    {
        EepRecordSize += (2 * GET_PROP_WORD_NB_IN_EEP(Idx));
    }
    return(EepRecordSize);
}
