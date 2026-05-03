#include <FrSky_FBUS.h>
#include <FrSky_FBUS_Telemetry.h>

// Exemple Teensy 4.0 :
// Serial1 = FBUS vers Archer R10+
// Serial2 = GPS u-blox configuré pour sortir UBX NAV-PVT

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1);     // FBUS 460800, 8N1, non inversé
  Serial2.begin(38400);          // GPS u-blox, à adapter à ton module

  FrSky_FBUS_Telemetry.setGpsEnabled(true);
  FrSky_FBUS_Telemetry.setEnabled(true);
}

void loop() {
  FrSky_FBUS.read();
  FrSky_FBUS_Telemetry.gpsProcessUBlox(Serial2);
}
