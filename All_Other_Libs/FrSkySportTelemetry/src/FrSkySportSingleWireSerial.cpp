/*
  FrSky single wire serial class for Teensy LC/3.x/4.x, ESP32, ESP8266, ATmega2560 (Mega) and ATmega328P based boards (e.g. Pro Mini, Nano, Uno)
  (c) Pawelsky 20250412
  Not for commercial use
*/

#include "FrSkySportSingleWireSerial.h"

#if defined(ESP32_HW)
  #include "driver/uart.h"
#endif

FrSkySportSingleWireSerial::FrSkySportSingleWireSerial()
{
  port = NULL;
#if defined(TEENSY_HW)
  uartCtrl = NULL;
#elif defined(ESP32_HW)
  hwSerial = NULL;
  rxPin = -1;
  txPin = -1;
  uartNum = -1;
#else
  softSerial = NULL;
#endif
}

#if defined(TEENSY_HW)
#if defined(__IMXRT1062__)
#define UART_CTRL_TXDIR_FLAG LPUART_CTRL_TXDIR
void FrSkySportSingleWireSerial::initTeensySerial(SerialId id, HardwareSerial *serial, volatile uint32_t *uartCtrl1, volatile uint32_t *uartCtrl2, uint8_t uartCtrl2Daisy)
#else
#define UART_CTRL_TXDIR_FLAG UART_C3_TXDIR
void FrSkySportSingleWireSerial::initTeensySerial(SerialId id, HardwareSerial *serial, volatile uint8_t *uartCtrl1, volatile uint8_t *uartCtrl2)
#endif
{
  port = serial;
  if((id & EXTINV_FLAG) == EXTINV_FLAG) serial->begin(57600); // Start Serial, two wire, no inversion
  else
  {
    serial->begin(57600, SERIAL_8N1_RXINV_TXINV); // Start Serial with RX and TX inverted
    // Put Serial into single wire mode
    uartCtrl = uartCtrl1;
#if defined(__IMXRT1062__)
    *uartCtrl1 |= (LPUART_CTRL_LOOPS | LPUART_CTRL_RSRC);
    if(uartCtrl2 != NULL) *uartCtrl2 = uartCtrl2Daisy;
#else
    *uartCtrl2 |= (UART_C1_LOOPS | UART_C1_RSRC);
#endif
  }
}
#elif defined(ESP32_HW)
void FrSkySportSingleWireSerial::initEsp32Serial(SerialId id, HardwareSerial *serial, int8_t rx, int8_t tx)
{
  serialId = id;
  port = serial;
  hwSerial = serial;
  rxPin = rx;
  txPin = tx;

  // Arduino ESP32: Serial1 = UART1, Serial2 = UART2.
  // Keep the UART opened permanently. Do NOT call end()/begin() inside setMode():
  // S.Port timing is too short for a full UART restart between RX and TX.
  if((id == SERIAL_1_S3) || (id == SERIAL_1_S3_EXTINV)) uartNum = 1;
  else if((id == SERIAL_2_S3) || (id == SERIAL_2_S3_EXTINV)) uartNum = 2;
  else uartNum = -1;

  const bool invert = ((serialId & EXTINV_FLAG) != EXTINV_FLAG);

  // Normal FrSky S.Port is inverted at 57600 bauds.
  // EXTINV means: external inverter used, so UART itself is not inverted.
  hwSerial->begin(57600, SERIAL_8N1, rxPin, txPin, invert);

  // Force plain UART mode. Direction is handled by the GPIO matrix below.
  if(uartNum >= 0)
  {
    uart_set_mode((uart_port_t)uartNum, UART_MODE_UART);
    uart_set_pin((uart_port_t)uartNum,
                 txPin >= 0 ? txPin : UART_PIN_NO_CHANGE,
                 rxPin >= 0 ? rxPin : UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);
  }

  setMode(RX);
}

#else 
void FrSkySportSingleWireSerial::initTwoWireSerial(SerialId id, HardwareSerial *serial)
{
  serialId = id;
  port = serial;
  serial->begin(57600); // Start Serial, two wire, no inversion
}

void FrSkySportSingleWireSerial::initSoftSerial(SerialId id)
{
  serialId = id;
  if(softSerial != NULL) { delete softSerial; softSerial = NULL; }
  softSerial = new SoftwareSerial(serialId, serialId, true);
  port = softSerial;
  softSerial->begin(57600);
}
#endif

void FrSkySportSingleWireSerial::begin(SerialId id)
{
#if defined(TEENSY_HW)
  if(id == SERIAL_USB) // Added for debug purposes via USB
  {
    port = &Serial;
    Serial.begin(57600);
  }
#if defined(__IMXRT1062__)
  else if((id == SERIAL_1) || (id == SERIAL_1_EXTINV)) initTeensySerial(id, &Serial1, &LPUART6_CTRL, &IOMUXC_LPUART6_TX_SELECT_INPUT, 1);
  else if((id == SERIAL_2) || (id == SERIAL_2_EXTINV)) initTeensySerial(id, &Serial2, &LPUART4_CTRL, &IOMUXC_LPUART4_TX_SELECT_INPUT, 2);
  else if((id == SERIAL_3) || (id == SERIAL_3_EXTINV)) initTeensySerial(id, &Serial3, &LPUART2_CTRL, &IOMUXC_LPUART2_TX_SELECT_INPUT, 1);
  else if((id == SERIAL_4) || (id == SERIAL_4_EXTINV)) initTeensySerial(id, &Serial4, &LPUART3_CTRL, &IOMUXC_LPUART3_TX_SELECT_INPUT, 0);
  else if((id == SERIAL_5) || (id == SERIAL_5_EXTINV)) initTeensySerial(id, &Serial5, &LPUART8_CTRL, &IOMUXC_LPUART8_TX_SELECT_INPUT, 1);
  else if((id == SERIAL_6) || (id == SERIAL_6_EXTINV)) initTeensySerial(id, &Serial6, &LPUART1_CTRL, NULL, 0);
  else if((id == SERIAL_7) || (id == SERIAL_7_EXTINV)) initTeensySerial(id, &Serial7, &LPUART7_CTRL, &IOMUXC_LPUART7_TX_SELECT_INPUT, 1);
#if defined(ARDUINO_TEENSY41)  // Although Teensy 4.0 and 4.1 use the same IMXRT1062 chip Serial8 is not broken out on teensy 4.0 board
  else if((id == SERIAL_8) || (id == SERIAL_8_EXTINV)) initTeensySerial(id, &Serial8, &LPUART5_CTRL, &IOMUXC_LPUART5_TX_SELECT_INPUT, 1);
#endif
#else
  else if((id == SERIAL_1) || (id == SERIAL_1_EXTINV)) initTeensySerial(id, &Serial1, &UART0_C3, &UART0_C1);
  else if((id == SERIAL_2) || (id == SERIAL_2_EXTINV)) initTeensySerial(id, &Serial2, &UART1_C3, &UART1_C1);
  else if((id == SERIAL_3) || (id == SERIAL_3_EXTINV)) initTeensySerial(id, &Serial3, &UART2_C3, &UART2_C1);
#if defined(__MK66FX1M0__) || defined(__MK64FX512__)
  else if((id == SERIAL_4) || (id == SERIAL_4_EXTINV)) initTeensySerial(id, &Serial4, &UART3_C3, &UART3_C1);
  else if((id == SERIAL_5) || (id == SERIAL_5_EXTINV)) initTeensySerial(id, &Serial5, &UART4_C3, &UART4_C1);
  else if((id == SERIAL_6) || (id == SERIAL_6_EXTINV)) initTeensySerial(id, &Serial6, &UART5_C3, &UART5_C1);
#endif
#endif
#elif defined(ESP32_HW)
  // ESP32-S3 needs explicit pins. Use begin(id, rxPin, txPin).
  // This fallback uses Arduino core default pins if available.
  if((id == SERIAL_1_S3) || (id == SERIAL_1_S3_EXTINV)) initEsp32Serial(id, &Serial1, -1, -1);
  else if((id == SERIAL_2_S3) || (id == SERIAL_2_S3_EXTINV)) initEsp32Serial(id, &Serial2, -1, -1);
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__) || defined(ESP8266)
  if(id == SERIAL_EXTINV) initTwoWireSerial(id, &Serial);
#if defined(__AVR_ATmega2560__)
  else if(id == SERIAL_1_EXTINV) initTwoWireSerial(id, &Serial1);
  else if(id == SERIAL_2_EXTINV) initTwoWireSerial(id, &Serial2);
  else if(id == SERIAL_3_EXTINV) initTwoWireSerial(id, &Serial3);
#endif
  else initSoftSerial(id); 
#else
  #error "Unsupported processor! Only Teensy LC/3.x/4.x, ESP32/ESP32-S3, ESP8266, ATmega2560 and ATmega328P based boards are supported.";
#endif
  crc = 0;
  setMode(RX);
}


#if defined(ESP32_HW)
void FrSkySportSingleWireSerial::begin(SerialId id, int8_t rx, int8_t tx)
{
  if((id == SERIAL_1_S3) || (id == SERIAL_1_S3_EXTINV)) initEsp32Serial(id, &Serial1, rx, tx);
  else if((id == SERIAL_2_S3) || (id == SERIAL_2_S3_EXTINV)) initEsp32Serial(id, &Serial2, rx, tx);

  crc = 0;
  setMode(RX);
}

void FrSkySportSingleWireSerial::begin(SerialId id, int8_t sportPin)
{
  begin(id, sportPin, sportPin);
}
#endif

void FrSkySportSingleWireSerial::setMode(SerialMode mode)
{
#if defined(TEENSY_HW)
  if((port != NULL) && (uartCtrl != NULL))
  {
    if(mode == TX) *uartCtrl |= UART_CTRL_TXDIR_FLAG;
    else if(mode == RX) *uartCtrl &= ~UART_CTRL_TXDIR_FLAG;
  }
#elif defined(ESP32_HW)
  if((hwSerial != NULL) && (uartNum >= 0))
  {
    // ESP32/S3 S.Port handling:
    // - UART stays alive permanently.
    // - In RX mode, the S.Port GPIO is released as input.
    // - In TX mode, the same GPIO, or txPin in 2-wire/diode mode, is connected to UART TX.
    // This mimics the fast Teensy TXDIR register switch without restarting the UART.

    const bool oneWire = (rxPin >= 0) && (txPin >= 0) && (rxPin == txPin);

    if(mode == TX)
    {
      // Drop any received echo/pending byte before sending our answer.
      while(hwSerial->available()) hwSerial->read();

      if(txPin >= 0)
      {
        pinMode(txPin, OUTPUT);
        uart_set_pin((uart_port_t)uartNum,
                     txPin,
                     oneWire ? rxPin : (rxPin >= 0 ? rxPin : UART_PIN_NO_CHANGE),
                     UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
      }
    }
    else // RX
    {
      // Wait until TX shift register is empty before releasing the line.
      hwSerial->flush();

      if(txPin >= 0)
      {
        pinMode(txPin, INPUT); // release S.Port bus
      }

      if(rxPin >= 0)
      {
        pinMode(rxPin, INPUT);
        uart_set_pin((uart_port_t)uartNum,
                     oneWire ? UART_PIN_NO_CHANGE : (txPin >= 0 ? txPin : UART_PIN_NO_CHANGE),
                     rxPin,
                     UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
      }

      // Remove our own echo if RX was connected during TX.
      while(hwSerial->available()) hwSerial->read();
    }
  }
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__) || defined(ESP8266)
  if((port != NULL) && ((serialId & EXTINV_FLAG) != EXTINV_FLAG))
  {
    if(mode == TX) pinMode(serialId, OUTPUT);
    else if(mode == RX) pinMode(serialId, INPUT);
  }
#else
  #error "Unsupported processor! Only Teensy LC/3.x/4.x, ESP32/ESP32-S3, ESP8266, ATmega2560 and ATmega328P based boards are supported.";
#endif
}

void FrSkySportSingleWireSerial::sendHeader(uint8_t id)
{
  if(port != NULL)
  {
    setMode(TX);
    port->write(FRSKY_TELEMETRY_START_FRAME);
    port->write(id);
    port->flush();
    setMode(RX);
  }
}

void FrSkySportSingleWireSerial::sendByte(uint8_t byte)
{
  if(port != NULL)
  {
    if(byte == 0x7E)
    {
      port->write(FRSKY_STUFFING);
      port->write(0x5E); // 0x7E xor 0x20
    }
    else if(byte == 0x7D)
    {
      port->write(FRSKY_STUFFING);
      port->write(0x5D); // 0x7D xor 0x20
    }
    else
    {
      port->write(byte);
    }
    crc += byte;
    crc += crc >> 8; crc &= 0x00ff;
  }
}

void FrSkySportSingleWireSerial::sendCrc()
{
  // Send and reset CRC
  sendByte(0xFF - crc);
  crc = 0;
}

void FrSkySportSingleWireSerial::sendFrame(uint16_t dataTypeId, uint8_t frameType, uint32_t data)
{
  if(port != NULL)
  {
    setMode(TX);
    sendByte(frameType);
    uint8_t *bytes = (uint8_t*)&dataTypeId;
    sendByte(bytes[0]);
    sendByte(bytes[1]);
    bytes = (uint8_t*)&data;
    sendByte(bytes[0]);
    sendByte(bytes[1]);
    sendByte(bytes[2]);
    sendByte(bytes[3]);
    sendCrc();
    port->flush();
    setMode(RX);
  }
}

void FrSkySportSingleWireSerial::sendData(uint16_t dataTypeId, uint32_t data)
{
  sendFrame(dataTypeId, FRSKY_SENSOR_DATA_FRAME, data);
}

void FrSkySportSingleWireSerial::sendEmpty(uint16_t dataTypeId)
{
  sendFrame(dataTypeId, FRSKY_SENSOR_EMPTY_FRAME, 0x00000000);
}
