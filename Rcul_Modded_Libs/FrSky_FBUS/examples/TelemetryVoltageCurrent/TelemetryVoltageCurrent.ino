#include <FrSky_FBUS.h>
#include <FrSky_FBUS_Telemetry.h>

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1); // FBUS 460800, 8N1, non inversé

  FrSky_FBUS_Telemetry.setVoltage_V(12.60f);
  FrSky_FBUS_Telemetry.setCurrent_A(3.4f);
  FrSky_FBUS_Telemetry.setEnabled(true);
}

void loop() {
  FrSky_FBUS.read();

  // Exemple de mise à jour lente des valeurs simulées.
  static uint32_t lastUpdateMs = 0;
  if (millis() - lastUpdateMs >= 500) {
    lastUpdateMs = millis();
    FrSky_FBUS_Telemetry.setVoltage_V(12.60f);
    FrSky_FBUS_Telemetry.setCurrent_A(3.4f);
  }
}
