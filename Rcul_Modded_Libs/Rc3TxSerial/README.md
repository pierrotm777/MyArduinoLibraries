# Rc3TxSerial 1.3.1

Lightweight generic RC3 transmitter using an `Rcul` destination.

## New in V1.3: printInfo()

`printInfo()` now reports the complete RC3 TX configuration, including
RepeatNb, trit widths and RX-calibration pattern status:

```cpp
Rc3Tx.printInfo(Serial);
```

Example:

```text
----------------------------------------
Rc3TxSerial V1.3
Protocol      : RC3 v0.4
RepeatNb      : 2 (3 physical samples/trit)
Trit widths   : 1000 / 1500 / 2000 us
Pattern use   : RX 3-level calibration
Pattern mode  : ON
Pattern       : 0 -> 1 -> 2, 25 physical frames/level
Pattern level : 1 (12 frames left)
TX feedback   : NONE
----------------------------------------
```

The final line is intentional: this Rc3TxSerial pattern has no return channel.
It can report the emitted pattern state, but not the remote calibration result.

## RX-calibration pattern emitted by TX

Rc3TxSerial cannot measure the received PWM levels itself, so this mode is an
**emission pattern** for calibrating the receiver through the complete radio
path.

```cpp
Rc3Tx.startLearn();       // default: 25 physical RC frames per level
```

While the calibration pattern is active, normal messages are refused and the output cycles:

```text
trit 0 -> trit 1 -> trit 2 -> trit 0 -> ...
```

Each level is held for 25 physical receiver frames by default. On a ~20 ms
radio cycle this is about 0.5 s per level and ~1.5 s for a complete cycle.

The pattern sample count is independent of `RepeatNb`.

```cpp
Rc3Tx.startLearn(40);     // 40 physical frames per level
Rc3Tx.learnActive();
Rc3Tx.stopLearn();
```

`stopLearn()` automatically inserts the normal idle separator before the next
RC3 frame, which also gives `Rc3RxSerial` AUTO repetition detection a clean
SYNC boundary.

## Existing API

```cpp
Rc3TxSerial Tx(&RculDst, RC3_TX_SERIAL_REPEAT2, 8, Channel);

Tx.sendMsg(data, len);
Tx.sendNibbleMsg(packedNibbles, nibbleCount, 1);
Tx.setRepeatNb(RC3_TX_SERIAL_REPEAT2);
Tx.setTritWidths(1000, 1500, 2000);
Rc3TxSerial::process();
```

No dynamic allocation and no STL.
