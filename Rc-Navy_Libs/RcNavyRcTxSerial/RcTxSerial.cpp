#include <RcTxSerial.h>

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

#define NEUTRAL_WIDTH_US            1500
#define NIBBLE_WIDTH_US             56
#define FULL_EXCURSION_US           (NIBBLE_WIDTH_US * NIBBLE_NB)
#define PULSE_MIN_US                (NEUTRAL_WIDTH_US - (FULL_EXCURSION_US / 2))
#define PULSE_WIDTH_US(NibbleIdx)   (PULSE_MIN_US + (NIBBLE_WIDTH_US / 2)+ ((NibbleIdx) * NIBBLE_WIDTH_US))

#define GET_PULSE_WIDTH_US(NibbleIdx) (uint16_t)pgm_read_word(&PulseWidth[(NibbleIdx)])

const uint16_t PulseWidth[] PROGMEM = {PULSE_WIDTH_US(NIBBLE_0), PULSE_WIDTH_US(NIBBLE_1), PULSE_WIDTH_US(NIBBLE_2), PULSE_WIDTH_US(NIBBLE_3),
                                       PULSE_WIDTH_US(NIBBLE_4), PULSE_WIDTH_US(NIBBLE_5), PULSE_WIDTH_US(NIBBLE_6), PULSE_WIDTH_US(NIBBLE_7),
                                       PULSE_WIDTH_US(NIBBLE_8), PULSE_WIDTH_US(NIBBLE_9), PULSE_WIDTH_US(NIBBLE_A), PULSE_WIDTH_US(NIBBLE_B),
                                       PULSE_WIDTH_US(NIBBLE_C), PULSE_WIDTH_US(NIBBLE_D), PULSE_WIDTH_US(NIBBLE_E), PULSE_WIDTH_US(NIBBLE_F),
                                       PULSE_WIDTH_US(NIBBLE_R), PULSE_WIDTH_US(NIBBLE_I)};

#define PULSE_WIDTH_IDX_MAX           17

RcTxSerial    *RcTxSerial::last = NULL;

/*************************************************************************
                           GLOBAL VARIABLES
*************************************************************************/

/* Constructor */
RcTxSerial::RcTxSerial(Rcul *Rcul, uint8_t RepeatNb, uint8_t TxFifoSize, uint8_t Ch /* = RCUL_NO_CH */)
{
  _Rcul      = NULL;
#ifdef PPM_TX_SERIAL_USES_POWER_OF_2_AUTO_MALLOC
  if(TxFifoSize > 128) TxFifoSize = 128; /* Must fit in a 8 bits  */
  _TxFifoSize = 1;
  do
  {
    _TxFifoSize <<= 1;
  }while(_TxFifoSize < TxFifoSize); /* Search for the _TxFifoSize in power of 2 just greater or equal to requested size */
#else
  _TxFifoSize = TxFifoSize;
#endif
  _TxFifo = (char *)malloc(_TxFifoSize);
  if(_TxFifo != NULL)
  {
    _Rcul                    = Rcul;
    _Nibble.TxMode           = RC_TX_BIN_BYTE; /* By default */
    _Nibble.TxNb             = 0;
    _Nibble.TxInProgress     = 0;
    _Nibble.TxCharInProgress = 0;
    _Nibble.TxFifoEmpty      = 1;
    _Nibble.NbToSend         = RepeatNb + 1;
    _Nibble.SentCnt          = 0;
    _Nibble.SweepTest        = 0;
    _Nibble.SweepDec         = 0;
    _Ch                      = Ch;
    _TxFifoTail              = 0;
    _TxFifoHead              = 0;
    _ChecksumForByteMsg      = 0;
    prev                     = last;
    last                     = this;
  }
}

void RcTxSerial::reassignRculDst(Rcul *Rcul)
{
  _Rcul      = Rcul;
}

void RcTxSerial::setCh(uint8_t Ch)
{
  _Ch = Ch;
}

uint8_t RcTxSerial::getCh(void)
{
  return(_Ch);
}

void RcTxSerial::setTxMode(uint8_t TxMode)
{
  _Nibble.TxMode = TxMode;
}

void RcTxSerial::setRepeatNb(uint8_t RepeatNb)
{
  _Nibble.NbToSend = RepeatNb + 1;
}

size_t RcTxSerial::write(uint8_t b)
{
  size_t Ret = 0;

  // if buffer full, discard the character and return
  if ((_TxFifoTail + 1) % _TxFifoSize != _TxFifoHead)
  {
    // save new data in buffer: tail points to where byte goes
    _TxFifo[_TxFifoTail] = b; // save new byte
    _TxFifoTail = (_TxFifoTail + 1) % _TxFifoSize;
    _ChecksumForByteMsg ^= b;
    _Nibble.TxFifoEmpty = 0;
    Ret = 1;
  }
  return(Ret);
}

int RcTxSerial::read()
{
  return -1;
}

int RcTxSerial::available()
{
  return 0;
}

void RcTxSerial::flush()
{
  _TxFifoHead = _TxFifoTail = 0;
}

int RcTxSerial::peek()
{
  // Empty buffer?
  if (_TxFifoHead == _TxFifoTail)
    return -1;

  // Read from "head"
  return _TxFifo[_TxFifoHead];
}

uint8_t RcTxSerial::isReadyForTx(void)
{
  return(_Nibble.TxFifoEmpty);
}

#define _NblNb  _TxFifoHead
void RcTxSerial::sendNibbleMsg(uint8_t *NibbleMsg, uint8_t NibbleNb, uint8_t AddChecksum /*= 1*/)
{
  uint8_t ByteNb, Checksum = 0;

  if(NibbleNb)
  {
    _Nibble.TxMode = RC_TX_BIN_NIBBLE; /* Switch to Nibble Mode if not already done */
    ByteNb = (NibbleNb + 1) / 2;
    if( (ByteNb + !!AddChecksum) > _TxFifoSize) ByteNb = _TxFifoSize - !!AddChecksum; /* -1 to keep room for checksum */
    memcpy(_TxFifo, NibbleMsg, ByteNb);
    if(AddChecksum)
    {
      if(NibbleNb & 1) _TxFifo[ByteNb - 1] &= 0xF0; /* Ensure last nibble is 0 (for Checksum) */
      for(uint8_t ByteIdx = 0; ByteIdx < ByteNb; ByteIdx++)
      {
        Checksum ^= _TxFifo[ByteIdx];
      }
      if(NibbleNb & 1)
      {
        _TxFifo[ByteNb - 1] |= ((Checksum & 0xF0) >> 4) ^ 0x05; /* Most  Significant Nibble of Checksum */
        _TxFifo[ByteNb]      = ((Checksum & 0x0F) << 4) ^ 0x50; /* Least Significant Nibble of Checksum */
      }
      else
      {
        _TxFifo[ByteNb] = Checksum ^ 0x55;
      }
    }
    _NblNb = NibbleNb + (!!AddChecksum * 2);
    _Nibble.TxNb = 0;
    _Nibble.TxFifoEmpty = 0;
  }
}

void RcTxSerial::addChecksumToByteMsg(void)
{
  write(_ChecksumForByteMsg ^ 0x55);
}

uint8_t RcTxSerial::process()
{
  uint8_t    Ret = 0, Idx = 0;
  RcTxSerial *t;
  
  for(t = last; t != 0; t = t->prev)
  {
    if(t->_Rcul)
    {
      if(t->_Rcul->RculIsSynchro(Idx++)) /* Each instance of RcTxSerial SHALL use a different Index for Synchro test */
      {
        if(!t->_Nibble.TxInProgress)
        {
          /* Prepare next nibble to send */
          t->_Nibble.TxInProgress = 1;
          if(t->_Nibble.SweepTest)
          {
            if(t->_Nibble.SweepDec)
            {
              if(!t->_Nibble.CurIdx)
              {
                t->_Nibble.SweepDec = 0;
                t->_Nibble.CurIdx = 1;
              }
              else t->_Nibble.CurIdx--;
            }
            else
            {
              t->_Nibble.CurIdx++;
              if(t->_Nibble.CurIdx > PULSE_WIDTH_IDX_MAX)
              {
                t->_Nibble.SweepDec = 1;
                t->_Nibble.CurIdx = (PULSE_WIDTH_IDX_MAX - 1);
              }
            }
          }
          else
          {
            if(!t->_Nibble.TxCharInProgress)
            {
              /* Get next char to send */
              if(t->_Nibble.TxMode == RC_TX_BIN_NIBBLE)
              {
                /* RC_TX_BIN_NIBBLE Mode */
                if(t->_Nibble.TxNb < t->_NblNb)
                {
                  t->_TxChar = t->_TxFifo[t->_Nibble.TxNb / 2];
                  t->_Nibble.CurIdx = ((t->_TxChar & 0xF0) >> 4); /* MSN first */
                  if(((t->_Nibble.TxNb + 1) != t->_NblNb) || !(t->_NblNb & 1))
                  {
                    /* Not the last Nibble OR Odd */
                    t->_Nibble.TxCharInProgress = 1;
                  }
                  else
                  {
                    /* Last Nibble AND Even */
                  }
                }
                else
                {
                  t->_Nibble.CurIdx = NIBBLE_I; /* Nothing to transmit */
                  t->_Nibble.TxFifoEmpty = 1;
                }
              }
              else
              {
                /* RC_TX_BIN_BYTE Mode */
                if(t->TxFifoRead(&t->_TxChar))
                {
                  t->_Nibble.CurIdx = ((t->_TxChar & 0xF0) >> 4); /* MSN first */
                  t->_Nibble.TxCharInProgress = 1;
                }
                else
                {
                  t->_Nibble.CurIdx = NIBBLE_I; /* Nothing to transmit */
                  t->_Nibble.TxFifoEmpty  = 1;
                  t->_ChecksumForByteMsg = 0;
                }
              }
            }
            else
            {
              /* Tx Char in progress: send least significant nibble */
              t->_Nibble.CurIdx = t->_TxChar & 0x0F; /* LSN */
              t->_Nibble.TxCharInProgress = 0;
            }
            if(t->_Nibble.CurIdx == t->_Nibble.PrevIdx) t->_Nibble.CurIdx = NIBBLE_R; /* Repeat symbol */
            t->_Nibble.PrevIdx = t->_Nibble.CurIdx;
          }
        }
        /* Send the Nibble or the Repeat or the Idle symbol */
        t->_Rcul->RculSetWidth_us(GET_PULSE_WIDTH_US(t->_Nibble.CurIdx), t->_Ch); /* /!\ Ch as last argument /!\ */
        t->_Nibble.SentCnt++;
        if(t->_Nibble.SentCnt >= t->_Nibble.NbToSend)
        {
          t->_Nibble.SentCnt = 0;
          t->_Nibble.TxInProgress = 0;
          if(t->_Nibble.TxMode == RC_TX_BIN_NIBBLE)
          {
            if(t->_Nibble.TxNb < (t->_TxFifoSize * 2)) t->_Nibble.TxNb ++;
            if(t->_Nibble.CurIdx == NIBBLE_I) t->_Nibble.TxNb = 0; /* Otherwise, pb when repetition <> of 0 */
          }
        }
      }
      Ret = 1;
    }
  }
  return(Ret);
}

void RcTxSerial::setSweepTest(uint8_t OffOn)
{
  _Nibble.SweepTest = OffOn;
}

uint8_t RcTxSerial::getSweepTest(void)
{
  return(_Nibble.SweepTest);
}

//========================================================================================================================
// PRIVATE FUNCTIONS
//========================================================================================================================
uint8_t RcTxSerial::TxFifoRead(char *TxChar)
{
uint8_t Ret = 0;
  // Empty buffer?
  if (_TxFifoHead != _TxFifoTail)
  {
    // Read from "head"
    *TxChar = _TxFifo[_TxFifoHead]; // grab next byte
    _TxFifoHead = (_TxFifoHead + 1) % _TxFifoSize;
    Ret=1;
  }
  return(Ret);
}
