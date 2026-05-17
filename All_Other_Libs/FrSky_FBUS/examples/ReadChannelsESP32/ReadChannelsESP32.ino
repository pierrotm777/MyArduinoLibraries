#include <FrSky_FBUS.h>

// Exemple ESP32 / ESP32-C3 / ESP32-S3
// FBUS est NON inversé, 460800 bauds, 8N1.
// Pour RX-only : TX peut rester à -1.
// Pour télémétrie half-duplex : relier TX au fil FBUS via 470R à 1k.

static constexpr int FBUS_RX_PIN = 4;
static constexpr int FBUS_TX_PIN = -1; // RX-only. Mettre une vraie pin TX pour la télémétrie.

HardwareSerial FbusSerial(1);

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.print("FBUS CPU: ");
  Serial.println(FrSky_FBUS.cpuName());

  // Sur ESP32, cette surcharge configure explicitement RX/TX + buffer UART.
  FrSky_FBUS.begin(FbusSerial, FBUS_RX_PIN, FBUS_TX_PIN);
}

void loop() {
  if (FrSky_FBUS.read()) {
    for (uint8_t i = 0; i < FrSky_FBUS.channelCount(); i++) {
      Serial.print(FrSky_FBUS.channelUs(i));
      Serial.print('	');
    }
    Serial.print(" FS=");
    Serial.print(FrSky_FBUS.failsafe());
    Serial.print(" RSSI=");
    Serial.println(FrSky_FBUS.rssi());
  }
}
