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

### 📡 Wiring (Bus B/D ↔ Teensy UART, half-duplex bus)

**Recommended (Option B — diode “open-drain”):**  
- Teensy **TX** → **diode 1N4148** → **BUS B/D** (cathode on BUS, anode on TX).  
  This prevents driving the line high; the receiver’s pull-up defines idle.  
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

## 🛰️ GPS Sensor Support

You can emulate a Multiplex GPS sensor by calling the `Gps()` function:

```cpp
mpx.Gps(48.858289, 2.294502,   // Latitude / Longitude (degrees, N/E positive)
         245.5,                 // Altitude (m)
         100.0,                 // Speed (m/s)
         90.23,                 // Course (degrees 0–359)
         24, 9, 14,             // Date (YY, MM, DD)
         12, 00, 00);           // Time (HH, MM, SS)
```

You can update these values dynamically in the `loop()` (for example from a **TinyGPS++** or **u-blox** module).  
All GPS data are transmitted in standard MSB GPS frames and are recognized by Multiplex transmitters.

---

## 🪂 Vario Sensor Support

You can emulate a Multiplex variometer by calling the `Vario()` function:

```cpp
mpx.Vario(250.5,  // Altitude in meters (can be negative)
           -1.5);  // Vertical speed in m/s (positive = up, negative = down)
```

This lets your radio use the **VSpd** value as a true variometer source.  
Combine with a barometric sensor (e.g. **BMP280**) to generate real altitude and climb/sink rates.

---

## 📂 Examples Included

- **SimpleDemo** → Battery + 2 temps + 2 alarms.  
- **DigitalAlarms4_Liquid_0_1** → 4 binary alarms on LIQUID (0/1).  
- **DigitalAlarms_Mixed** → RPM=0/100, SPEED=0/2.0 km/h, LIQUID=0/1, DIST=0/1.0 km.  
- **EchoDemo** → Demonstrates `setEchoMasking(true/false)`.  
- **GPS_uBlox_TinyGPS** → Example integration with u-blox GPS and TinyGPSPlus.  
- **GPS_NMEA_TinyGPS** → Example integration with NMEA GPS and TinyGPSPlus.
- **VbatFixed_Only** → Minimal test with fixed values only.  
- **BMP280_Vario** → Example using Adafruit BMP280 for altitude and variometer.

---

## ⚙️ Compatibility

- ✅ Tested with **Teensy 4.0** (Cortex-M7, 600 MHz).  
- ✅ The wrapper API is **Arduino AVR compatible** (compiles on ATmega328P/Pro Mini).  
- ⚠️ Performance and available serial ports may vary depending on the board.  

---

## 📖 Background

The **MSRC Multiplex telemetry code** by *laneboysrc* provides a full, feature-rich Multiplex **M-LINK** telemetry implementation.  
This wrapper, `Mpx_Msb`, was created to simplify usage in typical RC/Arduino projects:
- Direct setters (`sendVbat`, `sendTmp1`, `sendTmp2`).  
- Easy digital alarms (`addAlarmDigital`).  
- Retains full control through `raw()`.

This version was **designed together with ChatGPT**, starting from MSRC’s codebase, to be easier and faster to use for hobbyists.

---

## 🧪 Tested Hardware

- Multiplex RX-9-DR receiver  
- Teensy 4.0 board  
- Arduino Pro Mini (compatibility test only)

---

## 👤 Credits

- Original MSB implementation: **laneboysrc / rc-telemetry**  
- Simplified wrapper & docs: **pierrotm777**, assisted by **ChatGPT**
