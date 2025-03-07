#include "Arduino.h"
#include <RcRxSerial.h>
#if (defined(__AVR__))
#include <avr\pgmspace.h>
#else
#include <pgmspace.h>// ajout pour ESP32
#endif
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

/*
NIBBLE_WIDTH_US
  <--->
 996                                                                     2004
  |-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|-+-|
    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F   R   I
    <--->
    |   |                                                               |
  1024 1080                                                           1976
INTER_NIBBLE
*/
enum {NIBBLE_0=0, NIBBLE_1, NIBBLE_2, NIBBLE_3, NIBBLE_4, NIBBLE_5, NIBBLE_6, NIBBLE_7, NIBBLE_8, NIBBLE_9, NIBBLE_A, NIBBLE_B, NIBBLE_C, NIBBLE_D, NIBBLE_E, NIBBLE_F, NIBBLE_R, NIBBLE_I, NIBBLE_NB};

#define NEUTRAL_WIDTH_US          1500
#define NIBBLE_WIDTH_US           56
#define FULL_EXCURSION_US         (NIBBLE_WIDTH_US * NIBBLE_NB)
#define PULSE_MIN_US              (NEUTRAL_WIDTH_US - (FULL_EXCURSION_US / 2))
#define PULSE_MAX_US              (NEUTRAL_WIDTH_US + (FULL_EXCURSION_US / 2))

#define PULSE_MIN(NibbleIdx)      (PULSE_MIN_US + ((NibbleIdx) * NIBBLE_WIDTH_US))
#define PULSE_MAX(NibbleIdx)      (PULSE_MIN((NibbleIdx)) + NIBBLE_WIDTH_US)
#define PULSE_MIN_MAX(NibbleIdx)  {PULSE_MIN(NibbleIdx), PULSE_MAX(NibbleIdx)}

typedef struct {
  boolean Available;
  boolean Nibble;
  char    Char;
}RcMsgRxSt_t;

typedef struct {
  uint16_t Min;
  uint16_t Max;
}NibblePulseSt_t;

const NibblePulseSt_t NibblePulse[] PROGMEM = {
							  PULSE_MIN_MAX(NIBBLE_0),
							  PULSE_MIN_MAX(NIBBLE_1),
							  PULSE_MIN_MAX(NIBBLE_2),
							  PULSE_MIN_MAX(NIBBLE_3),
							  PULSE_MIN_MAX(NIBBLE_4),
							  PULSE_MIN_MAX(NIBBLE_5),
							  PULSE_MIN_MAX(NIBBLE_6),
							  PULSE_MIN_MAX(NIBBLE_7),
							  PULSE_MIN_MAX(NIBBLE_8),
							  PULSE_MIN_MAX(NIBBLE_9),
							  PULSE_MIN_MAX(NIBBLE_A),
							  PULSE_MIN_MAX(NIBBLE_B),
							  PULSE_MIN_MAX(NIBBLE_C),
							  PULSE_MIN_MAX(NIBBLE_D),
							  PULSE_MIN_MAX(NIBBLE_E),
							  PULSE_MIN_MAX(NIBBLE_F),
							  PULSE_MIN_MAX(NIBBLE_R),
							  PULSE_MIN_MAX(NIBBLE_I)
							  };

#define TABLE_ITEM_NBR(Tbl)  (sizeof(Tbl) / sizeof(Tbl[0]))
                                  
static int8_t PulseWithToNibbleIdx(uint16_t PulseWidth);

/*************************************************************************
                              GLOBAL VARIABLES
*************************************************************************/
/* Constructor */
RcRxSerial::RcRxSerial(Rcul *Rcul, uint8_t Filter, uint8_t Ch /*= RCUL_NO_CH*/, uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  _Nibble.Filter = Filter;
  reassignRculSrc(Rcul, Ch, ClientIdx);
}

void RcRxSerial::reassignRculSrc(Rcul *Rcul, uint8_t Ch /*= RCUL_NO_CH*/, uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  _Rcul = Rcul;
  _rcSrc.Ch = Ch;
  _rcSrc.ClientIdx = ClientIdx;
  _available = 0;
  _Nibble.Phase = 0;
  _Nibble.MsnPending = 0;
  _Nibble.PrevIdx = NIBBLE_NB;
  _Nibble.PrevValid = NIBBLE_I;
  _Nibble.Available = 0;
  _Char = 0;
  _MsgLen = 0;
}

void RcRxSerial::setFilter(uint8_t Filter)
{
  _Nibble.Filter = Filter;
}

#define RX_SERIAL_IDLE_NIBBLE_AVAILABLE  (1 << 0)
#define RX_SERIAL_CHAR_AVAILABLE         (1 << 1)

uint8_t RcRxSerial::somethingAvailable(void)
{
  uint8_t Ret = 0;
  int8_t NibbleIdx; /* Shall be signed */

  if(_Rcul) /* In case _Rcul is NULL */
  {
    if(_Rcul->RculIsSynchro(_rcSrc.ClientIdx))
    {
      _LastWidth_us = _Rcul->RculGetWidth_us(_rcSrc.Ch);
      _Nibble.Available = 1;
      NibbleIdx = PulseWithToNibbleIdx(_LastWidth_us);
      if(NibbleIdx >= 0)
      {
        if(NibbleIdx != _Nibble.PrevIdx)
        {
          _Nibble.PrevIdx = NibbleIdx;
          _Nibble.SameCnt = 0;
        }
        _Nibble.SameCnt++;
        if(_Nibble.SameCnt == (_Nibble.Filter + 1)) /* == and not >= to avoid repeating the redundant character */
        {
          /* OK, Nibble valid */
          if(NibbleIdx == NIBBLE_R)
          {
            _Nibble.Itself = _Nibble.PrevValid; /* Previous nibble is repeated */
          }
          else
          {
            _Nibble.Itself = NibbleIdx;
          }
          _Nibble.PrevValid = _Nibble.Itself;
          if(_Nibble.Itself == NIBBLE_I)
          {
            Ret = RX_SERIAL_IDLE_NIBBLE_AVAILABLE;
            _Nibble.Phase = 0; /* Idle -> Re-Synch */
          }
          else
          {
            if(!_Nibble.Phase)
            {
              _Char = (_Nibble.Itself << 4);  /* MSN first */
              _Nibble.MsnPending = 1;
            }
            else
            {
              _Char |= _Nibble.Itself;  /* LSN */
              _Nibble.MsnPending = 0;
              Ret = RX_SERIAL_CHAR_AVAILABLE;
              _available = 1;
            }
            _Nibble.Phase = !_Nibble.Phase;
          }
        }
      }
    }
  }
  return(Ret);
}

uint8_t RcRxSerial::available()
{
  somethingAvailable();
  return(_available);
}

uint8_t RcRxSerial::msgAvailable(char *RxBuf, uint8_t RxBufMaxLen)
{
  uint8_t Ret = 0;

  switch(somethingAvailable())
  {
    case RX_SERIAL_IDLE_NIBBLE_AVAILABLE:
    if(_Nibble.MsnPending)
    {
        if(_MsgLen < RxBufMaxLen)
        {
            RxBuf[_MsgLen] = _Char; /* Store the_Char containing the Most Significant Nibble */
            Ret = RC_RX_SERIAL_PENDING_NIBBLE_INDICATOR | _MsgLen; /* End of Message: Most Significant Nibble indicator armed */
        }
        _MsgLen = 0;   /* Re-init */
        _Nibble.MsnPending = 0;
    }
    else
    {
        if(_MsgLen)
        {
            Ret = _MsgLen; /* End of Message */
            _MsgLen = 0;   /* Re-init */
        }
    }
    break;
    
    case RX_SERIAL_CHAR_AVAILABLE:
    if(_MsgLen < RxBufMaxLen)
    {
      RxBuf[_MsgLen++] = _Char;
    }
    else
    {
      Ret = 0; /* Message too long! */
      _MsgLen = 0;
    }
    break;
  }
  return(Ret);
}

uint8_t RcRxSerial::read()
{
   _available = 0;
   return(_Char);
}

uint8_t RcRxSerial::nibbleAvailable() /* Only for calibration purpose */
{
  return(_Nibble.Available);
}

uint16_t RcRxSerial::lastWidth_us() /* Only for calibration purpose */
{
  _Nibble.Available = 0;
  return(_LastWidth_us);
}

uint8_t RcRxSerial::msgChecksumIsValid(char *Msg, uint8_t MsgAttr)
{
  uint8_t RxMsgByteLen, RxMsgNibbleLen, Idx, RxMsgChecksum, ComputedChecksum = 0;

  RxMsgByteLen   = (MsgAttr & ~RC_RX_SERIAL_PENDING_NIBBLE_INDICATOR);
  RxMsgNibbleLen = (RxMsgByteLen * 2);
  if(MsgAttr & RC_RX_SERIAL_PENDING_NIBBLE_INDICATOR)
  {
    RxMsgNibbleLen ++;
  }
#if 0
Serial.print(F("RxMsgByteLen="));Serial.print(RxMsgByteLen);Serial.print(F(" RxMsgNibbleLen="));Serial.println(RxMsgNibbleLen);
for(uint8_t Idx = 0; Idx < ((RxMsgNibbleLen + 1) /  2); Idx++)
{
  Serial.println((uint8_t)Msg[Idx], HEX);
}
#endif
  /* Extract received checksum */
  if(RxMsgNibbleLen & 1)
  {
    RxMsgChecksum = ( ((Msg[RxMsgByteLen - 1] & 0x0F) << 4) | ((Msg[RxMsgByteLen] & 0xF0) >> 4) );
  }
  else
  {
    RxMsgChecksum = Msg[RxMsgByteLen - 1];
  }
  /* Compute checksum */
  for(Idx = 0; Idx < (RxMsgByteLen - 1); Idx++)
  {
    ComputedChecksum ^= Msg[Idx];
  }
  if(RxMsgNibbleLen & 1)
  {
    ComputedChecksum ^= (Msg[Idx] & 0xF0);
  }
  ComputedChecksum ^= 0x55;
  
  return(RxMsgChecksum == ComputedChecksum);
}

//========================================================================================================================
// PRIVATE FUNCTIONS
//========================================================================================================================
static int8_t PulseWithToNibbleIdx(uint16_t PulseWidth)
{
  int8_t  Ret = -1;
  uint8_t Idx;

  for(Idx = 0; Idx < TABLE_ITEM_NBR(NibblePulse); Idx++)
  {
    if( (PulseWidth >= pgm_read_word(&NibblePulse[Idx].Min)) && (PulseWidth <= pgm_read_word(&NibblePulse[Idx].Max)) )
    {
      Ret = Idx;
      break;
    }
  }
  return(Ret);
}

