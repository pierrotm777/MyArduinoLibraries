
#include <RculPPMRead.h>

#define NEUTRAL_US           1500
#define SYNCHRO_TIME_MIN_US  3000

typedef struct{
  uint8_t PeriodPhase   :1,
          ChIdx         :5;
}CppmSt_t;


static volatile uint16_t _ChWidthUs[CPPM_READER_CH_MAX];

static volatile uint8_t  _CppmFrameInputPin;
static volatile int8_t   _VirtualPort;
static volatile CppmSt_t _Cppm = {0, 0};
static volatile uint8_t  _Synchro;
static volatile uint8_t  _ChIdx;
static volatile uint8_t  _ChIdxMax;
static volatile uint16_t _PrevEdgeUs;
static volatile uint16_t _StartCppmPeriodUs;
static volatile uint16_t _CppmPeriodUs;

//RculPPMRead CppmReader = RculPPMRead();à tester

/* Public functions */
RculPPMRead::RculPPMRead(void) /* Constructor */
{
}

uint8_t RculPPMRead::attach(uint8_t CppmInputPin)
{
  uint8_t Ret = 0;
  attachInterrupt(digitalPinToInterrupt(CppmInputPin), rcChannelCollectorIsr, RISING);
  //if(_VirtualPort >= 0)
  //{
    for(uint8_t Idx = 0; Idx < CPPM_READER_CH_MAX; Idx++)
    {
      _ChWidthUs[Idx] = NEUTRAL_US;
    }
    _ChIdx    = (CPPM_READER_CH_MAX + 1);
    _ChIdxMax = 0;
    _Synchro  = 0;
    _CppmFrameInputPin = CppmInputPin;
    //TinyPinChange_EnablePin(_CppmFrameInputPin);
    Ret = 1;
  //}
  return(Ret);
}

// uint8_t RculPPMRead::detach(void)
// {
  // suspend();
  // return(TinyPinChange_UnregisterIsr(_CppmFrameInputPin, RculPPMRead::rcChannelCollectorIsr));
// }

void RculPPMRead::trackChId(uint8_t ChId)
{
  if((ChId >= 1) && (ChId <= CPPM_READER_CH_MAX))
  {
    _Cppm.ChIdx = ChId - 1;
  }
}

uint8_t RculPPMRead::detectedChannelNb(void)
{ 
  return(_ChIdxMax); /* No need to mask/unmask interrupt (8 bits) */
}

uint16_t RculPPMRead::width_us(uint8_t Ch)
{
  uint16_t Width_us = 1500;

  if((Ch >= 1) && (Ch <= RculPPMRead::detectedChannelNb()))
  {
    Ch--;
    /* Read pulse width without disabling interrupts */
    do
    {
      Width_us = _ChWidthUs[Ch];
    }while(Width_us != _ChWidthUs[Ch]);
  }
  return(Width_us);
}

uint16_t RculPPMRead::cppmPeriod_us(void)
{
  uint16_t CppmPeriod_us = 0;
    /* Read CPPM Period without disabling interrupts */
    do
    {
      CppmPeriod_us = _CppmPeriodUs;
    }while(CppmPeriod_us != _CppmPeriodUs);
  return(CppmPeriod_us);
}

uint8_t RculPPMRead::isSynchro(uint8_t ClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(_Synchro & RCUL_CLIENT_MASK(ClientIdx));
  if(Ret) _Synchro &= ~RCUL_CLIENT_MASK(ClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

/* Begin of Rcul support */
uint8_t RculPPMRead::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(isSynchro(ClientIdx));
}

uint16_t RculPPMRead::RculGetWidth_us(uint8_t Ch)
{
  return(width_us(Ch));
}

void RculPPMRead::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= RCUL_NO_CH*/)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}

/* End of Rcul support */

// void RculPPMRead::suspend(void)
// {
  // TinyPinChange_DisablePin(_CppmFrameInputPin);
  // _ChIdx = (CPPM_READER_CH_MAX + 1);
// }

// void RculPPMRead::resume(void)
// {
  // _PrevEdgeUs = (uint16_t)(micros() & 0xFFFF);
  // TinyPinChange_EnablePin(_CppmFrameInputPin);
// }

/* ISR */
void RculPPMRead::rcChannelCollectorIsr(void)
{
  uint16_t CurrentEdgeUs, PulseDurationUs;
  
  //if(TinyPinChange_RisingEdge(_VirtualPort, _CppmFrameInputPin))
  //{
    CurrentEdgeUs   = (uint16_t)(micros() & 0xFFFF);
    PulseDurationUs = CurrentEdgeUs - _PrevEdgeUs;
    _PrevEdgeUs = CurrentEdgeUs;
    if(PulseDurationUs >= SYNCHRO_TIME_MIN_US)
    {
      _ChIdxMax = _ChIdx;
      _ChIdx    = 0;
      _Synchro  = 0xFF; /* Synchro detected */
      _Cppm.PeriodPhase = !_Cppm.PeriodPhase;
      if(_Cppm.PeriodPhase) _StartCppmPeriodUs = CurrentEdgeUs;
      else                  _CppmPeriodUs      = CurrentEdgeUs - _StartCppmPeriodUs;
    }
    else
    {
      if(_ChIdx < CPPM_READER_CH_MAX)
      {
        _ChWidthUs[_ChIdx] = PulseDurationUs;
        _ChIdx++;
      }
    }
  //}

}
