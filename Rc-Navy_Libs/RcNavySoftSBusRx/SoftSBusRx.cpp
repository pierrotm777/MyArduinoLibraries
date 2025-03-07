/*
 English: by RC Navy (2023)
 =======
 <SoftSBusRx>: a library to read RC SBUS frame. No hardware inverter needed. All implementation done in software.
 http://p.loussouarn.free.fr
 V1.0 (12/02/2023): initial release

 English: by RC Navy (2023)
 =======
 <SoftSBusRx>: une bibliotheque pour lire les trames SBUS RC. Pas besoin d'inverseur materiel. Toute l'implementation est faite par logiciel.
 http://p.loussouarn.free.fr
 V1.0 (12/02/2023): release initiale
*/

#include "SoftSBusRx.h"
#include "Misclib.h"
#include <util/delay.h>

#define SBUS_HEADER_0x0F      0x0F
#define SBUS_TRAILER_0x00     0x00

#define SBUS_SILENCE_3_MS     3000

SoftSBusRxClass               SoftSBusRx = SoftSBusRxClass();
static uint8_t volatile       Synchro = 0;
static sbusInfo_t volatile    sbusInfo;

//#define DBG_PIN               H, 4 //B, 2 // Uncomment this line after having adjusted the Port and the bit in port to debug

#ifdef INPUT
#undef INPUT
#endif
#define INPUT    0

#ifdef OUTPUT
#undef OUTPUT
#endif
#define OUTPUT   1

#ifdef LOW
#undef LOW
#endif
#define LOW     0

#ifdef HIGH
#undef HIGH
#endif
#define HIGH    1

// Some macros to easily manipulate I/O pins (for debugging purpose only)
#define PIN_MODE2(Port,  BitInPort, Dir)     (Dir)? ((DDR##Port)  |= _BV(BitInPort)): ((DDR##Port)  &= ~_BV(BitInPort))
#define PIN_MODE(Port_BitInPort, Dir)         PIN_MODE2(Port_BitInPort, Dir)

#define DIGITAL_READ2(Port, BitInPort)        !!((PIN##Port) & _BV(BitInPort))
#define DIGITAL_READ(Port_BitInPort)          DIGITAL_READ2(Port_BitInPort)

#define DIGITAL_WRITE2(Port, BitInPort, Val)  (Val)? ((PORT##Port) |= _BV(BitInPort)): ((PORT##Port) &= ~_BV(BitInPort))
#define DIGITAL_WRITE(Port_BitInPort, Val)    DIGITAL_WRITE2(Port_BitInPort, Val)

#define DIGITAL_TOGGLE2(Port, BitInPort)      (PIN##Port) |= _BV(BitInPort)
#define DIGITAL_TOGGLE(Port_BitInPort)        DIGITAL_TOGGLE2(Port_BitInPort)

// quick IO functions
#define portOfPin(P)          portOutputRegister(digitalPinToPort(P))
#define ddrOfPin(P)           portModeRegister(digitalPinToPort(P))
#define pinOfPin(P)           portInputRegister(digitalPinToPort(P))
#define pinMask(P)            digitalPinToBitMask(P)
#define pinAsInput(P)         (*(ddrOfPin(P)) &= ~pinMask(P))
#define pinAsInputPullUp(P)   (*(ddrOfPin(P)) &= ~pinMask(P)); digitalHigh(P)
#define pinAsOutput(P)        (*(ddrOfPin(P)) |= pinMask(P))
#define digitalLow(P)         (*(portOfPin(P)) &= ~pinMask(P))
#define digitalHigh(P)        (*(portOfPin(P)) |= pinMask(P))
#define isHigh(P)             ((*(pinOfPin(P)) & pinMask(P)) > 0)
#define isLow(P)              ((*(pinOfPin(P)) & pinMask(P)) == 0)
#define pinGet(pin, mask)     (((*(pin)) & (mask)) > 0)

#define WAIT_TENTH_OF_US(TenthOfUs)   _delay_loop_1(((TenthOfUs) * 8) / 15) // Max(us) = (256 x 3) / 16 = 48us@16MHz (0 is equivalent to 256)

static uint8_t                _pin;

static volatile uint8_t      *s_pin;
static uint8_t                s_pinMask;
static uint8_t                s_PCICRMask;

// contains which byte is currently receiving (0..24, 24 is the last byte)
static volatile int8_t        s_RxByteIdx = -1;

#if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1)
// Part of frame containing a single channel 
#define SBUS_USEFULL_BYTE_NB  3 // SBUS 11 bit channel value may be scattered into 3 bytes in the frame
#else
// Full SBUS data frame (Header and trailer bytes skipped)
#define SBUS_USEFULL_BYTE_NB  23
#endif

static volatile uint8_t       sbus_data_frame[SBUS_USEFULL_BYTE_NB];

struct chinfo_t {
  uint8_t idx;
  uint8_t shift[3];
};

static const chinfo_t ElevenBitChLoc[16] PROGMEM = {
                              {0,  {0, 8, 11}}, {1,  {3, 5, 11}}, {2,  {6, 2, 10}}, {4,  {1, 7, 11}},
                              {5,  {4, 4, 11}}, {6,  {7, 1,  9}}, {8,  {2, 6, 11}}, {9,  {5, 3, 11}},
                              {11, {0, 8, 11}}, {12, {3, 5, 11}}, {13, {6, 2, 10}}, {15, {1, 7, 11}},
                              {16, {4, 4, 11}}, {17, {7, 1,  9}}, {19, {2, 6, 11}}, {20, {5, 3, 11}} };               

inline void enablePinChangeInterrupts();

/* Constructor */
SoftSBusRxClass::SoftSBusRxClass(void)
{

}

void SoftSBusRxClass::begin(uint8_t pin, uint8_t Inverted /*= 1*/)
{
  _pin         = pin;
  s_pin        = pinOfPin(pin);
  s_pinMask    = pinMask(pin);
  s_PCICRMask  = 1 << digitalPinToPCICRbit(pin);
  sbusInfo.inv = !!Inverted;

  // setup pin change interrupt
  pinAsInput(pin);

  TinyPinChange_RegisterIsr(pin, SoftSBusRxClass::handle_interrupt);

  // enable pin
  *digitalPinToPCMSK(pin) |= 1 << digitalPinToPCMSKbit(pin);  
  enablePinChangeInterrupts();
#if defined(DBG_PIN)
  #warning Debug enabled in SoftSBusRx!
  PIN_MODE(DBG_PIN, OUTPUT);
#endif
}

void SoftSBusRxClass::trackChId(uint8_t ChId)
{
  chinfo_t chinfo;
  
  memcpy_P(&chinfo, ElevenBitChLoc + ChId - 1, sizeof(chinfo_t));
  sbusInfo.byteIdx = chinfo.idx; // The first byte of the channel in the SBUS stream
}

inline void enablePinChangeInterrupts()
{
  // clear any outstanding interrupt
  PCIFR |= s_PCICRMask; 
  
  // enable interrupt for the group
  PCICR |= s_PCICRMask;   
}

inline void disablePinChangeInterrupts()
{
  PCICR &= ~s_PCICRMask; 
}

// ChId SHALL be within [1..16]
// returns a value within [0..2047] (as received by SoftSBusRxClass)
// Note: this method SHALL be called before the beginning of the next SBUS frame to be valid!
uint16_t SoftSBusRxClass::rawData(uint8_t ChId)
{
  uint8_t  idx = 0;
  chinfo_t chinfo;
  
  if (ChId >= 1 && ChId <= 16)
  {
    memcpy_P(&chinfo, ElevenBitChLoc + ChId - 1, sizeof(chinfo_t));
    #if (SOFT_SBUS_RX_SINGLE_CHANNEL == 0)
    idx = chinfo.idx;
    #endif
    return (((sbus_data_frame[idx++] >> chinfo.shift[0]) | (sbus_data_frame[idx++] << chinfo.shift[1]) | (sbus_data_frame[idx] << chinfo.shift[2])) & 0x7FF);
  }
  return 0; // error
}

// ChId SHALL be within [1..16]
// returns a value within [880..2159]
uint16_t SoftSBusRxClass::width_us(uint8_t ChId)
{
  return (5 * rawData(ChId) / 8 + 880);
}

uint8_t SoftSBusRxClass::flags(uint8_t FlagId)
{
  #if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1)
  FlagId = FlagId; /* To avoid a compilation warning */

  return(0);
  #else
  return(!!(sbus_data_frame[22] & FlagId));
  #endif
}

uint8_t SoftSBusRxClass::isSynchro(uint8_t SynchroClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(Synchro & RCUL_CLIENT_MASK(SynchroClientIdx));
  if(Ret)
  {
    Synchro &= ~RCUL_CLIENT_MASK(SynchroClientIdx); /* Clear indicator for the Synchro client */
  }

  return(Ret);
}

/* Rcul support */
uint8_t SoftSBusRxClass::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(isSynchro(ClientIdx));  
}

uint16_t SoftSBusRxClass::RculGetWidth_us(uint8_t Ch)
{
  return(width_us(Ch));
}

void SoftSBusRxClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}

/* static */
inline void SoftSBusRxClass::handle_interrupt()
{
  uint8_t RxByte;
  static uint16_t StartUs = micros16();

  // start bit?
  if(!pinGet(s_pin, s_pinMask) ^ sbusInfo.inv)
  {
    disablePinChangeInterrupts();
    // start bit received    
    RxByte = 0;    
    uint8_t parity = 0xFF;    
    // receive other bits (including parity bit, ignore stop bits)
    for (uint8_t RxBitIdx = 0; RxBitIdx < 9; ++RxBitIdx)
    {
#if defined(DBG_PIN)
      WAIT_TENTH_OF_US(70);// 7.0 us when Debug is enabled
      DIGITAL_TOGGLE(DBG_PIN);
#else
      WAIT_TENTH_OF_US(75);// 7.5 us
#endif
      // sample current bit
      if (pinGet(s_pin, s_pinMask))
        RxByte |= 1 << RxBitIdx; // parity shift here as > 7 bit, so it is just discarded
      else
      {
        parity = ~parity;
        WAIT_TENTH_OF_US(19); // Well balanced
      }
    }
    if(ElapsedUs16Since(StartUs) >= SBUS_SILENCE_3_MS)
    {
      s_RxByteIdx = -1; // Byte received after a silence of at least 3ms -> First Byte
    }
    StartUs = micros16();
    if (!parity)
    {
      // parity check failed!
      s_RxByteIdx = -1;
      goto SBusIsrEnd;
    }
    else
    {
      // parity ok
      if(sbusInfo.inv) RxByte = ~RxByte;
      if(s_RxByteIdx == -1)
      {
        //  check start byte (must be 0x0F)
        if (RxByte == SBUS_HEADER_0x0F) 
        {
          //DIGITAL_TOGGLE(DBG_PIN);
          ++s_RxByteIdx; // bypass this byte: here s_RxByteIdx SHALL be 0
          #if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1)
          sbusInfo.shiftIdx = 0; // Used to track a specific channel
          #endif
        }
        else
        {
          goto SBusIsrEnd;
        }
      }
      else if (s_RxByteIdx == 23)
      {
        if (RxByte == SBUS_TRAILER_0x00) // OK, we got the full SBUS frame
        {
#if defined(DBG_PIN)
          DIGITAL_TOGGLE(DBG_PIN);
#endif
          Synchro     = 0xFF;
          s_RxByteIdx = -1;    // last byte, restart byte index
        }
        else
        {
          s_RxByteIdx = -1;    // error, restart byte index
          goto SBusIsrEnd;
        }
      }
      else
      {
        #if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1)
        if((s_RxByteIdx >= sbusInfo.byteIdx) && ((sbusInfo.shiftIdx) < 3))
        {
          sbus_data_frame[sbusInfo.shiftIdx] = RxByte;
          sbusInfo.shiftIdx++;
        }
        #else
        // store channels and flags
        sbus_data_frame[s_RxByteIdx] = RxByte;
        #endif
        // next byte          
        ++s_RxByteIdx;
      }
    }
  }
SBusIsrEnd:
  // reset pin change interrupt flag
  PCIFR |= s_PCICRMask;
  enablePinChangeInterrupts();
}
