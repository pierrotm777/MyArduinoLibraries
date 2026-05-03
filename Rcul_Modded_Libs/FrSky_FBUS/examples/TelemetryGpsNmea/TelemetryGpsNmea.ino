#include <FrSky_FBUS.h>
#include <FrSky_FBUS_Telemetry.h>

// Exemple Teensy 4.0 :
// Serial1 = FBUS vers Archer R10+
// Serial2 = GPS NMEA à 9600 ou 38400 bauds selon module

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1);     // FBUS 460800, 8N1, non inversé
  Serial2.begin(9600);           // GPS NMEA

  FrSky_FBUS_Telemetry.setGpsEnabled(true);
  FrSky_FBUS_Telemetry.setEnabled(true);
}

void loop() {
  FrSky_FBUS.read();
  FrSky_FBUS_Telemetry.gpsProcessNMEA(Serial2);
}
