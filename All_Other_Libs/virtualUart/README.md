# VirtualUart

Lightweight virtual UART implementation for ESP boards.

## Overview

VirtualUart implements the `Stream` interface using a circular buffer.
It allows you to inject bytes from non-UART transports (ESP-NOW, radio links, network bridges, etc.) and use them as if they were coming from a hardware serial port.

This is especially useful for:

- SBUS over ESP-NOW
- RC protocol bridging
- Virtual serial ports
- Custom transport layers

---

## Features

- Circular ring buffer
- Non-blocking push
- Automatic overflow handling (oldest byte dropped)
- Fully compatible with Arduino `Stream`

---

## Basic Usage

```cpp
#include <VirtualUart.h>

VirtualUart vUart(2048);

void setup() {
  Serial.begin(115200);
}

void loop() {
  if (vUart.available()) {
    int b = vUart.read();
    Serial.write(b);
  }
}