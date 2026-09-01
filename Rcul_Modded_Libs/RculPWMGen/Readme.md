# RculPWMGen 1.1

Portable software RC pulse generator derived from the timing strategy of
`SoftRcPulseOut`.

Primary targets:
- ESP32 family, including ESP32-C3 and ESP32-S3
- Teensy, including Teensy 4.x

## Timing strategy

All attached outputs start together every 20 ms. Outputs are sorted by pulse
width. Interrupts remain enabled during most of each pulse, then are masked only
for the final `RCUL_PWM_GEN_EDGE_GUARD_US` (16 us by default) before each falling
edge. Closely spaced edges remain masked between edges.

Unlike the previous RculPWMGen attempt, `refresh()` does **not** call `yield()`
while a pulse is active.

## API

Compatible in spirit with SoftRcPulseOut:

```cpp
RculPWMGen Pwm;
Pwm.attach(5);
Pwm.write_us(1500);

void loop()
{
  RculPWMGen::refresh();
}
```

`refresh()` returns 1 only when a refresh has actually been executed.
`refresh(1)` forces an immediate refresh.

## RCUL

`RculPWMGen` derives from `Rcul` and implements:
- `RculIsSynchro()`
- `RculSetWidth_us()`
- `RculGetWidth_us()`

`RculIsSynchro()` calls `refresh()`. If a protocol engine uses
`RculIsSynchro()`, do not also call `RculPWMGen::refresh()` independently in the
same scheduling path.
