#include <FrSky_FBUS.h>
#include <FrSky_FBUS_Telemetry.h>

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1); // FBUS 460800, 8N1, non inversé

  FrSky_FBUS_Telemetry.setVoltage_V(12.60f);
  FrSky_FBUS_Telemetry.setCurrent_A(3.4f);
  FrSky_FBUS_Telemetry.setTemperature1_C(24.0f);
  FrSky_FBUS_Telemetry.setTemperature2_C(42.0f);
  FrSky_FBUS_Telemetry.setRpm(8500);
  FrSky_FBUS_Telemetry.setEnabled(true);
}

void loop() {
  FrSky_FBUS.read();
}
