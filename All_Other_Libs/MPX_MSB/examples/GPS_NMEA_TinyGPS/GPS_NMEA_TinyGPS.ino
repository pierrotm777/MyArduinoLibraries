/*
 * GPS_NMEA.ino — MPX_MSB v2.0.0 + TinyGPS++ (NMEA)
 * Matériel: RX-9-DR (M-LINK) + Teensy 4.0 (Serial3=Telemetry, Serial1=GPS à 9600 bauds)
 * Mapping:
 *   - Vbat = addr 3 (comme d'hab)
 *   - Temp1 = 6 ; Temp2 = 7
 *   - Altitude = addr 8 (MPX_HEIGHT)
 *   - Vario (Vspeed) = addr 2 (MPX_VSPEED)
 *   - Vitesse sol = addr 4 (MPX_SPEED)
 *   - Cap = addr 1 (MPX_DIR)
 */
#include <MPX_MSB.h>
#include <TinyGPSPlus.h>
using namespace MPX;

Mpx_Msb mpx;
TinyGPSPlus gps;

void setup(){
  mpx.begin(Serial3, 38400);

  // mappage GPS + VARIO
  mpx.mapGpsAddrs(/*alt*/8, /*spd*/4, /*cog*/1);
  mpx.mapVarioAddrs(/*alt*/8, /*vspd*/2);

  // GPS NMEA sur Serial1
  Serial1.begin(9600);
}

static uint32_t lastFixMs = 0;
static float lastAlt = 0.0f;

void loop(){
  // --- Parse NMEA
  while (Serial1.available()){
    gps.encode(Serial1.read());
  }

  if (gps.location.isValid() && gps.date.isUpdated() && gps.time.isUpdated()){
    double lat = gps.location.lat();
    double lon = gps.location.lng();
    float alt  = gps.altitude.isValid() ? gps.altitude.meters() : lastAlt;
    float spd  = gps.speed.isValid()    ? gps.speed.mps()      : 0.0f;
    float cog  = gps.course.isValid()   ? gps.course.deg()     : 0.0f;

    // calcule un vario simple (dérivée altitude)
    uint32_t now = millis();
    float vspd = 0.0f;
    if (lastFixMs){
      float dt = (now - lastFixMs) / 1000.0f;
      if (dt > 0.001f) vspd = (alt - lastAlt) / dt;
    }
    lastFixMs = now; lastAlt = alt;

    // Alimente la lib
    mpx.Gps(lat, lon, alt, spd, cog,
            (uint8_t)(gps.date.year() - 2000),
            (uint8_t)gps.date.month(),
            (uint8_t)gps.date.day(),
            (uint8_t)gps.time.hour(),
            (uint8_t)gps.time.minute(),
            (uint8_t)gps.time.second());

    mpx.Vario(alt, vspd);
  }

  // Répond aux polls de la radio
  mpx.poll();
}
