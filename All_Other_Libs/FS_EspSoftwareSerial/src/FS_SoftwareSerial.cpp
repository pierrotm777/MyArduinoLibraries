
// Some changes for ESP32C3   Based on version=6.16.1
/*

  SoftwareSerial.cpp - Implementation of the Arduino software serial for ESP8266/ESP32.
  Copyright (c) 2015-2016 Peter Lerup. All rights reserved.
  Copyright (c) 2018-2019 Dirk O. Kaar. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "FS_SoftwareSerial.h"
#include <Arduino.h>

#ifndef ESP32
uint32_t SoftwareSerial::m_savedPS = 0;
#else
portMUX_TYPE SoftwareSerial::m_interruptsMux = portMUX_INITIALIZER_UNLOCKED;
#endif

inline void IRAM_ATTR SoftwareSerial::disableInterrupts()
{
#ifndef ESP32
  m_savedPS = xt_rsil(15);
#else
  taskENTER_CRITICAL(&m_interruptsMux);
#endif
}

inline void IRAM_ATTR SoftwareSerial::restoreInterrupts()
{
#ifndef ESP32
  xt_wsr_ps(m_savedPS);
#else
  taskEXIT_CRITICAL(&m_interruptsMux);
#endif
}

constexpr uint8_t BYTE_ALL_BITS_SET = ~static_cast<uint8_t>(0);

SoftwareSerial::SoftwareSerial() {
  m_isrOverflow = false;
  m_rxGPIOPullupEnabled = true;
}

SoftwareSerial::SoftwareSerial(int8_t rxPin, int8_t txPin, bool invert)
{
  m_isrOverflow = false;
  m_rxGPIOPullupEnabled = true;
  m_rxPin = rxPin;
  m_txPin = txPin;
  m_invert = invert;
}

SoftwareSerial::~SoftwareSerial() {
  end();
}

bool SoftwareSerial::isValidGPIOpin(int8_t pin) {
#if defined(ESP8266)
  return (pin >= 0 && pin <= 16) && !isFlashInterfacePin(pin);
#elif defined(ESP32)
  // Remove the strapping pins as defined in the datasheets, they affect bootup and other critical operations
  // Remmove the flash memory pins on related devices, since using these causes memory access issues.
#ifdef CONFIG_IDF_TARGET_ESP32
  // Datasheet https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf,
  // Pinout    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/_images/esp32-devkitC-v4-pinout.jpg
  return (pin == 1) || (pin >= 3 && pin <= 5) ||
         (pin >= 12 && pin <= 15) ||
         (!psramFound() && pin >= 16 && pin <= 17) ||
         (pin >= 18 && pin <= 19) ||
         (pin >= 21 && pin <= 23) || (pin >= 25 && pin <= 27) || (pin >= 32 && pin <= 39);
#elif CONFIG_IDF_TARGET_ESP32S2
  // Datasheet https://www.espressif.com/sites/default/files/documentation/esp32-s2_datasheet_en.pdf,
  // Pinout    https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/_images/esp32-s2_saola1-pinout.jpg
  return (pin >= 1 && pin <= 21) || (pin >= 33 && pin <= 44);
#elif CONFIG_IDF_TARGET_ESP32C3
  // Datasheet https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf,
  // Pinout    https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/_images/esp32-c3-devkitm-1-v1-pinout.jpg
  return (pin >= 0 && pin <= 1) || (pin >= 3 && pin <= 7) || (pin >= 18 && pin <= 21);
#else
  return pin >= 0;
#endif
#else
  return pin >= 0;
#endif
}

bool SoftwareSerial::isValidRxGPIOpin(int8_t pin) {
  return isValidGPIOpin(pin)
#if defined(ESP8266)
         && (pin != 16)
#endif
         ;
}

bool SoftwareSerial::isValidTxGPIOpin(int8_t pin) {
  return isValidGPIOpin(pin)
#if defined(ESP32)
#ifdef CONFIG_IDF_TARGET_ESP32
         && (pin < 34)
#elif CONFIG_IDF_TARGET_ESP32S2
         && (pin <= 45)
#elif CONFIG_IDF_TARGET_ESP32C3
         // no restrictions
#endif
#endif
         ;
}

bool SoftwareSerial::hasRxGPIOPullUp(int8_t pin) {
#if defined(ESP32)
  return !(pin >= 34 && pin <= 39);
#else
  (void)pin;
  return true;
#endif
}

void SoftwareSerial::setRxGPIOPullUp() {
  if (m_rxValid) {
    pinMode(m_rxPin, hasRxGPIOPullUp(m_rxPin) && m_rxGPIOPullupEnabled ? INPUT_PULLUP : INPUT);
  }
}

void SoftwareSerial::begin(uint32_t baud, SoftwareSerialConfig config,
                           int8_t rxPin, int8_t txPin,
                           bool invert, int bufCapacity, int isrBufCapacity) {
  if (-1 != rxPin) m_rxPin = rxPin;
  if (-1 != txPin) m_txPin = txPin;
  m_oneWire = (m_rxPin == m_txPin);
  m_invert = invert;
  m_dataBits = 5 + (config & 07);
  m_parityMode = static_cast<SoftwareSerialParity>(config & 070);
  m_stopBits = 1 + ((config & 0300) ? 1 : 0);
  m_pduBits = m_dataBits + static_cast<bool>(m_parityMode) + m_stopBits;
  m_bitCyclesWrite = (ESP.getCpuFreqMHz() * 1000000UL + baud / 2) / baud;  // FS +++   usefull for Write.
  // m_bitCycles  is for read (in ISR)
#if defined (CONFIG_IDF_TARGET_ESP32C3)
//  m_bitCycles = 1000000UL / baud; //   FS +++++++++++++++++++++++++++++++++++++++ durée en micro sec
//    m_bitCycles = (1000000UL / baud)*2; //   FS ++++++++++++++ durée en micro sec si on stocke 2*micros() dans ISR
    m_bitCycles=  uint32_t((1000000UL / float(baud))+0.5)*2;  // FS  pour le fun ... change uniquement pour 115200bds.
#else
  m_bitCycles = (ESP.getCpuFreqMHz() * 1000000UL + baud / 2) / baud;
#endif
  m_intTxEnabled = true;
  if (isValidRxGPIOpin(m_rxPin)) {
    m_rxReg = portInputRegister(digitalPinToPort(m_rxPin));
    m_rxBitMask = digitalPinToBitMask(m_rxPin);
    m_buffer.reset(new circular_queue<uint8_t>((bufCapacity > 0) ? bufCapacity : 64));
    if (m_parityMode)
    {
      m_parityBuffer.reset(new circular_queue<uint8_t>((m_buffer->capacity() + 7) / 8));
      m_parityInPos = m_parityOutPos = 1;
    }
    m_isrBuffer.reset(new circular_queue<uint32_t, SoftwareSerial*>((isrBufCapacity > 0) ?
                      isrBufCapacity : m_buffer->capacity() * (2 + m_dataBits + static_cast<bool>(m_parityMode))));
    if (m_buffer && (!m_parityMode || m_parityBuffer) && m_isrBuffer) {
      m_rxValid = true;
      setRxGPIOPullUp();
    }
  }
  if (isValidTxGPIOpin(m_txPin)) {
#if !defined(ESP8266)
    m_txReg = portOutputRegister(digitalPinToPort(m_txPin));
#endif
    m_txBitMask = digitalPinToBitMask(m_txPin);
    m_txValid = true;
    if (!m_oneWire) {
      pinMode(m_txPin, OUTPUT);
      digitalWrite(m_txPin, !m_invert);
    }
  }
  if (!m_rxEnabled) {
    enableRx(true);
  }
}

void SoftwareSerial::end()
{
  enableRx(false);
  m_txValid = false;
  if (m_buffer) {
    m_buffer.reset();
  }
  m_parityBuffer.reset();
  if (m_isrBuffer) {
    m_isrBuffer.reset();
  }
}

uint32_t SoftwareSerial::baudRate() {
  return ESP.getCpuFreqMHz() * 1000000UL / m_bitCyclesWrite;
}

void SoftwareSerial::setTransmitEnablePin(int8_t txEnablePin) {
  if (isValidTxGPIOpin(txEnablePin)) {
    m_txEnableValid = true;
    m_txEnablePin = txEnablePin;
    pinMode(m_txEnablePin, OUTPUT);
    digitalWrite(m_txEnablePin, LOW);
  }
  else {
    m_txEnableValid = false;
  }
}

void SoftwareSerial::enableIntTx(bool on) {
  m_intTxEnabled = on;
}

void SoftwareSerial::enableRxGPIOPullup(bool on) {
  m_rxGPIOPullupEnabled = on;
  setRxGPIOPullUp();
}

void SoftwareSerial::enableTx(bool on) {
  if (m_txValid && m_oneWire) {
    if (on) {
      enableRx(false);
      pinMode(m_txPin, OUTPUT);
      digitalWrite(m_txPin, !m_invert);
    }
    else {
      setRxGPIOPullUp();
      enableRx(true);
    }
  }
}

void SoftwareSerial::enableRx(bool on) {
  if (m_rxValid) {
    if (on) {
      m_rxLastBit = m_pduBits - 1;
      // Init to stop bit level and current cycle
#if defined (CONFIG_IDF_TARGET_ESP32C3)
      //    m_isrLastCycle = (micros() | 1) ^ m_invert;    //  FS +++++++++
      m_isrLastCycle = ((micros() * 2) | 1) ^ m_invert;  //  FS +++++++++  ???? si on stocke 2*micros() dans ISR
      attachInterruptArg(digitalPinToInterrupt(m_rxPin), reinterpret_cast<void (*)(void*)>(rxBitISR), this, CHANGE);
#else
      m_isrLastCycle = (ESP.getCycleCount() | 1) ^ m_invert;
      if (m_bitCycles >= (ESP.getCpuFreqMHz() * 1000000UL) / 74880UL) {
        attachInterruptArg(digitalPinToInterrupt(m_rxPin), reinterpret_cast<void (*)(void*)>(rxBitISR), this, CHANGE);
      }
      else {
        attachInterruptArg(digitalPinToInterrupt(m_rxPin), reinterpret_cast<void (*)(void*)>(rxBitSyncISR), this, m_invert ? RISING : FALLING);
      }
#endif
    }
    else {
      detachInterrupt(digitalPinToInterrupt(m_rxPin));
    }
    m_rxEnabled = on;
  }
}

int SoftwareSerial::read() {
  if (!m_rxValid) {
    return -1;
  }
  if (!m_buffer->available()) {
    rxBits();
    if (!m_buffer->available()) {
      return -1;
    }
  }
  auto val = m_buffer->pop();
  if (m_parityBuffer)
  {
    m_lastReadParity = m_parityBuffer->peek() & m_parityOutPos;
    m_parityOutPos <<= 1;
    if (!m_parityOutPos)
    {
      m_parityOutPos = 1;
      m_parityBuffer->pop();
    }
  }
  return val;
}

int SoftwareSerial::read(uint8_t* buffer, size_t size) {
  if (!m_rxValid) {
    return 0;
  }
  int avail;
  if (0 == (avail = m_buffer->pop_n(buffer, size))) {
    rxBits();
    avail = m_buffer->pop_n(buffer, size);
  }
  if (!avail) return 0;
  if (m_parityBuffer) {
    uint32_t parityBits = avail;
    while (m_parityOutPos >>= 1) ++parityBits;
    m_parityOutPos = (1 << (parityBits % 8));
    m_parityBuffer->pop_n(nullptr, parityBits / 8);
  }
  return avail;
}

size_t SoftwareSerial::readBytes(uint8_t* buffer, size_t size) {
  if (!m_rxValid || !size) {
    return 0;
  }
  size_t count = 0;
  auto start = millis();
  do {
    auto readCnt = read(&buffer[count], size - count);
    count += readCnt;
    if (count >= size) break;
    if (readCnt) start = millis();
    else optimistic_yield(1000UL);
  } while (millis() - start < _timeout);
  return count;
}

int SoftwareSerial::available() {
  if (!m_rxValid) {
    return 0;
  }
  rxBits();
  int avail = m_buffer->available();
  if (!avail) {
    optimistic_yield(10000UL);
  }
  return avail;
}


void SoftwareSerial::lazyDelay() {
  // Reenable interrupts while delaying to avoid other tasks piling up
  if (!m_intTxEnabled) {
    restoreInterrupts();
  }
  const auto expired = ESP.getCycleCount() - m_periodStart;
  const int32_t remaining = m_periodDuration - expired;
  const int32_t ms = remaining > 0 ? remaining / 1000L / static_cast<int32_t>(ESP.getCpuFreqMHz()) : 0;
  if (ms > 0)
  {
    delay(ms);
  }
  else
  {
    optimistic_yield(10000UL);
  }
  // Disable interrupts again if applicable
  if (!m_intTxEnabled) {
    disableInterrupts();
  }
  preciseDelay();
}

void IRAM_ATTR SoftwareSerial::preciseDelay() {
  while ((ESP.getCycleCount() - m_periodStart) < m_periodDuration) {}
  m_periodDuration = 0;
  m_periodStart = ESP.getCycleCount();
}


void IRAM_ATTR SoftwareSerial::writePeriod(
  uint32_t dutyCycle, uint32_t offCycle, bool withStopBit) {
  preciseDelay();
  if (dutyCycle)
  {
#if defined(ESP8266)
    if (16 == m_txPin) {
      GP16O = 1;
    }
    else {
      GPOS = m_txBitMask;
    }
#else
    *m_txReg |= m_txBitMask;
#endif
    m_periodDuration += dutyCycle;
    if (offCycle || (withStopBit && !m_invert)) {
      if (!withStopBit || m_invert) {
        preciseDelay();
      } else {
        lazyDelay();
      }
    }
  }
  if (offCycle)
  {
#if defined(ESP8266)
    if (16 == m_txPin) {
      GP16O = 0;
    }
    else {
      GPOC = m_txBitMask;
    }
#else
    *m_txReg &= ~m_txBitMask;
#endif
    m_periodDuration += offCycle;
    if (withStopBit && m_invert) lazyDelay();
  }
}

size_t SoftwareSerial::write(uint8_t byte) {
  return write(&byte, 1);
}

size_t SoftwareSerial::write(uint8_t byte, SoftwareSerialParity parity) {
  return write(&byte, 1, parity);
}

size_t SoftwareSerial::write(const uint8_t* buffer, size_t size) {
  return write(buffer, size, m_parityMode);
}

size_t IRAM_ATTR SoftwareSerial::write(const uint8_t* buffer, size_t size, SoftwareSerialParity parity) {
  if (m_rxValid) {
    rxBits();
  }
  if (!m_txValid) {
    return -1;
  }

  if (m_txEnableValid) {
    digitalWrite(m_txEnablePin, HIGH);
  }
  // Stop bit: if inverted, LOW, otherwise HIGH
  bool b = !m_invert;
  uint32_t dutyCycle = 0;
  uint32_t offCycle = 0;
  if (!m_intTxEnabled) {
    // Disable interrupts in order to get a clean transmit timing
    disableInterrupts();
  }
  const uint32_t dataMask = ((1UL << m_dataBits) - 1);
  bool withStopBit = true;
  m_periodDuration = 0;
  m_periodStart = ESP.getCycleCount();
  for (size_t cnt = 0; cnt < size; ++cnt) {
    uint8_t byte = pgm_read_byte(buffer + cnt) & dataMask;
    // push LSB start-data-parity-stop bit pattern into uint32_t
    // Stop bits: HIGH
    uint32_t word = ~0UL;
    // inverted parity bit, performance tweak for xor all-bits-set word
    if (parity && m_parityMode)
    {
      uint32_t parityBit;
      switch (parity)
      {
        case SWSERIAL_PARITY_EVEN:
          // from inverted, so use odd parity
          parityBit = byte;
          parityBit ^= parityBit >> 4;
          parityBit &= 0xf;
          parityBit = (0x9669 >> parityBit) & 1;
          break;
        case SWSERIAL_PARITY_ODD:
          // from inverted, so use even parity
          parityBit = byte;
          parityBit ^= parityBit >> 4;
          parityBit &= 0xf;
          parityBit = (0x6996 >> parityBit) & 1;
          break;
        case SWSERIAL_PARITY_MARK:
          parityBit = 0;
          break;
        case SWSERIAL_PARITY_SPACE:
        // suppresses warning parityBit uninitialized
        default:
          parityBit = 1;
          break;
      }
      word ^= parityBit;
    }
    word <<= m_dataBits;
    word |= byte;
    // Start bit: LOW
    word <<= 1;
    if (m_invert) word = ~word;
    for (int i = 0; i <= m_pduBits; ++i) {
      bool pb = b;
      b = word & (1UL << i);
      if (!pb && b) {
        writePeriod(dutyCycle, offCycle, withStopBit);
        withStopBit = false;
        dutyCycle = offCycle = 0;
      }
      if (b) {
        dutyCycle += m_bitCyclesWrite;
      }
      else {
        offCycle += m_bitCyclesWrite;
      }
    }
    withStopBit = true;
  }
  writePeriod(dutyCycle, offCycle, true);
  if (!m_intTxEnabled) {
    // restore the interrupt state if applicable
    restoreInterrupts();
  }
  if (m_txEnableValid) {
    digitalWrite(m_txEnablePin, LOW);
  }
  return size;
}

void SoftwareSerial::flush() {
  if (!m_rxValid) {
    return;
  }
  m_buffer->flush();
  if (m_parityBuffer)
  {
    m_parityInPos = m_parityOutPos = 1;
    m_parityBuffer->flush();
  }
}

bool SoftwareSerial::overflow() {
  bool res = m_overflow;
  m_overflow = false;
  return res;
}

int SoftwareSerial::peek() {
  if (!m_rxValid) {
    return -1;
  }
  if (!m_buffer->available()) {
    rxBits();
    if (!m_buffer->available()) return -1;
  }
  auto val = m_buffer->peek();
  if (m_parityBuffer) m_lastReadParity = m_parityBuffer->peek() & m_parityOutPos;
  return val;
}

void SoftwareSerial::rxBits() {
#ifdef ESP8266
  if (m_isrOverflow.load()) {
    m_overflow = true;
    m_isrOverflow.store(false);
  }
#else
  if (m_isrOverflow.exchange(false)) {
    m_overflow = true;
  }
#endif

  m_isrBuffer->for_each(m_isrBufferForEachDel);

  // A stop bit can go undetected if leading data bits are at same level
  // and there was also no next start bit yet, so one word may be pending.
  // Check that there was no new ISR data received in the meantime, inserting an
  // extraneous stop level bit out of sequence breaks rx.
  if (m_rxLastBit < m_pduBits - 1) {
    const uint32_t detectionCycles = (m_pduBits - 1 - m_rxLastBit) * m_bitCycles;
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  //  if (!m_isrBuffer->available() && micros() - m_isrLastCycle > detectionCycles) {  //   FS ++++++++++++++  ??
     if (!m_isrBuffer->available() && micros()*2 - m_isrLastCycle > detectionCycles) {  //   FS +++++++++  ????? si on stocke 2*micros() dans ISR
#else
    if (!m_isrBuffer->available() && ESP.getCycleCount() - m_isrLastCycle > detectionCycles) {
#endif
      // Produce faux stop bit level, prevents start bit maldetection
      // cycle's LSB is repurposed for the level bit
      rxBits(((m_isrLastCycle + detectionCycles) | 1) ^ m_invert);
    }
  }
}

void SoftwareSerial::rxBits(const uint32_t isrCycle) {
  const bool level = (m_isrLastCycle & 1) ^ m_invert;

  // error introduced by edge value in LSB of isrCycle is negligible
  uint32_t cycles = isrCycle - m_isrLastCycle;
  m_isrLastCycle = isrCycle;

  uint32_t bits = cycles / m_bitCycles;
  if (cycles % m_bitCycles > (m_bitCycles >> 1)) ++bits;
  while (bits > 0) {
    // start bit detection
    if (m_rxLastBit >= (m_pduBits - 1)) {
      // leading edge of start bit?
      if (level) break;
      m_rxLastBit = -1;
      --bits;
      continue;
    }
    // data bits
    if (m_rxLastBit < (m_dataBits - 1)) {
      uint8_t dataBits = min(bits, static_cast<uint32_t>(m_dataBits - 1 - m_rxLastBit));
      m_rxLastBit += dataBits;
      bits -= dataBits;
      m_rxCurByte >>= dataBits;
      if (level) {
        m_rxCurByte |= (BYTE_ALL_BITS_SET << (8 - dataBits));
      }
      continue;
    }
    // parity bit
    if (m_parityMode && m_rxLastBit == (m_dataBits - 1)) {
      ++m_rxLastBit;
      --bits;
      m_rxCurParity = level;
      continue;
    }
    // stop bits
    // Store the received value in the buffer unless we have an overflow
    // if not high stop bit level, discard word
    if (bits >= static_cast<uint32_t>(m_pduBits - 1 - m_rxLastBit) && level) {
      m_rxCurByte >>= (sizeof(uint8_t) * 8 - m_dataBits);
      if (!m_buffer->push(m_rxCurByte)) {
        m_overflow = true;
      }
      else {
        if (m_parityBuffer)
        {
          if (m_rxCurParity) {
            m_parityBuffer->pushpeek() |= m_parityInPos;
          }
          else {
            m_parityBuffer->pushpeek() &= ~m_parityInPos;
          }
          m_parityInPos <<= 1;
          if (!m_parityInPos)
          {
            m_parityBuffer->push();
            m_parityInPos = 1;
          }
        }
      }
    }
    m_rxLastBit = m_pduBits - 1;
    // reset to 0 is important for masked bit logic
    m_rxCurByte = 0;
    m_rxCurParity = false;
    break;
  }
}

void IRAM_ATTR SoftwareSerial::rxBitISR(SoftwareSerial* self) {
#if defined (CONFIG_IDF_TARGET_ESP32C3)
  //uint32_t curCycle = micros();   //   FS +++++++++++++++++++
   uint32_t curCycle = micros()*2;   //   FS +++++++++++++++++++ pour plus de précision ??
#else
  uint32_t curCycle = ESP.getCycleCount();
#endif
  bool level = *self->m_rxReg & self->m_rxBitMask;

  // Store level and cycle in the buffer unless we have an overflow
  // cycle's LSB is repurposed for the level bit
  if (!self->m_isrBuffer->push((curCycle | 1U) ^ !level)) self->m_isrOverflow.store(true);

}

void IRAM_ATTR SoftwareSerial::rxBitSyncISR(SoftwareSerial* self) {
  uint32_t start = ESP.getCycleCount();
  uint32_t wait = self->m_bitCycles - 172U;

  bool level = self->m_invert;
  // Store level and cycle in the buffer unless we have an overflow
  // cycle's LSB is repurposed for the level bit
  if (!self->m_isrBuffer->push(((start + wait) | 1U) ^ !level)) self->m_isrOverflow.store(true);

  for (uint32_t i = 0; i < self->m_pduBits; ++i) {
    while (ESP.getCycleCount() - start < wait) {};
    wait += self->m_bitCycles;

    // Store level and cycle in the buffer unless we have an overflow
    // cycle's LSB is repurposed for the level bit
    if (static_cast<bool>(*self->m_rxReg & self->m_rxBitMask) != level)
    {
      if (!self->m_isrBuffer->push(((start + wait) | 1U) ^ level)) self->m_isrOverflow.store(true);
      level = !level;
    }
  }
}

void SoftwareSerial::onReceive(Delegate<void(int available), void*> handler) {
  receiveHandler = handler;
}

void SoftwareSerial::perform_work() {
  if (!m_rxValid) {
    return;
  }
  rxBits();
  if (receiveHandler) {
    int avail = m_buffer->available();
    if (avail) {
      receiveHandler(avail);
    }
  }
}
