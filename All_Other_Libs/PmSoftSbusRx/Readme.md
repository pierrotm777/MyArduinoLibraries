# PmSoftSbusRx v2.0

Experimental software SBUS receiver for Arduino Nano / ATmega328P.

Default test target:

- D11 input
- inverted SBUS signal
- 100000 baud, 8E2
- RCUL-compatible source for `RcRxSerial`

This version timestamps received bytes and accepts only frames starting with `0x0F` after an inter-frame gap, then validates the footer before updating channels.

Diagnostic counters:

- `rxCount()` raw received bytes
- `validCount()` accepted SBUS frames
- `rejectCount()` rejected candidate frames
