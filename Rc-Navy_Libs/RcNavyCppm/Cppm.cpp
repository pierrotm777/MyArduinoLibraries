/* An interrupt driven RC PPM frame generator *and* RC PPM frame reader library using compare match of a 16 bits timer

   CPPM Generator (CppmGen object):
   ===============================
   Features:
   - Can generate a PPM Frame containing up to 12 RC Channels (8 channels 600 -> 2000 us with the 20ms default PPM period, up to 12 channels with higher PPM period).
   - Positive or Negative Modulation supported
   - Constant PPM Frame period: configurable from 10 to 40 ms (default = 20 ms)
   - No need to wait the PPM Frame period (usually 20 ms) to set the pulse width order for the channels, can be done at any time
   - Synchronisation indicator for digital data transmission over PPM
   - Blocking fonctions such as delay() can be used in the loop() since it's an interrupt driven PPM generator
   - Supported devices:
       - ATmega328P (Arduino UNO, Nano V3):
         TIMER(1), CHANNEL(A) -> OC1A -> PB1 -> Pin#9
         
   CPPM Reader (CppmReader object):
   ===============================
   Features:
   - Can read a PPM Frame containing up to 12 RC Channels (8 channels 600 -> 2000 us with the 20ms default PPM period, up to 12 channels with higher PPM period).
   - Automatic detection of:
      - Positive or Negative Modulation
      - The PPM frame period
      - Number of transported channels
      - Pulse width of each transported channel
   - Synchronisation indicator for digital data transmission over PPM
   - Blocking fonctions such as delay() can be used in the loop() since it's an interrupt driven PPM reader
   - Supported devices:
       - ATmega328P (Arduino UNO, Nano V3):
         TIMER(1), CHANNEL(A) -> ICP1 -> PB0 -> Pin#8

RC Navy 2016
   http://p.loussouarn.free.fr
   08/11/2016: Creation
*/
#include <Cppm.h>


#define PPM_US_TO_TICK(Us)         ((Us) << 1)
#define PPM_TICK_TO_US(Ticks)      ((Ticks) >> 1)

#define CHANNEL_MAX_NB             12

#define SYNCHRO_MIN_US             3500
#define CHANNEL_MAX_US             2000

#define PPM_NEUTRAL_US             1500

#define PPM_FRAME_MIN_PERIOD_US    10000
#define PPM_FRAME_MAX_PERIOD_US    40000

#define SYNCHRO_TIME_MIN_US        3000

#define CPPM_ICP1_PIN              8 // Input  Capture Pin 1 of Timer1 is Arduino pin 8 (Atmega328 PB0)

#define CPPM_OC1A_PIN              9 // Output Compare Pin A of Timer1 is Arduino pin 9 (Atmega328 PB1)

/*
Positive PPM    .-----.                         .-----.
                |     |                         |     |
                |  H  |                         |  H  |
                |     |                         |     |
             ---'     '-------------------------'     '-----
                <------------------------------->
                        Channel Duration
                        
Negative PPM ---.     .-------------------------.     .-----
                |     |                         |     |
                |  H  |                         |  H  |
                |     |                         |     |
                '-----'                         '-----'
                <------------------------------->
                        Channel Duration
Legend:
======
 H: Pulse header duration in ticks (Nb of Ticks corresponding to PPM_HEADER_US us)
*/

/* Global variables */

CppmGenClass    CppmGen    = CppmGenClass();
CppmReaderClass CppmReader = CppmReaderClass();

static void Timer1_Init(void);
static uint8_t Timer1InitDOne = 0;
/*********************************************
              CPPM GENERATION
*********************************************/
/* Public functions */
CppmGenClass::CppmGenClass(void) /* Constructor */
{
  
}

uint16_t CppmGenClass::begin(uint8_t PpmModu, uint8_t ChNb, uint16_t PpmPeriod_us /*= DEFAULT_PPM_PERIOD*/, uint16_t PpmHeader_us /*= DEFAULT_PPM_HEADER_US*/)
{
  uint8_t Ok = false;
  uint8_t Ch;

  _Modu = PpmModu;
  _PpmPeriod_us = 0;
  _ChMaxNb = (ChNb > CHANNEL_MAX_NB)?CHANNEL_MAX_NB:ChNb; /* Max is 12 Channels */
  if(!_Next_Tick_Nb) _Next_Tick_Nb    = (uint16_t *) malloc(sizeof(uint16_t) * (_ChMaxNb + 1)); /* + 1 for Synchro Channel */
  else               _Next_Tick_Nb    = (uint16_t *)realloc(_Next_Tick_Nb, sizeof(uint16_t) * (_ChMaxNb + 1)); /* + 1 for Synchro Channel */
  Ok = (_Next_Tick_Nb != NULL);
  if(Ok)
  {
    _Idx = _ChMaxNb; /* To reload values at startup */
    _PpmPeriod_us = PpmPeriod_us;
    _PpmHeader_us    = PpmHeader_us;
    if(_PpmPeriod_us < PPM_FRAME_MIN_PERIOD_US)                                  _PpmPeriod_us = PPM_FRAME_MIN_PERIOD_US;
    if(_PpmPeriod_us < ((_ChMaxNb * (uint16_t)CHANNEL_MAX_US) + SYNCHRO_MIN_US)) _PpmPeriod_us = (_ChMaxNb * (uint16_t)CHANNEL_MAX_US) + SYNCHRO_MIN_US;
    if(_PpmPeriod_us > PPM_FRAME_MAX_PERIOD_US)                                  _PpmPeriod_us = PPM_FRAME_MAX_PERIOD_US;
    for(Ch = 1; Ch <= _ChMaxNb; Ch++) /* Init all the channels to Neutral */
    {
      this->width_us(Ch, PPM_NEUTRAL_US);
    }
    /* Configure the output compare pin */
    digitalWrite(CPPM_OC1A_PIN, _Modu? LOW: HIGH);
    pinMode(CPPM_OC1A_PIN, OUTPUT);
    
    Timer1_Init();
    // Enable Timer1 output compare interrupt...
    bitSet(TIFR1, OCF1A); // clr pending interrupt
    bitSet(TIMSK1, OCIE1A); // enable interrupt
  }
  return(_PpmPeriod_us);
}

void CppmGenClass::width_us(uint8_t Ch, uint16_t Width_us)
{
  uint16_t SumTick = 0, Ch_Next_Tick_Nb, Ch0_Next_Tick_Nb;

  if((Ch >= 1) && (Ch <= _ChMaxNb))
  {
    Ch_Next_Tick_Nb = PPM_US_TO_TICK(Width_us - _PpmHeader_us); /* Convert in Timer Ticks */
    /* Update Synchro Time */
    for(uint8_t Idx = 1; Idx <= _ChMaxNb; Idx++)
    {
      SumTick += PPM_US_TO_TICK(_PpmHeader_us);
      if(Idx != Ch)
      {
        SumTick += _Next_Tick_Nb[Idx];
      }
      else
      {
        SumTick += Ch_Next_Tick_Nb;
      }
    }
    Ch0_Next_Tick_Nb = PPM_US_TO_TICK(_PpmPeriod_us) - SumTick - PPM_US_TO_TICK(_PpmHeader_us);
    /* Update requested Channel AND Synchro to keep constant the period (usually 20ms) */
    cli();
    _Next_Tick_Nb[0]  = Ch0_Next_Tick_Nb;
    _Next_Tick_Nb[Ch] = Ch_Next_Tick_Nb;
    sei();
  }
}

void CppmGenClass::header_us(uint16_t Header_us)
{
  cli();
  _PpmHeader_us = Header_us;
  sei();
}

uint8_t CppmGenClass::isSynchro(uint8_t SynchroClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(_Synchro & RCUL_CLIENT_MASK(SynchroClientIdx));
  if(Ret) _Synchro &= ~RCUL_CLIENT_MASK(SynchroClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

void CppmGenClass::suspend(void)
{
    bitClear(TIMSK1, OCIE1A); /* disable interrupt */
}

void CppmGenClass::resume(void)
{
    bitSet(TIMSK1, OCIE1A); /* enable interrupt */
}

/* Begin of Rcul support */
uint8_t CppmGenClass::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(isSynchro(ClientIdx));
}

void CppmGenClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  this->width_us(Ch, Width_us); /* Take care about argument order (like that, since Ch will be optional for other pulse generator) */
}

uint16_t CppmGenClass::RculGetWidth_us(uint8_t Ch)
{
  Ch = Ch; /* To avoid a compilation warning */
  return(0);
}

/* End of Rcul support */


void CppmGenClass::rcPpmGenIsr(void)
{
  CppmGenClass *PpmGen = &CppmGen;

  /* Next Channel or Synchro */
  if(PpmGen->_EndOfCh)
  {
    PpmGen->_Idx++;
    if(PpmGen->_Idx > PpmGen->_ChMaxNb)
    {
      /* End of PPM Frame */
      PpmGen->_Idx = 0;
      PpmGen->_Synchro = 0xFF; /* OK: Widths loaded */
    }
  }
  /* Generate Next Channel or Synchro */
  if ( (PpmGen->_Modu && (PINB & _BV(PINB1))) || (!PpmGen->_Modu && !(PINB & _BV(PINB1))) )
  {
    OCR1A += PPM_US_TO_TICK(PpmGen->_PpmHeader_us);
    PpmGen->_EndOfCh = 0;
  }
  else
  {
    OCR1A += PpmGen->_Next_Tick_Nb[PpmGen->_Idx];
    PpmGen->_EndOfCh = 1;
  }
}

/* CPPM GENERATION INTERRUPT ROUTINE */
SIGNAL(TIMER1_COMPA_vect)
{
  CppmGenClass::rcPpmGenIsr();
}

/*********************************************
              CPPM READER
*********************************************/
/* Public functions */
CppmReaderClass::CppmReaderClass(void) /* Constructor */
{
  
}

void CppmReaderClass::begin(void)
{
  /* Configure the input capture pin */
  pinMode(CPPM_ICP1_PIN, INPUT_PULLUP);

  for(uint8_t Idx = 0; Idx < CPPM_READER_CH_MAX; Idx++)
  {
    _ChWidthTicks[Idx] = PPM_TICK_TO_US(PPM_NEUTRAL_US);
  }
  _ChIdx    = (CPPM_READER_CH_MAX + 1);
  _ChIdxMax = 0;
  _Synchro  = 0;
  Timer1_Init();
  TCCR1B = _BV(ICES1)*1 | _BV(CS11); /* Next Input Capture Interrupt at rising edge. */
  // Enable Noise canceler
  bitSet(TCCR1B, ICNC1);
  // Enable Timer1 input capture interrupt...
  bitSet(TIFR1, ICF1); // clr pending interrupt
  bitSet(TIMSK1, ICIE1); // enable interrupt
}

uint8_t CppmReaderClass::detectedChannelNb(void)
{ 
  return(_ChIdxMax); /* No need to mask/unmask interrupt (8 bits) */
}

uint16_t CppmReaderClass::width_us(uint8_t Ch)
{
  uint16_t WidthTicks = PPM_TICK_TO_US(1500);
  if((Ch >= 1) && (Ch <= CppmReaderClass::detectedChannelNb()))
  {
    Ch--;
    /* Read pulse width without disabling interrupts */
    do
    {
      WidthTicks = _ChWidthTicks[Ch];
    }while(WidthTicks != _ChWidthTicks[Ch]);
  }
  return(PPM_TICK_TO_US(WidthTicks));
}

uint16_t CppmReaderClass::ppmPeriod_us(void)
{
  uint16_t PpmPeriodTicks = 0;
    /* Read PPM Period without disabling interrupts */
    do
    {
      PpmPeriodTicks = _PpmPeriodTicks;
    }while(PpmPeriodTicks != _PpmPeriodTicks);
  return(PPM_TICK_TO_US(PpmPeriodTicks));
}

uint16_t CppmReaderClass::ppmHeader_us(void)
{
  uint16_t PpmHeaderUs;
  cli();
//  do {
  PpmHeaderUs = (uint16_t)pulseIn(CPPM_ICP1_PIN, (_Modu == CPPM_GEN_POS_MOD));
  if(!PpmHeaderUs)
  {
    /* Try again */
    PpmHeaderUs = (uint16_t)pulseIn(CPPM_ICP1_PIN, (_Modu == CPPM_GEN_POS_MOD));
  }
//  }while(PpmHeaderUs < 100 || PpmHeaderUs > 500); /* /!\ TODO: do not block here /!\ */
  sei();
  return(PpmHeaderUs);
}

uint8_t CppmReaderClass::modulation(void)
{
  return(_Modu);
}

uint8_t CppmReaderClass::isSynchro(uint8_t SynchroClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(_Synchro & RCUL_CLIENT_MASK(SynchroClientIdx));
  if(Ret) _Synchro &= ~RCUL_CLIENT_MASK(SynchroClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

/* Begin of Rcul support */
uint8_t CppmReaderClass::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(isSynchro(ClientIdx));
}

uint16_t CppmReaderClass::RculGetWidth_us(uint8_t Ch)
{
  return(width_us(Ch));
}

void CppmReaderClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}

/* End of Rcul support */

void CppmReaderClass::suspend(void)
{
  // Disable Timer1 input capture interrupt...
  bitClear(TIMSK1, ICIE1); // disnable interrupt
  bitSet(TIFR1, ICF1); // clr pending interrupt
  _ChIdx = (CPPM_READER_CH_MAX + 1);
}

void CppmReaderClass::resume(void)
{
  cli();
  _PrevEdgeTicks = ICR1;
  sei();
  // Enable Timer1 input capture interrupt...
  bitSet(TIFR1, ICF1); // clr pending interrupt
  bitSet(TIMSK1, ICIE1); // enable interrupt
}

void CppmReaderClass::rcChannelCollectorIsr(void)
{
  static uint8_t Period = 0;
  CppmReaderClass *PpmReader = &CppmReader;
  uint16_t CurrentEdgeTicks, PulseDurationTicks;

  if(TCCR1B &	_BV(ICES1)) // rising edge
  {
    CurrentEdgeTicks   = ICR1;
    PulseDurationTicks = CurrentEdgeTicks - PpmReader->_PrevEdgeTicks;
    PpmReader->_PrevEdgeTicks = CurrentEdgeTicks;
    if(PulseDurationTicks >= PPM_US_TO_TICK(SYNCHRO_TIME_MIN_US))
    {
      PpmReader->_ChIdxMax = PpmReader->_ChIdx;
      PpmReader->_ChIdx    = 0;
      PpmReader->_Synchro  = 0xFF; /* Synchro detected */
      PpmReader->_Modu     = (PINB & _BV(PINB0))? CPPM_GEN_NEG_MOD: CPPM_GEN_POS_MOD;
      Period = !Period;
      if(Period) PpmReader->_StartPpmPeriodTicks = CurrentEdgeTicks;
      else       PpmReader->_PpmPeriodTicks      = CurrentEdgeTicks - PpmReader->_StartPpmPeriodTicks;
    }
    else
    {
      if(PpmReader->_ChIdx < CPPM_READER_CH_MAX)
      {
        PpmReader->_ChWidthTicks[PpmReader->_ChIdx] = PulseDurationTicks;
        PpmReader->_ChIdx++;
      }
    }
  }  
}

/* CPPM READING INTERRUPT ROUTINE */
ISR(TIMER1_CAPT_vect)
{
  CppmReaderClass::rcChannelCollectorIsr();
}

static void Timer1_Init(void)
{

  if(!Timer1InitDOne)
  {
    // Configure timer1: disable PWM, set prescaler /8 (0.5 usec ticks)
    TCCR1A = _BV(COM1A0);	// Toggle OC1A/OC1B on Compare Match.
    TCCR1B = _BV(ICNC1) | _BV(ICES1)*0 | _BV(CS11);
    TCCR1C = 0;

    OCR1A += PPM_US_TO_TICK(DEFAULT_PPM_HEADER_US); // start at around 300us...

    // Enable Timer1 overflow...
    bitSet(TIFR1, TOV1); // clr pending interrupt
    // bitSet(TIMSK1, TOIE1); // enable interrupt
    Timer1InitDOne = 1;
  }
}
