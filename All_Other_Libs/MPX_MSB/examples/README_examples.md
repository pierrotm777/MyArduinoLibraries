
# MPX_MSB adapted examples

These examples were checked and adapted for both Teensy 4.0 and ESP32-S3.

## Default telemetry serial
- Teensy 4.0: `Serial3` at 38400 bauds.
- ESP32/ESP32-S3: `HardwareSerial(1)` with default pins `RX=2`, `TX=1`.

Change `MPX_RX_PIN` and `MPX_TX_PIN` at the top of each sketch if your ESP32-S3 board uses different pins.

## Important
- `addAlarmDigital()` must stay in `setup()`, never in `loop()`.
- Dynamic values must be refreshed in `loop()` before `mpx.poll()`.
- Do not reuse the MPX UART RX/TX pins as alarm inputs.
- The recommended B/D wiring is diode + 1 kΩ + common ground.

## External libraries
- `BMP280_Vario`: Adafruit BMP280 Library.
- `GPS_NMEA_TinyGPS`: TinyGPSPlus.
- `GPS_uBlox_TinyGPS`: SparkFun u-blox GNSS Arduino Library. The folder name is kept for compatibility; the sketch uses SparkFun GNSS over I2C, not TinyGPS++.
