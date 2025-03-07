/*
<TinySerial> library is exactly the same as the <SoftwareSerial> library but:
- It is used with the <TinyPinChange> library which allows to share the Pin Change Interrupt Vector
- It supports from 5 to 8 data bit
- It supports from 1 to 2 stop bit
- It supports the following parities: None, Odd, Even
- It does NOT support multi-instances (almost never used)
- It supports data rate from 9600 to 57600 bauds at 16 MHz
RC Navy (2012-2020): http://p.loussouarn.free.fr

 V0.2 (31/12/2020): Initial release
 V0.3 (12/03/2023): Tx bit time too short. _tx_delay is now _rx_delay_intrabit + 3 rather than _rx_delay_intrabit + 2

*/

#define PIN_MODE2(Port,  BitInPort, Dir)     (Dir)? ((DDR##Port)  |= _BV(BitInPort)): ((DDR##Port)  &= ~_BV(BitInPort))
#define PIN_MODE(Port_BitInPort, Dir)         PIN_MODE2(Port_BitInPort, Dir)

#define DIGITAL_WRITE2(Port, BitInPort, Val)  (Val)? ((PORT##Port) |= _BV(BitInPort)): ((PORT##Port) &= ~_BV(BitInPort))
#define DIGITAL_WRITE(Port_BitInPort, Val)    DIGITAL_WRITE2(Port_BitInPort, Val)

#define DBG_PIN C, 5

// When set, TS_DEBUG co-opts DBG_PIN pin for debugging with an
// oscilloscope or logic analyzer.  Beware: it also slightly modifies
// the bit times, so don't rely on it too much at high baud rates
//#define TS_DEBUG

#ifdef TS_DEBUG
#define TOGGLE_PIN2(Port, BitInPort)           ((PIN##Port) |= _BV(BitInPort))
#define TOGGLE_PIN(Port_BitInPort)             TOGGLE_PIN2(Port_BitInPort)
#else
#define TOGGLE_PIN(Port_BitInPort)
#endif

// 
// Includes
// 
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "TinySerial.h"

#if !defined(COMPIL_TIME_BAUD_RATE_AT_16_MHZ)
//
// Lookup table
//
typedef struct _DELAY_TABLE
{
  uint16_t baud;
  uint8_t  rx_delay_centering;
  uint8_t  rx_delay_intrabit;
} DELAY_TABLE;

/* 02/01/2021                                                              */
/* Note: timing have been adjusted only with F_CPU = 16000000              */
/*       Use TS_DEBUG to adjust the value with the help of an Oscilloscope */
/*       for the other CPU clocks                                          */

#if F_CPU == 16000000
static const DELAY_TABLE PROGMEM table[] = 
{
  //  baud    rxcenter   rxintra
  { 57600,    3,         29     },
  { 38400,    12,        50     },
  { 19200,    44,        113    },
  { 9600,     112,       232    },
};
const int XMIT_START_ADJUSTMENT = 0;
#elif F_CPU == 16500000

static const DELAY_TABLE PROGMEM table[] = 
{
  //  baud    rxcenter   rxintra
  { 57600,    3,         35     },
  { 38400,    12,        56     },
  { 19200,    52,        118    },
  { 9600,     118,       241    },
};

const int XMIT_START_ADJUSTMENT = 0;
#elif F_CPU == 8000000

static const DELAY_TABLE table[] PROGMEM = 
{
  //  baud    rxcenter    rxintra
  { 57600,    1,          15     },
  { 38400,    2,          25     },
  { 19200,    20,         55     },
  { 9600,     50,         114    },
  { 4800,     110,        233    },
};

const int XMIT_START_ADJUSTMENT = 4;

#elif F_CPU == 20000000

// 20MHz support courtesy of the good people at macegr.com.
// Thanks, Garrett!

static const DELAY_TABLE PROGMEM table[] =
{
  //  baud    rxcenter    rxintra
  { 57600,    20,         43     },
  { 38400,    37,         73     },
  { 19200,    71,         148    },
};

const int XMIT_START_ADJUSTMENT = 6;

#else
#error This version of TinySerial supports only 20 MHz, 16.5 MHz, 16 MHz and 8 MHz processors
#endif

#else
const int XMIT_START_ADJUSTMENT = 0; /* For 16 MHz */
#endif

//
// Statics
//
TinySerial *TinySerial::active_object = 0;
char TinySerial::_receive_buffer[_TS_MAX_RX_BUFF]; 
volatile uint8_t TinySerial::_receive_buffer_tail = 0;
volatile uint8_t TinySerial::_receive_buffer_head = 0;

//
// Constructor
//
TinySerial::TinySerial(uint8_t receivePin, uint8_t transmitPin)
{
  setRX(receivePin);
  setTX(transmitPin);
  TinyPinChange_RegisterIsr(receivePin, TinySerial::handle_interrupt);
  active_object = this;
}

//
// Destructor
//
TinySerial::~TinySerial()
{
    TinyPinChange_UnregisterIsr(_receivePin, TinySerial::handle_interrupt);
    end();
}

//
// Public methods
//
void TinySerial::begin(uint16_t baudrate, uint8_t config /* = SERIAL_8N1 */, uint8_t inverse_logic /* = false */)
{
#if defined(COMPIL_TIME_BAUD_RATE_AT_16_MHZ)
  baudrate = baudrate; /* To avoid a compilation warning */
#warning Baud Rate is defined at compilation time (see: COMPIL_TIME_BAUD_RATE_AT_16_MHZ in TinySerial.h)
#endif
  _tsInfo.addTo5DataBitNb = ((config & B00000110) >> 1); // 0 to 3
  _tsInfo.parity          = ((config & B00110000) >> 4); // Here:    0 -> None, 2 -> Even, 3 -> Odd
  if(_tsInfo.parity == 3) _tsInfo.parity = 1;            // Finally: 0 -> None, 1 -> Odd,  2 -> Even
  _tsInfo.addTo1stopBitNb = ((config & B00001000) >> 3); // 0 or 1
  _tsInfo.inverse_logic  = inverse_logic;
  _endTxRxBitMask        = 1 << (5 + _tsInfo.addTo5DataBitNb + !!_tsInfo.parity); //Normal is 8 data bit
#if !defined(COMPIL_TIME_BAUD_RATE_AT_16_MHZ)
  /* Use the table (more flexible, but use more flash memory) */
  for (uint8_t i = 0; i < (sizeof(table) / sizeof(table[0])); ++i)
  {
    uint16_t baud = pgm_read_word(&table[i].baud);
    if (baud == baudrate)
    {
      _rx_delay_centering = pgm_read_byte(&table[i].rx_delay_centering);
      _rx_delay_intrabit  = pgm_read_byte(&table[i].rx_delay_intrabit);
      _tx_delay           = _rx_delay_intrabit + 3;
      break;
    }
  }
#elif (COMPIL_TIME_BAUD_RATE_AT_16_MHZ == 9600)
    _rx_delay_centering   = 112;
    _rx_delay_intrabit    = 232;
    _tx_delay             = _rx_delay_intrabit + 3;
#elif (COMPIL_TIME_BAUD_RATE_AT_16_MHZ == 19200)
    _rx_delay_centering   = 44;
    _rx_delay_intrabit    = 113;
    _tx_delay             = _rx_delay_intrabit + 3;
#elif (COMPIL_TIME_BAUD_RATE_AT_16_MHZ == 38400)
    _rx_delay_centering   = 12;
    _rx_delay_intrabit    = 50;
    _tx_delay             = _rx_delay_intrabit + 3;
#elif (COMPIL_TIME_BAUD_RATE_AT_16_MHZ == 57600)
    _rx_delay_centering   = 3;
    _rx_delay_intrabit    = 29;
    _tx_delay             = _rx_delay_intrabit + 3;
#else

#endif
    
#if 0
  _tx_delay             = F_CPU / (7 * baudrate) - 6;
  _rx_delay_stopbit     = _tx_delay + 3;
  _rx_delay_intrabit    = _rx_delay_stopbit;
  _rx_delay_centering   = _tx_delay/2 - 5;
#endif
#if 0
Serial.print("_tx_delay=");Serial.println(_tx_delay);
Serial.print("_rx_delay_centering=");Serial.println(_rx_delay_centering);
Serial.print("_rx_delay_intrabit=");Serial.println(_rx_delay_intrabit);
Serial.print("_rx_delay_stopbit=");Serial.println(_rx_delay_intrabit);
#endif
  // Set up RX interrupts, but only if we have a valid RX baud rate
  if (digitalPinToPCICR(_receivePin))
  {
    *digitalPinToPCICR(_receivePin) |= _BV(digitalPinToPCICRbit(_receivePin));
    *digitalPinToPCMSK(_receivePin) |= _BV(digitalPinToPCMSKbit(_receivePin));
  }
  tunedDelay(_tx_delay); // if we were low this establishes the end

#ifdef TS_DEBUG
  PIN_MODE(DBG_PIN, OUTPUT);
#endif

}

void TinySerial::end()
{
  if (digitalPinToPCMSK(_receivePin))
    *digitalPinToPCMSK(_receivePin) &= ~_BV(digitalPinToPCMSKbit(_receivePin));
}

void TinySerial::setTX(uint8_t tx)
{
  _transmitBitMask = digitalPinToBitMask(tx);
  if(_transmitBitMask != _receiveBitMask)
  {
    pinMode(tx, OUTPUT);
    digitalWrite(tx, HIGH);
  }
  uint8_t port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputRegister(port);
}

void TinySerial::setRX(uint8_t rx)
{
  pinMode(rx, INPUT);
  if (!_tsInfo.inverse_logic)
    digitalWrite(rx, HIGH);  // pullup for normal logic!
  _receivePin = rx;
  _receiveBitMask = digitalPinToBitMask(rx);
  uint8_t port = digitalPinToPort(rx);
  _receivePortRegister = portInputRegister(port);
}

// Read data from buffer
int TinySerial::read()
{
  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  uint8_t d = _receive_buffer[_receive_buffer_head]; // grab next byte
  _receive_buffer_head = (_receive_buffer_head + 1) % _TS_MAX_RX_BUFF;
  return d;
}

int TinySerial::available()
{
  return (_receive_buffer_tail + _TS_MAX_RX_BUFF - _receive_buffer_head) % _TS_MAX_RX_BUFF;
}

size_t TinySerial::write(uint8_t b)
{
  uint16_t dataParStop  = 0;
  uint8_t  OddParity    = 0;

  /*     in dataParStop      */
  /* |<------------------->| */
  /* (Bit0)...(Bit n-1)[Par](Stop)[Stop]: interrupt are re-enabled after the [Par] bit (if present) */
  dataParStop = b & (uint8_t)((1 << (5 + _tsInfo.addTo5DataBitNb)) - 1); // Keep only the right number of bits
  if(_tsInfo.parity)
  {
    for(uint8_t Idx = 0; Idx < (5 + _tsInfo.addTo5DataBitNb); Idx++)
    {
      OddParity ^= (b & 1); /* Result is 0 if bit at 1 are even */
      b >>= 1;
    }
    /* Set parity bit */
    dataParStop |= (OddParity ^ (_tsInfo.parity == 1)) << (5 + _tsInfo.addTo5DataBitNb);
  }
  /* Apply logic */
  if (_tsInfo.inverse_logic)
  {
    dataParStop = ~dataParStop;
  }
  uint8_t oldSREG = SREG;
  cli();  // turn off interrupts for a clean txmit

  // Write the start bit according to the logic
  tx_pin_write(_tsInfo.inverse_logic);
  tunedDelay(_tx_delay + XMIT_START_ADJUSTMENT);

  // Write each of the data parity and stop bits
  for (uint16_t mask = 0x01; mask != _endTxRxBitMask; mask <<= 1)
  {
    if (dataParStop & mask) // choose bit
      tx_pin_write(HIGH);   // send 1
    else
      tx_pin_write(LOW);    // send 0
  
    tunedDelay(_tx_delay);
  }
  tx_pin_write(!_tsInfo.inverse_logic); // restore pin to natural state according to the logic
  SREG = oldSREG;                       // turn interrupts back on
  tunedDelay(_tx_delay);                // First stop bit
  if(_tsInfo.addTo1stopBitNb)
  {
    tunedDelay(_tx_delay);              // Second stop bit (if present)
  }
  return 1;
}

void TinySerial::flush()
{
  uint8_t oldSREG = SREG;
  cli();
  _receive_buffer_head = _receive_buffer_tail = 0;
  SREG = oldSREG;
}

int TinySerial::peek()
{
  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  return _receive_buffer[_receive_buffer_head];
}

/* RC Navy: hack to use SofSerial as single wire bidirectional serial port */
void TinySerial::txMode()
{
  /* Disable Pin Change Interrupt capabilities for this pin */
  TinyPinChange_DisablePin(_receivePin);
  /* Switch Pin to Output */
  pinMode(_receivePin, OUTPUT);
  digitalWrite(_receivePin, _tsInfo.inverse_logic?LOW:HIGH);
}

void TinySerial::rxMode()
{
  /* Enable Pin Change Interrupt capabilities for this pin */
  TinyPinChange_EnablePin(_receivePin);
  /* Switch Pin to Input */
  pinMode(_receivePin, INPUT);
  if (!_tsInfo.inverse_logic)
    digitalWrite(_receivePin, HIGH);  // pullup for normal logic!
}

//
// Private methods
//

/* static */ 
inline void TinySerial::tunedDelay(uint16_t delay) { 
  uint8_t tmp=0;

  asm volatile("sbiw    %0, 0x01 \n\t"
    "ldi %1, 0xFF \n\t"
    "cpi %A0, 0xFF \n\t"
    "cpc %B0, %1 \n\t"
    "brne .-10 \n\t"
    : "+r" (delay), "+a" (tmp)
    : "0" (delay)
    );
}

//
// The receive routine called by the interrupt handler
//
void TinySerial::recv()
{
  uint8_t  RxByte = 0, NotRxBitMask;

  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  if (!rx_pin_read() ^ _tsInfo.inverse_logic)
  {
    TinyPinChange_DisablePin(_receivePin);
    tunedDelay(_rx_delay_centering + _rx_delay_intrabit); // After this delay, we are in the center of the first data bit
    // Read each of the 8 bits
    for (uint16_t RxBitMask = 0x1; RxBitMask != (_endTxRxBitMask >> (!!_tsInfo.parity)); RxBitMask <<= 1)
    {
      TOGGLE_PIN(DBG_PIN);
      NotRxBitMask = ~RxBitMask;
      if (rx_pin_read())
      {
        RxByte |= RxBitMask;   // parity shift here as > 7 bit, so it is just discarded
      }
      else // else clause added to ensure function timing is ~balanced
      {
        RxByte &= NotRxBitMask;// parity shift here as >7 bit, so it is just discarded
      }
      tunedDelay(_rx_delay_intrabit);
    }
    if(_tsInfo.parity)
    {
      TOGGLE_PIN(DBG_PIN);
      tunedDelay(_rx_delay_intrabit); // Parity is not checked
    }
    // Stop bit
    TOGGLE_PIN(DBG_PIN);
    tunedDelay(_rx_delay_intrabit);
    TOGGLE_PIN(DBG_PIN);
    // No need to wait for 2 Stop bit (if any)
    TinyPinChange_EnablePin(_receivePin);
    if(_tsInfo.inverse_logic)
    {
      RxByte  = ~RxByte;
      RxByte &= ((1 << (5 + _tsInfo.addTo5DataBitNb)) - 1); // Keep only data bits
    }

    // if buffer full, return
    if ((_receive_buffer_tail + 1) % _TS_MAX_RX_BUFF != _receive_buffer_head) 
    {
      // save new data in buffer: tail points to where byte goes
      _receive_buffer[_receive_buffer_tail] = RxByte; // save new byte
      _receive_buffer_tail = (_receive_buffer_tail + 1) % _TS_MAX_RX_BUFF;
    } 
  }
}

void TinySerial::tx_pin_write(uint8_t pin_state)
{
  if (pin_state == LOW)
    *_transmitPortRegister &= ~_transmitBitMask;
  else
    *_transmitPortRegister |= _transmitBitMask;
}

uint8_t TinySerial::rx_pin_read()
{
  return *_receivePortRegister & _receiveBitMask;
}

//
// Interrupt handling
//

/* static */
inline void TinySerial::handle_interrupt()
{
    active_object->recv();
}
