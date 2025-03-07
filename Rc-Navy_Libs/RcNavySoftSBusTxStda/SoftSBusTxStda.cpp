/*
 English: by RC Navy (2023)
 =======
 <SoftSBusTx>: a library to generate RC Normal/Inverted SBUS frame. No hardware inverter needed. All implementation done in software.
 http://p.loussouarn.free.fr
 V1.0 (16/07/2023): initial release

 English: by RC Navy (2023)
 =======
 <SoftSBusTx>: une bibliotheque pour generer des trames SBUS RC Normales/Inversees. Pas besoin d'inverseur materiel. Toute l'implementation est faite par logiciel.
 http://p.loussouarn.free.fr
 V1.0 (16/07/2023): release initiale
*/

#include "SoftSBusTxStda.h"
#include <util/delay.h>

SoftSBusTxClass                   SoftSBusTx = SoftSBusTxClass();

#define SBUS_VAL_MIN              0
#define SBUS_VAL_MAX              2047
#define SBUS_VAL_NEUTRAL          ((SBUS_VAL_MIN + SBUS_VAL_MAX) / 2)

#define SBUS_HEADER_0x0F          0x0F
#define SBUS_TRAILER_0x00         0x00

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

// Some high speed macros to easily manipulate I/O pins
#define PIN_MODE2(Port,  BitInPort, Dir)      (Dir)? ((DDR##Port)  |= _BV(BitInPort)): ((DDR##Port)  &= ~_BV(BitInPort))
#define PIN_MODE(Port_BitInPort, Dir)         PIN_MODE2(Port_BitInPort, Dir)

#define DIGITAL_WRITE2(Port, BitInPort, Val)  (Val)? ((PORT##Port) |= _BV(BitInPort)): ((PORT##Port) &= ~_BV(BitInPort))
#define DIGITAL_WRITE(Port_BitInPort, Val)    DIGITAL_WRITE2(Port_BitInPort, Val)

#define DIGITAL_TOGGLE2(Port, BitInPort)      (PIN##Port) |= _BV(BitInPort)
#define DIGITAL_TOGGLE(Port_BitInPort)        DIGITAL_TOGGLE2(Port_BitInPort)

#define WAIT_TENTH_OF_US(TenthOfUs)           _delay_loop_1(((TenthOfUs) * 8) / 15) // Max(us) = (256 x 3) / 16 = 48us@16MHz (0 is equivalent to 256)

#define TX_DELAY_SBUS_FUTABA_MODE              90/*91*/ //This value has been tuned to obtain exactly 10us (bit time @ 100 000 Bauds)
#define TX_DELAY_SBUS_NON_FUT_MODE             91/*91*/ //This value has been tuned to obtain exactly 10us (bit time @ 100 000 Bauds)

#define TX_DELAY_SBUS_FRAME_MODE              (_FutabaPolar? TX_DELAY_SBUS_FUTABA_MODE: TX_DELAY_SBUS_NON_FUT_MODE)

#define END_TX_RX_BIT_MASK                    (1 << (8 + 1))

/* Constructor */
SoftSBusTxClass::SoftSBusTxClass(void)
{

}

void SoftSBusTxClass::begin(uint8_t TxPeriodMs, uint8_t FutabaPolar /*= 1*/)
{
  _TxPeriodMs   = TxPeriodMs;
  _FutabaPolar  = !!FutabaPolar;
  _StartFrameMs = millis();
  PIN_MODE(SOFT_SBUS_TX_PIN, OUTPUT);
  if(_FutabaPolar) DIGITAL_WRITE(SOFT_SBUS_TX_PIN, LOW);
  else             DIGITAL_WRITE(SOFT_SBUS_TX_PIN, HIGH);
}

/**
* \file   SoftSBusTxStda.cpp
* \fn     rawData(uint8_t ChId, uint16_t RawData)
* \brief  Set the 11 bit value of a SBUS channel given by its id (1 to 16) correspond to a value given in us (880 to 2160)
* \param  ChId:    the channel id (1 to 16)
* \param  RawData: the value in us (880 to 2160)
* \return Void
*/
void SoftSBusTxClass::width_us(uint8_t ChId, uint16_t Width_us)
{
  uint16_t RawData;
  
  if((ChId >= 1) && (ChId <= SOFT_SBUS_TX_CHANNEL_NB))
  {
    Width_us = constrain(Width_us, 880, 2160);
    RawData = map(Width_us, 880, 2160, SBUS_VAL_MIN, SBUS_VAL_MAX);
    RawData = constrain(RawData, 0, SBUS_VAL_MAX);
    _SbusChValue[ChId - 1] = RawData;
  }
}

/**
* \file   SoftSBusTxStda.cpp
* \fn     rawData(uint8_t ChId, uint16_t RawData)
* \brief  Set the 11 bit value of a SBUS channel given by its id (1 to 16)
* \param  ChId:    the channel id (1 to 16)
* \param  RawData: the 11 bit value
* \return Void
*/
void SoftSBusTxClass::rawData(uint8_t ChId, uint16_t RawData)
{
  if((ChId >= 1) && (ChId <= SOFT_SBUS_TX_CHANNEL_NB))
  {
    _SbusChValue[ChId - 1] = RawData;
  }
}

/**
* \file   SoftSBusTxStda.cpp
* \fn     void SbusTxProcess(void)
* \brief  Generate a SBUS frame on the SBUS pin every xx ms
* \param  Void
* \return Void
*/
void SoftSBusTxClass::sendFrame(void)
{
  uint32_t        bits = 0;
  uint8_t         bitsavailable = 0, SbusByte;
  uint16_t        value;

  // Generate SBUS frame
  serialWrite(SBUS_HEADER_0x0F); // Header
  for (uint8_t ChIdx = 0; ChIdx < SOFT_SBUS_TX_CHANNEL_NB; ChIdx++)
  {
    value = _SbusChValue[ChIdx];
    bits |= (uint32_t)value << bitsavailable;
    bitsavailable += 11; // 11 bits per channel
    while (bitsavailable >= 8)
    {
      SbusByte = ((uint8_t) (bits & 0xff));
      serialWrite(SbusByte);
      bits >>= 8;
      bitsavailable -= 8;
    }
  }
  serialWrite(0x00); // Flags
  serialWrite(SBUS_TRAILER_0x00); // Trailer
}

/**
* \file   SoftSBusTxStda.cpp
* \fn     size_t serialWrite(uint8_t b)
* \brief  Write one byte on the SBUS taking polarity into account
* \param  b: The byte to be written
* \return The number of byte written (1)
*/
size_t SoftSBusTxClass::serialWrite(uint8_t b)
{
  uint16_t ParityData  = 0;
  uint8_t  OddParity   = 0;

  /*     in ParityData      */
  /* |<------------------>| */
  /* (Par)(Bit7)...(Bit 0): interrupt are re-enabled after the (Par) bit */
  ParityData = b; // Keep the 8 bits

  /* Compute parity */
  for(uint8_t Idx = 0; Idx < 8; Idx++)
  {
    OddParity ^= (b & 1); /* Result is 0 if bit at 1 are even */
    b >>= 1;
  }

  /* Set parity bit */
  ParityData |= OddParity << 8; /* [Par][B7][B6][B5][B4][B3][B2][B1][B0] */

  /* Apply logic */
  if(_FutabaPolar)
  {
    ParityData = ~ParityData;
  }
  uint8_t oldSREG = SREG;
  cli();  // Turn off interrupts for a clean txmit

  /* Write the start bit according to the logic */
  if(_FutabaPolar) {DIGITAL_WRITE(SOFT_SBUS_TX_PIN, HIGH);} // send 1
  else             {DIGITAL_WRITE(SOFT_SBUS_TX_PIN, LOW);}  // send 0
  WAIT_TENTH_OF_US(TX_DELAY_SBUS_FRAME_MODE);

  /* Write each of the data bits, parity and stop bits */
  for (uint16_t BitMask = 0x01; BitMask != END_TX_RX_BIT_MASK; BitMask <<= 1) // Bits are sent in reverse order (Starting with Bit 0)
  {
    if (ParityData & BitMask) // Choose bit
    {
      DIGITAL_WRITE(SOFT_SBUS_TX_PIN, HIGH); // Send 1
    }
    else
    {
      DIGITAL_WRITE(SOFT_SBUS_TX_PIN, LOW);  // Send 0
    }
    WAIT_TENTH_OF_US(TX_DELAY_SBUS_FRAME_MODE);
  }
  /* Restore pin to natural state according to the logic */
  if(_FutabaPolar) {DIGITAL_WRITE(SOFT_SBUS_TX_PIN, LOW);}  // Send 0
  else             {DIGITAL_WRITE(SOFT_SBUS_TX_PIN, HIGH);} // Send 1
  SREG = oldSREG;               // Turn interrupts back on
  WAIT_TENTH_OF_US(TX_DELAY_SBUS_FRAME_MODE); // First  stop bit
  WAIT_TENTH_OF_US(TX_DELAY_SBUS_FRAME_MODE); // Second stop bit

  return 1;
}

/**
* \file   SoftSBusTxStda.cpp
* \fn     uint8_t SoftSBusTxClass::process(void)
* \brief  Send the SBUS frame every _TxPeriodMs (non blocking, except during the frame)
* \param  void
* \return 0: the frame has not been sent, 1: the frame has just been sent (test this return value if need to synchronize job just after a frame)
*/
uint8_t SoftSBusTxClass::process(void)
{
  uint8_t Ret = 0;
  
  if((millis() - _StartFrameMs) >= (uint32_t)(_TxPeriodMs))
  {
    _StartFrameMs = millis();
    SoftSBusTx.sendFrame();
    Ret = 1;
  }
  
  return(Ret);
}