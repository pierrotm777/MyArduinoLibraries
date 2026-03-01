#include "VirtualUart.h"
#include <stdlib.h>
#include <string.h>

VirtualUart::VirtualUart(size_t rxSize, size_t txSize) {
  if (rxSize < 64) rxSize = 64;
  if (txSize < 64) txSize = 64;

  _rx.cap = rxSize;
  _tx.cap = txSize;

  _rx.buf = (uint8_t*)malloc(_rx.cap);
  _tx.buf = (uint8_t*)malloc(_tx.cap);

  clearAll();
}

VirtualUart::~VirtualUart() {
  if (_rx.buf) free(_rx.buf);
  if (_tx.buf) free(_tx.buf);
  _rx.buf = _tx.buf = nullptr;
  _rx.cap = _tx.cap = 0;
}

size_t VirtualUart::ringAvail(const Ring& r) {
  if (!r.buf || r.cap == 0) return 0;
  if (r.head >= r.tail) return (r.head - r.tail);
  return (r.cap - (r.tail - r.head));
}

int VirtualUart::ringPeek(const Ring& r) {
  if (r.tail == r.head) return -1;
  return r.buf[r.tail];
}

int VirtualUart::ringRead(Ring& r) {
  if (r.tail == r.head) return -1;
  uint8_t v = r.buf[r.tail];
  r.tail = (r.tail + 1) % r.cap;
  return (int)v;
}

void VirtualUart::ringClear(Ring& r) {
  r.head = r.tail = 0;
}

void VirtualUart::ringPush(Ring& r, uint8_t b) {
  size_t n = (r.head + 1) % r.cap;
  if (n == r.tail) {
    // plein => drop oldest (avance tail)
    r.tail = (r.tail + 1) % r.cap;
    r.drop++;
  }
  r.buf[r.head] = b;
  r.head = n;
  r.push++;
}

// ─────────────────────────────────────────────
// RX injection
// ─────────────────────────────────────────────
void VirtualUart::pushRx(const uint8_t* data, size_t len) {
  if (!data || !len) return;
  portENTER_CRITICAL(&_mux);
  for (size_t i = 0; i < len; i++) ringPush(_rx, data[i]);
  portEXIT_CRITICAL(&_mux);
}

void VirtualUart::pushRx(uint8_t b) {
  portENTER_CRITICAL(&_mux);
  ringPush(_rx, b);
  portEXIT_CRITICAL(&_mux);
}

// ─────────────────────────────────────────────
// TX extraction
// ─────────────────────────────────────────────
int VirtualUart::availableTx() const {
  portENTER_CRITICAL((portMUX_TYPE*)&_mux);
  int a = (int)ringAvail(_tx);
  portEXIT_CRITICAL((portMUX_TYPE*)&_mux);
  return a;
}

int VirtualUart::readTx() {
  portENTER_CRITICAL(&_mux);
  int v = ringRead(_tx);
  portEXIT_CRITICAL(&_mux);
  return v;
}

size_t VirtualUart::readTx(uint8_t* out, size_t maxLen) {
  if (!out || !maxLen) return 0;
  portENTER_CRITICAL(&_mux);
  size_t n = 0;
  while (n < maxLen) {
    int v = ringRead(_tx);
    if (v < 0) break;
    out[n++] = (uint8_t)v;
  }
  portEXIT_CRITICAL(&_mux);
  return n;
}

// ─────────────────────────────────────────────
// Clear
// ─────────────────────────────────────────────
void VirtualUart::clearRx() {
  portENTER_CRITICAL(&_mux);
  ringClear(_rx);
  portEXIT_CRITICAL(&_mux);
}

void VirtualUart::clearTx() {
  portENTER_CRITICAL(&_mux);
  ringClear(_tx);
  portEXIT_CRITICAL(&_mux);
}

void VirtualUart::clearAll() {
  portENTER_CRITICAL(&_mux);
  ringClear(_rx);
  ringClear(_tx);
  _rx.push = _rx.drop = 0;
  _tx.push = _tx.drop = 0;
  portEXIT_CRITICAL(&_mux);
}

VirtualUart::Stats VirtualUart::stats() const {
  Stats s;
  portENTER_CRITICAL((portMUX_TYPE*)&_mux);
  s.rx_push = _rx.push;
  s.rx_drop = _rx.drop;
  s.tx_push = _tx.push;
  s.tx_drop = _tx.drop;
  portEXIT_CRITICAL((portMUX_TYPE*)&_mux);
  return s;
}

// ─────────────────────────────────────────────
// Stream RX
// ─────────────────────────────────────────────
int VirtualUart::available() {
  portENTER_CRITICAL(&_mux);
  int a = (int)ringAvail(_rx);
  portEXIT_CRITICAL(&_mux);
  return a;
}

int VirtualUart::read() {
  portENTER_CRITICAL(&_mux);
  int v = ringRead(_rx);
  portEXIT_CRITICAL(&_mux);
  return v;
}

int VirtualUart::peek() {
  portENTER_CRITICAL(&_mux);
  int v = ringPeek(_rx);
  portEXIT_CRITICAL(&_mux);
  return v;
}

void VirtualUart::flush() {
  // no-op: on ne jette pas le TX automatiquement
}

// ─────────────────────────────────────────────
// Stream/Print TX write
// ─────────────────────────────────────────────
size_t VirtualUart::write(uint8_t b) {
  portENTER_CRITICAL(&_mux);
  ringPush(_tx, b);
  portEXIT_CRITICAL(&_mux);
  return 1;
}

size_t VirtualUart::write(const uint8_t* buffer, size_t size) {
  if (!buffer || !size) return 0;
  portENTER_CRITICAL(&_mux);
  for (size_t i = 0; i < size; i++) ringPush(_tx, buffer[i]);
  portEXIT_CRITICAL(&_mux);
  return size;
}