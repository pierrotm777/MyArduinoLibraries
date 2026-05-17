# EspBoatAudio

`EspBoatAudio` is an ESP32 / ESP32-S3 WAV audio engine designed for RC boats, vehicles and embedded sound modules.

It is optimized for projects that need:

- preloaded engine sound in RAM/PSRAM
- dynamic pitch and volume
- short FX sounds
- long ambient sounds streamed from SD
- priority-based FX slots
- queue playback
- repeat playback
- handle-based sound tracking
- migration from Teensy Audio projects

---

## Main architecture

| Voice | Usage | Source |
|---|---|---|
| `VOICE_ENGINE` | main engine loop | RAM / PSRAM |
| `VOICE_AMBIENT` | long ambient sounds | SD streaming |
| `VOICE_FX0..VOICE_FX3` | short or automatic FX | RAM or SD streaming |

The audio render task never accesses the SD card directly.

SD streaming must be serviced from `loop()` using:

```cpp
audio.streamTick();
audio.chainTick();