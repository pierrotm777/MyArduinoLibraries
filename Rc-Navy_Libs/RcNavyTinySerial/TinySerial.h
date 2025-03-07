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

#ifndef TinySerial_h
#define TinySerial_h

#include <inttypes.h>
#include <Stream.h>

#include <TinyPinChange.h>

#define TINY_SOFT_SERIAL_VERSION   0
#define TINY_SOFT_SERIAL_REVISION  3

/******************************************************************************
* Definitions
******************************************************************************/
//#define COMPIL_TIME_BAUD_RATE_AT_16_MHZ      19200 /* Uncomment this line to define the Baud Rate in order to reduce flash footprint */

#define _TS_MAX_RX_BUFF  16//64 // <-- Adjust here the RX buffer size (Shall be a power of 2: 4, 16, 32, 64)

#if (_TS_MAX_RX_BUFF < 4)
#warning TinySerial Rx Buffer size: _TS_MAX_RX_BUFF < 4 !
#endif

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#ifndef SERIAL_8N1
/* As defined in HardwareSerial.h of the regular Arduino core */
#define SERIAL_5N1 0x00
#define SERIAL_6N1 0x02
#define SERIAL_7N1 0x04
#define SERIAL_8N1 0x06
#define SERIAL_5N2 0x08
#define SERIAL_6N2 0x0A
#define SERIAL_7N2 0x0C
#define SERIAL_8N2 0x0E
#define SERIAL_5E1 0x20
#define SERIAL_6E1 0x22
#define SERIAL_7E1 0x24
#define SERIAL_8E1 0x26
#define SERIAL_5E2 0x28
#define SERIAL_6E2 0x2A
#define SERIAL_7E2 0x2C
#define SERIAL_8E2 0x2E
#define SERIAL_5O1 0x30
#define SERIAL_6O1 0x32
#define SERIAL_7O1 0x34
#define SERIAL_8O1 0x36
#define SERIAL_5O2 0x38
#define SERIAL_6O2 0x3A
#define SERIAL_7O2 0x3C
#define SERIAL_8O2 0x3E
#endif

typedef struct{
  uint8_t
          inverse_logic    :1,
          addTo5DataBitNb  :2, // 0 -> 5 data bits, 1 -> 6 data bits, 2 -> 7 data bits, 3 -> 8 data bits
          parity           :2, // 0 -> None,        1 -> Odd,         2 -> Even
          addTo1stopBitNb  :1, // 0 -> 1 stop bit,  1 -> 2 stop bit
          reserved         :2;
}TsInfoSt_t;

class TinySerial : public Stream
{
private:
  // per object data
  volatile uint8_t *_receivePortRegister;
  volatile uint8_t *_transmitPortRegister;
  TsInfoSt_t _tsInfo;
  uint16_t   _endTxRxBitMask;
  uint8_t    _receivePin;
  uint8_t    _receiveBitMask;
  uint8_t    _transmitBitMask;
  uint8_t    _rx_delay_centering;
  uint8_t    _rx_delay_intrabit;
  uint8_t    _tx_delay;

  // static data
  static char _receive_buffer[_TS_MAX_RX_BUFF]; 
  static volatile uint8_t _receive_buffer_tail;
  static volatile uint8_t _receive_buffer_head;
  static TinySerial *active_object;

  // private methods
  void recv();
  uint8_t rx_pin_read();
  void tx_pin_write(uint8_t pin_state);
  void setTX(uint8_t transmitPin);
  void setRX(uint8_t receivePin);

  // private static method for timing
  static inline void tunedDelay(uint16_t delay);

public:
  // public methods
  TinySerial(uint8_t receivePin, uint8_t transmitPin);
  ~TinySerial();
  void begin(uint16_t baudrate, uint8_t config = SERIAL_8N1, uint8_t inverse_logic = false);
  void end();
  int  peek();
  void txMode();
  void rxMode();
  virtual size_t write(uint8_t byte);
  virtual int read();
  virtual int available();
  virtual void flush();
  
  using Print::write;

  // public only for easy access by interrupt handlers
  static void handle_interrupt();
};

#endif
