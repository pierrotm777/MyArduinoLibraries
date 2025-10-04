# MPX_MSB Library (Simplified Wrapper)

A simplified wrapper around the original **[MSRC Multiplex MSB telemetry library](https://github.com/laneboysrc/rc-telemetry)**.  
This wrapper was designed with the help of **ChatGPT** to make it easier to use basic telemetry features such as **battery voltage, temperatures, and digital alarms**, while still keeping all advanced options from the original MSRC code.

---

## ✨ Features

- ✅ Simple API for common telemetry:
  - `sendVbat(volts)`
  - `sendTmp1(tempC)`
  - `sendTmp2(tempC)`
- ✅ Easy digital alarm mapping:
  - `addAlarmDigital(pin, addr, classId, activeLow=true, usePullup=true, onValue=1.0, offValue=0.0, scale=1.0)`
- ✅ Retains full power of MSRC:
  - Advanced methods: `addGenericChannel()`, `addAlarmChannel()`, `setEchoMasking()`, `setTxOnly()`, etc.
  - Direct access to the underlying `MPX::MpxMsb` object via `mpx.raw()`.
- ✅ Clean inline documentation in the header (`mpx_msb.h`).

---

## ⚡ Hardware Setup

This library has been **tested with a Multiplex RX-9-DR receiver** and a **Teensy 4.0**.

### 📡 Wiring (Bus B/D ↔ Teensy UART, half‑duplex bus)

**Recommended (Option B — diode “open-drain”):**  
- Teensy **TX** → **diode 1N4148** → **BUS B/D** (cathode on BUS, anode on TX).  
  This prevents driving the line high; the receiver’s pull‑up defines idle.  
- **BUS B/D** → **1 kΩ series resistor** → Teensy **RX** (same serial port).  
  Protects RX and limits currents in case of contention.  
- **GND (receiver)** ↔ **GND (Teensy)** (mandatory common ground).

**Alternative (direct TX — use with caution):**  
- Teensy **TX** → **BUS B/D** (direct).  
- **BUS B/D** → **1 kΩ** → Teensy **RX**.  
- **GND** shared.  
Use only if you are sure the bus idle level and voltage are safe for your MCU (Teensy 4.0 is **not 5V tolerant**).

> Configure the library with `mpx.setEchoMasking(true)` since TX and RX share the same wire (you will otherwise read your own frames).

**Typical Teensy 4.0 pins:**  
- `Serial3`: TX3 = pin 14, RX3 = pin 15 (verify your board pinout).

---

## 🚀 Quick Example

```cpp
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;

void setup() {
  mpx.begin(Serial3, 38400);

  // Publish direct values
  mpx.sendVbat(15.0f);   // Volts
  mpx.sendTmp1(25.0f);   // °C
  mpx.sendTmp2(30.0f);   // °C

  // Add two simple digital alarms
  mpx.addAlarmDigital(2,  9, MPX_LIQUID);  // Pin D2 → Addr9 → 0/1 with alarm bit
  mpx.addAlarmDigital(3, 10, MPX_LIQUID);  // Pin D3 → Addr10
}

void loop() {
  mpx.poll();
}
```

---

## 📂 Examples Included

- **SimpleDemo** → Battery + 2 temps + 2 alarms.  
- **DigitalAlarms4_Liquid_0_1** → 4 binary alarms on LIQUID (0/1).  
- **DigitalAlarms_Mixed** → RPM=0/100, SPEED=0/2.0 km/h, LIQUID=0/1, DIST=0/1.0 km.  
- **EchoDemo** → Demonstrates `setEchoMasking(true/false)`.  
- **GPS_uBlox_TinyGPS** → Example integration with u-blox GPS and TinyGPSPlus.  
- **VbatFixed_Only** → Minimal test with fixed values only.

---

## ⚙️ Compatibility

- ✅ Tested with **Teensy 4.0** (Cortex‑M7, 600 MHz).  
- ✅ The wrapper API is **Arduino AVR compatible** (compiles on ATmega328P/Pro Mini).  
- ⚠️ Performance and available serial ports may vary depending on the board.  

---

## 📖 Background

The **MSRC Multiplex telemetry code** by *laneboysrc* provides a full, feature‑rich Multiplex telemetry implementation.  
This wrapper, `MpxSimple`, was created to simplify usage in typical RC/Arduino projects:
- Direct setters (`sendVbat`, `sendTmp1`, `sendTmp2`).  
- Easy digital alarms (`addAlarmDigital`).  
- Retains full control through `raw()`.

This version was **designed together with ChatGPT**, starting from MSRC’s codebase, to be easier and faster to use for hobbyists.

---

## 👤 Credits

- Original MSB implementation: **laneboysrc / rc-telemetry**  
- Simplified wrapper & docs: **pierrotm777**, assisted by **ChatGPT**
