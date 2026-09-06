# RC3 Protocol

## Overview

**RC3** is a lightweight serial protocol designed to transmit digital data through a radio-control channel that can provide only **three distinct physical output levels**.

It was created for RC systems where a conventional serial-over-PWM protocol cannot be used because the receiver output does not provide enough stable pulse-width values.

The original use case was a **Pro-Tronik PTR-6A** system, where some channels associated with 3-position switches provide only about three usable PWM values.

Instead of requiring many different pulse widths to represent hexadecimal symbols or nibbles, RC3 uses only three physical states:

- `0`
- `1`
- `2`

These three states are called **trits**.

RC3 keeps the general philosophy of the existing `RcTxSerial` / `RcRxSerial` architecture while reducing the physical alphabet to three values.

---

## Why RC3 Was Created

Traditional `RcTxSerial` communication represents several logical symbols using many different PWM pulse widths.

That works well when the RC link can reproduce many distinct positions, but not when the physical channel can only reliably produce three values.

RC3 solves this limitation by encoding binary data using only three PWM levels.

Main design goals:

- support RC channels with only 3 reliable physical states;
- remain lightweight enough for AVR devices;
- remain compatible with ESP32 and other microcontrollers;
- avoid dynamic memory allocation;
- use small non-blocking state machines;
- provide error detection;
- support symbol repetition for noisy RC links;
- remain independent from the physical RC backend;
- integrate with the RCUL architecture;
- allow transparent bridging to the historical `RcSerial` protocol.

---

## Physical Trit Levels

The nominal RC3 pulse widths are:

| Trit | Nominal pulse width |
|---|---:|
| `0` | 1000 us |
| `1` | 1500 us |
| `2` | 2000 us |

Typical receiver decoding windows are:

| Trit | Accepted range |
|---|---:|
| `0` | 800 to 1200 us |
| `1` | 1300 to 1700 us |
| `2` | 1800 to 2200 us |

These values are only nominal. The actual three PWM levels can be calibrated to match the RC receiver.

---

## Binary Encoding

RC3 converts each group of **3 binary bits** into **2 trits**.

| 3-bit value | Trit pair |
|---|---|
| `000` | `00` |
| `001` | `01` |
| `010` | `02` |
| `011` | `10` |
| `100` | `11` |
| `101` | `12` |
| `110` | `20` |
| `111` | `21` |

The trit pair `22` is intentionally unused for normal encoded data.

This makes it possible to use consecutive `2` trits as a synchronization pattern.

---

## Frame Synchronization

Every RC3 frame starts with:

```text
2 2 2 0
```

This is the **SYNC** sequence.

Because valid encoded data never contains the pair `22`, this pattern is easy to detect and provides a robust frame boundary.

---

## Frame Format

A complete RC3 frame is:

```text
SYNC | HEADER | DATA | CRC4
```

Where:

- `SYNC` = `2,2,2,0`
- `HEADER` = 3 bits
- `DATA` = 1 to 4 bytes
- `CRC4` = 4-bit CRC

---

## Header

The 3-bit header is:

```text
bit 2    : sequence bit
bits 1:0 : payload length - 1
```

Payload size:

| Length bits | Payload bytes |
|---|---:|
| `00` | 1 |
| `01` | 2 |
| `10` | 3 |
| `11` | 4 |

The sequence bit allows the receiver to detect duplicate frames.

---

## CRC4

RC3 uses a 4-bit CRC.

```text
Polynomial : x^4 + x + 1
Low poly   : 0x3
Initial    : 0xF
Bit order  : MSB first
```

The CRC is calculated over:

```text
HEADER + PAYLOAD
```

A frame is accepted only when the received CRC matches the calculated CRC.

---

## Symbol Repetition

RC3 can repeat each physical trit several times.

This is **per-trit repetition**, not whole-frame repetition.

```text
RepeatNb = 0 -> 1 physical sample per trit
RepeatNb = 1 -> 2 physical samples per trit
RepeatNb = 2 -> 3 physical samples per trit
RepeatNb = 3 -> 4 physical samples per trit
```

A commonly validated setting is:

```text
RepeatNb = 2
```

The receiver performs majority decoding on the repeated samples, greatly improving robustness when pulse widths are occasionally distorted.

---

## Automatic Repeat Detection

`Rc3RxSerial` can automatically detect the transmitter repetition count from the SYNC sequence.

Nominal leading `2` counts are:

| Physical `2` samples | RepeatNb |
|---:|---:|
| 3 | 0 |
| 6 | 1 |
| 9 | 2 |
| 12 | 3 |
| 15 | 4 |

This avoids having to configure the same repeat value manually on both ends.

---

## RC3 Is Only a Transport Layer

RC3 transports bytes. It does not know what the payload means.

The payload can represent:

- switches;
- proportional values;
- angles;
- lighting commands;
- configuration values;
- telemetry;
- or any other application data.

This separation keeps the protocol reusable.

---

## RcSerial / X-Any Compatibility Mode

RC3 can transport data originating from the historical `RcTxSerial::sendNibbleMsg()` format.

The first RC3 payload byte contains compatibility metadata:

```text
bits 7..5 : 101  -> compatibility signature
bit 4     : legacy checksum enabled
bits 3..0 : number of useful legacy nibbles
```

Examples:

```text
0xB2 -> 2 useful nibbles, typically SW8
0xB4 -> 4 useful nibbles
0xB5 -> 5 useful nibbles
0xB6 -> 6 useful nibbles
```

For a pure SW8 message:

```cpp
uint8_t Message[1];
Message[0] = Contacts;

Rc3Tx.sendNibbleMsg(Message, 2, 1);
```

The RC3 compatibility payload becomes:

```text
Byte 0 = 0xB2
Byte 1 = SW8
```

A transparent bridge can then rebuild the historical RcSerial checksum after the RC3 CRC has validated the transport.

---

## SW8 Example

With:

```text
ANGLE = 0
PROP  = 0
SW_NB = 8
```

the payload is one byte:

```text
bit 0 -> SW1
bit 1 -> SW2
bit 2 -> SW3
bit 3 -> SW4
bit 4 -> SW5
bit 5 -> SW6
bit 6 -> SW7
bit 7 -> SW8
```

Expected RC3 compatibility values:

```text
Meta     = 0xB2
NibbleNb = 2
Length   = 2 bytes
```

---

## PCF8574 Example

A PCF8574 can be used on the transmitter side to read eight switches.

Typical wiring:

```text
P0 ---- switch ---- GND
P1 ---- switch ---- GND
...
P7 ---- switch ---- GND
```

Because the inputs are active-low in this configuration:

```cpp
Contacts = (uint8_t)~Raw;
```

So:

```text
closed switch -> logical 1
open switch   -> logical 0
```

If the PCF8574 shares the I2C bus with a timing-sensitive digital potentiometer such as an MCP4661, it should not be polled continuously.

A validated approach is to read the PCF8574 only **between two serial messages**, for example when the transmitter reports that it is ready for the next message.

This avoids disturbing the timing of the RC output backend.

---

## Physical Backend Independence

RC3 is independent from the physical hardware used to generate the RC signal.

Typical transmitter architecture:

```text
Rc3TxSerial
    |
    +-- RculPWMGen
    |
    +-- RculI2cPotTx
    |
    +-- another compatible Rcul backend
```

Typical receiver architecture:

```text
Receiver PWM
    |
SoftRcPulseIn
    |
Rc3RxSerial
    |
Application
```

---

## Transparent RC3-to-RcSerial Bridge

One important use case is:

```text
RC3 transmitter
      |
      v
RC radio link
      |
      v
RC3 receiver
      |
      v
Transparent bridge
      |
      v
Historical RcSerial PWM
      |
      v
Existing legacy decoder
```

The bridge does not need to understand the application semantics. It only rebuilds the legacy nibble stream and checksum.

This allows existing RcSerial-based equipment to remain unchanged.

---

## Receiver Diagnostics

`Rc3RxSerial` provides useful statistics such as:

```text
Good frames
Duplicate frames
CRC errors
Symbol errors
Majority corrections
Detected RepeatNb
Samples per trit
```

Example:

```text
Rc3RxSerial V1.1.1
Protocol      : RC3 v0.4
Repeat mode   : AUTO
Repeat detect : YES
RepeatNb      : 2 (3 physical samples/trit)
Good frames   : 40
Duplicates    : 0
CRC errors    : 1
Symbol errors : 0
Majority fix  : 165
```

These counters are useful when tuning pulse windows, repeat count, radio timing, or I2C activity.

---

## Reliability and Throughput

RC3 is designed primarily for reliability, not maximum speed.

With a stable link and:

```text
RepeatNb = 2
```

a pure SW8 message has been validated with a regular period of about:

```text
1.5 seconds
```

with reliable decoding.

The exact duration depends on:

- payload size;
- repetition count;
- RC transmitter update rate;
- physical backend timing;
- radio link timing.

---

## Summary

RC3 is a compact digital transport protocol for RC channels limited to three physical states.

Its main characteristics are:

- 3 physical PWM levels;
- 3 binary bits encoded into 2 trits;
- unambiguous `2,2,2,0` synchronization;
- 1 to 4 payload bytes;
- CRC4 error detection;
- optional per-trit repetition;
- majority decoding;
- automatic repeat detection;
- lightweight AVR-friendly implementation;
- ESP32 compatibility;
- RCUL backend independence;
- optional compatibility transport for historical RcSerial / X-Any messages.

RC3 was created to solve a specific RC hardware limitation, but its design makes it useful as a general-purpose low-bandwidth digital transport over any reliable 3-state RC channel.
