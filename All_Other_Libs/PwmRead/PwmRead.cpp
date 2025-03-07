
#include "PwmRead.h"


PwmRead *PwmRead::last = NULL;

PwmRead::PwmRead(uint8_t Inv /*= 0*/)
{
  _Info.Inv = Inv;
}

int8_t PwmRead::attach(uint8_t Pin, uint16_t PulseMin_us/*=600*/, uint16_t PulseMax_us/*=2400*/)
{
  int8_t Ret = -1;


  if(Pin < 16)
  {
    _Pin     = Pin;
    _Min_us  = PulseMin_us;
    _Max_us  = PulseMax_us;
    _Info.VirtualPortIdx = 0;
    prev  = last;
    last = this;
    pinMode(_Pin, INPUT);
    digitalWrite(_Pin, HIGH); /* Eanble Pull-up */
    attachInterrupt(digitalPinToInterrupt(_Pin), PwmRead::PwmReadInterrupt0ISR, CHANGE);
    Ret = 1;
  }

  return(Ret);
}

// void PwmRead::detach(void)
// {
  // PwmRead *RcPulseIn;
  // uint8_t sreg_save = SREG;

  // detachInterrupt(digitalPinToInterrupt(_Pin)) 
  // /* Remove object from the chained list */
  // cli();
  // if(last == this)
  // {
    // /* Is the last object */
    // last = this->prev;
  // }
  // else
  // {
    // /* Is NOT the last object */
    // for(RcPulseIn = last; RcPulseIn != 0; RcPulseIn = RcPulseIn->prev)
    // {
      // if(RcPulseIn->prev == this)
      // {
        // RcPulseIn->prev = RcPulseIn->prev->prev;
        // break;
      // }
    // }
  // }
  // SREG = sreg_save; /* Restore initial SREG value */
// }

uint8_t PwmRead::available(uint8_t ClientIdx /*= 7*/)
{
  uint8_t  Ret = 0;
  uint16_t PulseWidth_us;

  if(isSynchro(ClientIdx))
  {
    /* Read Pulse without disabling interrupts */
    do
    {
      PulseWidth_us = _Width_us;
    }while(PulseWidth_us != _Width_us);

    Ret = (PulseWidth_us >= _Min_us) && (PulseWidth_us <= _Max_us);
  }
  return(Ret);
}

uint8_t PwmRead::isSynchro(uint8_t ClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(_Available & RCUL_CLIENT_MASK(ClientIdx));
  if(Ret) _Available &= ~RCUL_CLIENT_MASK(ClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

uint8_t PwmRead::timeout(uint8_t TimeoutMs, uint8_t *CurrentState)
{
  uint8_t CurMs, Ret = 0;

  CurMs = (uint8_t)(millis() & 0x000000FF);
  if((uint8_t)(CurMs - _LastTimeStampMs) >= TimeoutMs)
  {
    *CurrentState = digitalRead(_Pin);
    Ret = 1;
  }
  return(Ret);
}

uint16_t PwmRead::width_us(void)
{
  uint16_t PulseWidth_us;

  /* Read Pulse without disabling interrupts */
  do
  {
    PulseWidth_us = _Width_us;
  }while(PulseWidth_us != _Width_us);

  return(PulseWidth_us);  
}

uint32_t PwmRead::start_us(void)
{
  uint32_t Start_us;
  
  /* Read Pulse start without disabling interrupts */
  do
  {
    Start_us = _Start_us;
  }while(Start_us != _Start_us);

  return(Start_us);
}

/* Begin of Rcul support */
uint8_t PwmRead::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(available(ClientIdx));
}

uint16_t PwmRead::RculGetWidth_us(uint8_t Ch)
{
  Ch = Ch; /* To avoid a compilation warning */
  return(width_us());
}

void PwmRead::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}
/* End of Rcul support */

#define DECLARE_READ_RC_PULSE_IN_ISR(VirtualPort)                                         \
void PwmRead::PwmReadInterrupt##VirtualPort##ISR(void)                                    \
{                                                                                         \
  PwmRead        *RcPulseIn;                                                              \
  uint8_t        PinState;                                                                \
  uint32_t       NowUs;                                                                   \
                                                                                          \
  NowUs = micros();                                                                       \
  for(RcPulseIn = last; RcPulseIn != 0; RcPulseIn = RcPulseIn->prev)                      \
  {                                                                                       \
    if(RcPulseIn->_Info.VirtualPortIdx != VirtualPort) continue;                          \
    PinState = digitalRead(RcPulseIn->_Pin);                                              \
    if(PinState  ^ RcPulseIn->_Info.Inv)                                                  \
    {                                                                                     \
      /* High level, rising edge: start chrono */                                         \
      RcPulseIn->_Start_us = NowUs;                                                       \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
      /* Low level, falling edge: stop chrono */                                          \
      RcPulseIn->_Width_us = (uint16_t)(NowUs - RcPulseIn->_Start_us);                    \
      RcPulseIn->_Available = 0xFF;                                                       \
      RcPulseIn->_LastTimeStampMs = (uint8_t)(millis() & 0x000000FF);                     \
    }                                                                                     \
  }                                                                                       \
}

DECLARE_READ_RC_PULSE_IN_ISR(0)