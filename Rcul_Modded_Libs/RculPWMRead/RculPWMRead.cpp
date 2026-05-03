#include "RculPWMRead.h"


RculPWMRead *RculPWMRead::last = NULL;

RculPWMRead::RculPWMRead(uint8_t Inv /*= 0*/)
{
  _Info.Inv = Inv;
  _Info.VirtualPortIdx = 0;
  _Pin = 0xFF;
  _Min_us = 600;
  _Max_us = 2400;
  _Start_us = 0;
  _Width_us = 1500;
  _Available = 0;
  _LastTimeStampMs = 0;
  _LastPinState = 0;
  prev = NULL;
}

int8_t RculPWMRead::attach(uint8_t Pin,
                           uint16_t PulseMin_us,
                           uint16_t PulseMax_us)
{
  int8_t Ret = -1;

  int irq = digitalPinToInterrupt(Pin);
  if (irq >= 0)
  {
    _Pin     = Pin;
    _Min_us  = PulseMin_us;
    _Max_us  = PulseMax_us;
    _Info.VirtualPortIdx = 0;

    pinMode(_Pin, INPUT_PULLUP);
    _LastPinState = digitalRead(_Pin);

#if defined(ESP32)
    /* ESP32/S3: attach one ISR context per object.
       This avoids the AVR-style shared ISR scanning all inputs on every edge. */
    attachInterruptArg(_Pin,
                       RculPWMRead::RculPWMReadInterruptArgISR,
                       this,
                       CHANGE);
#else
    /* Teensy/AVR/RP2040 path: attachInterrupt() does not pass "this".
       We keep one shared ISR, but the ISR now processes ONLY pins whose state changed.
       This prevents an edge on one RC input from corrupting another input width. */
    prev = last;
    last = this;

    attachInterrupt(irq,
                    RculPWMRead::RculPWMReadInterrupt0ISR,
                    CHANGE);
#endif

    Ret = 1;
  }

  return Ret;
}

void RculPWMRead::detach(void)
{
#if defined(ESP32)
  if(_Pin != 0xFF) detachInterrupt(_Pin);
#else
  if(_Pin != 0xFF) detachInterrupt(digitalPinToInterrupt(_Pin));

  /* Remove object from the chained list */
  noInterrupts();
  if(last == this)
  {
    last = this->prev;
  }
  else
  {
    RculPWMRead *RcPulseIn;
    for(RcPulseIn = last; RcPulseIn != 0; RcPulseIn = RcPulseIn->prev)
    {
      if(RcPulseIn->prev == this)
      {
        RcPulseIn->prev = RcPulseIn->prev->prev;
        break;
      }
    }
  }
  interrupts();
#endif

  _Pin = 0xFF;
}

uint8_t RculPWMRead::available(uint8_t ClientIdx /*= 7*/)
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

uint8_t RculPWMRead::isSynchro(uint8_t ClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(_Available & RCUL_CLIENT_MASK(ClientIdx));
  if(Ret) _Available &= ~RCUL_CLIENT_MASK(ClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

uint8_t RculPWMRead::timeout(uint8_t TimeoutMs, uint8_t *CurrentState)
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

uint16_t RculPWMRead::width_us(void)
{
  uint16_t PulseWidth_us;

  /* Read Pulse without disabling interrupts */
  do
  {
    PulseWidth_us = _Width_us;
  }while(PulseWidth_us != _Width_us);

  return(PulseWidth_us);  
}

uint32_t RculPWMRead::start_us(void)
{
  uint32_t Start_us;
  
  /* Read Pulse start without disabling interrupts */
  do
  {
    Start_us = _Start_us;
  }while(Start_us != _Start_us);

  return(Start_us);
}

void RculPWMRead::handleInterruptFromState(uint8_t PinState, uint32_t NowUs)
{
  if(PinState ^ _Info.Inv)
  {
    /* High level, rising edge: start chrono */
    _Start_us = NowUs;
  }
  else
  {
    /* Low level, falling edge: stop chrono */
    _Width_us = (uint16_t)(NowUs - _Start_us);
    _Available = 0xFF;
    _LastTimeStampMs = (uint8_t)(millis() & 0x000000FF);
  }
}

#if defined(ESP32)
void IRAM_ATTR RculPWMRead::RculPWMReadInterruptArgISR(void *Arg)
{
  if(Arg) ((RculPWMRead *)Arg)->handleInterrupt();
}

void IRAM_ATTR RculPWMRead::handleInterrupt(void)
{
  uint8_t  PinState;
  uint32_t NowUs;

  NowUs = micros();
  PinState = digitalRead(_Pin);
  _LastPinState = PinState;

  handleInterruptFromState(PinState, NowUs);
}
#endif

/* Begin of Rcul support */
uint8_t RculPWMRead::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(available(ClientIdx));
}

uint16_t RculPWMRead::RculGetWidth_us(uint8_t Ch)
{
  Ch = Ch; /* To avoid a compilation warning */
  return(width_us());
}

void RculPWMRead::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}
/* End of Rcul support */

#define DECLARE_READ_RC_PULSE_IN_ISR(VirtualPort)                                         \
void RculPWMRead::RculPWMReadInterrupt##VirtualPort##ISR(void)                            \
{                                                                                         \
  RculPWMRead *RcPulseIn;                                                                 \
  uint8_t     PinState;                                                                   \
  uint32_t    NowUs;                                                                      \
                                                                                          \
  NowUs = micros();                                                                       \
  for(RcPulseIn = last; RcPulseIn != 0; RcPulseIn = RcPulseIn->prev)                      \
  {                                                                                       \
    if(RcPulseIn->_Info.VirtualPortIdx != VirtualPort) continue;                          \
    PinState = digitalRead(RcPulseIn->_Pin);                                              \
    if(PinState == RcPulseIn->_LastPinState) continue;                                    \
    RcPulseIn->_LastPinState = PinState;                                                  \
    RcPulseIn->handleInterruptFromState(PinState, NowUs);                                 \
  }                                                                                       \
}

DECLARE_READ_RC_PULSE_IN_ISR(0)
