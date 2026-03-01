#pragma once
#include <Arduino.h>

// VirtualUart : "UART" mémoire full-duplex (Stream)
// - RX: injecté via pushRx(); lu via Stream::read()
// - TX: écrit via Stream::write(); récupéré via readTx()/availableTx()
// Thread-safe via portMUX (ESP32 / FreeRTOS).

class VirtualUart : public Stream {
public:
  struct Stats {
    uint32_t rx_push = 0;
    uint32_t rx_drop = 0;
    uint32_t tx_push = 0;
    uint32_t tx_drop = 0;
  };

  VirtualUart(size_t rxSize = 4096, size_t txSize = 4096);
  ~VirtualUart();

  // ── RX injection (depuis extérieur) ─────────────────────────
  void pushRx(const uint8_t* data, size_t len);
  void pushRx(uint8_t b);

  // ── TX extraction (vers extérieur) ─────────────────────────
  int    availableTx() const;
  int    readTx();                                // -1 si vide
  size_t readTx(uint8_t* out, size_t maxLen);     // lit jusqu'à maxLen

  // ── Gestion buffers ───────────────────────────────────────
  void clearRx();
  void clearTx();
  void clearAll();

  Stats stats() const;

  // ── Stream (RX) ───────────────────────────────────────────
  int available() override;
  int read() override;
  int peek() override;
  void flush() override;                          // no-op (voir note)
  size_t write(uint8_t b) override;               // écrit en TX buffer
  size_t write(const uint8_t* buffer, size_t size) override;

  using Print::write;

private:
  struct Ring {
    uint8_t* buf = nullptr;
    size_t   cap = 0;
    size_t   head = 0;
    size_t   tail = 0;
    uint32_t push = 0;
    uint32_t drop = 0;
  };

  static size_t ringAvail(const Ring& r);
  static int    ringPeek(const Ring& r);
  static int    ringRead(Ring& r);
  static void   ringClear(Ring& r);
  static void   ringPush(Ring& r, uint8_t b);

  Ring _rx;
  Ring _tx;

  mutable portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};