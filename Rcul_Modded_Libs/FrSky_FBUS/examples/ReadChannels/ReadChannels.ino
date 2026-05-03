#include <FrSky_FBUS.h>

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1); // FBUS 460800, 8N1, non inversé
}

void loop() {
  if (FrSky_FBUS.read()) {
    for (uint8_t i = 0; i < FrSky_FBUS.channelCount(); i++) {
      Serial.print(FrSky_FBUS.channelUs(i));
      Serial.print('\t');
    }
    Serial.print(" FS=");
    Serial.print(FrSky_FBUS.failsafe());
    Serial.print(" RSSI=");
    Serial.println(FrSky_FBUS.rssi());
  }
}
