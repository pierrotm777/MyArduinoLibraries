/*
 * GPS_uBlox.ino — MPX_MSB v2.0.0 + SparkFun u-blox GNSS
 * Matériel: RX-9-DR + Teensy 4.0 (Serial3=Telemetry, I2C ou Serial1 pour GNSS)
 * Dépendance: SparkFun_u-blox_GNSS_Arduino_Library
 * Mapping identique à l’exemple NMEA.
 */
#include <MPX_MSB.h>
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // Install via Library Manager
using namespace MPX;

Mpx_Msb mpx;
SFE_UBLOX_GNSS gnss;

void setup(){
  mpx.begin(Serial3, 38400);
  mpx.mapGpsAddrs(/*alt*/8, /*spd*/4, /*cog*/1);
  mpx.mapVarioAddrs(/*alt*/8, /*vspd*/2);

  // GNSS en I2C
  Wire.begin();
  if (!gnss.begin()){
    // si besoin, fallback sur Serial1: gnss.begin(Serial1) après Serial1.begin(38400) ou autre
    while(1);
  }
  gnss.setAutoPVT(true);  // NAV-PVT en auto
  gnss.setAutoVELNED(true); // pour vitesse verticale (Down)
}

static uint32_t lastPvtMs = 0;
static long lastAlt = 0;

void loop(){

  if (gnss.getPVT()){
    // NAV-PVT donne tout
    double lat = gnss.getLatitude() / 1e7;   // deg
    double lon = gnss.getLongitude() / 1e7;  // deg
    long   altMSL = gnss.getAltitudeMSL();   // mm
    float  alt = altMSL / 1000.0f;           // m
    float  spd_kmh = gnss.getGroundSpeed() / 1000.0f * 3.6f; // mm/s → m/s → km/h
    float  cog = gnss.getHeading() / 1e5;    // deg * 1e5

    // Vario depuis VELNED si dispo, sinon dérivée altitude
    float vspd = 0.0f;
    if (gnss.getVELNED()){
      // down (mm/s) → m/s (négatif = montée)
      vspd = -(gnss.getDownSpeed()) / 1000.0f;
    } else {
      uint32_t now = millis();
      if (lastPvtMs){
        float dt = (now - lastPvtMs) / 1000.0f;
        vspd = (alt - (lastAlt/1000.0f)) / dt;
      }
      lastPvtMs = now; lastAlt = altMSL;
    }

    // Alimente la lib
    // (on passe aussi YY/MM/DD hh:mm:ss si tu veux les exploiter plus tard)
    mpx.Gps(lat, lon, alt, spd_kmh/3.6f, cog,
            (uint8_t)(gnss.getYear() - 2000),
            (uint8_t)gnss.getMonth(),
            (uint8_t)gnss.getDay(),
            (uint8_t)gnss.getHour(),
            (uint8_t)gnss.getMinute(),
            (uint8_t)gnss.getSecond());

    mpx.Vario(alt, vspd);
  }

  mpx.poll();
}
