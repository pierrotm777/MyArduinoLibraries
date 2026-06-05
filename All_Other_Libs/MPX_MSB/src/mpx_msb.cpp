
// MPX_MSB.cpp - v2.0.0 (simplified, independent)
#include "MPX_MSB.h"
#include <math.h>
namespace MPX {

void Mpx_Msb::begin(HardwareSerial &ser, uint32_t baud){
  _ser = &ser; _ser->begin(baud);
  _daCount = 0;
}

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
void Mpx_Msb::begin(HardwareSerial &ser,
                    int8_t rxPin,
                    int8_t txPin,
                    uint32_t baud){
  _ser = &ser;
  _ser->begin(baud, SERIAL_8N1, rxPin, txPin);
  _daCount = 0;
}
#endif

void Mpx_Msb::addAlarmDigital(uint8_t pin, uint8_t addr, uint8_t classId,
                              bool activeLow, bool usePullup,
                              float onValue, float offValue, float scale){
  if (_daCount >= MAX_DA) return;
  if (usePullup) pinMode(pin, INPUT_PULLUP); else pinMode(pin, INPUT);
  _da[_daCount++] = DAlarm{pin, (uint8_t)(addr&0x0F), classId, true, activeLow, onValue, offValue, scale};
}

uint16_t Mpx_Msb::encodeDeci(float deci, bool alarm){
  int v = (int)lroundf(deci);           // value in tenths
  if (v < -16384) v = -16384;
  if (v >  16383) v =  16383;
  uint16_t u = ((uint16_t)(v << 1)) & 0xFFFE;
  if (alarm) u |= 1;
  return u;
}

void Mpx_Msb::drainEcho(uint8_t n){
  if(!_ser || !_echoMask) return;
  uint32_t s = micros();
  uint16_t d = 0;
  while (d < n && (micros()-s) < 1000){
    if (_ser->read() >= 0) d++;
  }
}

void Mpx_Msb::send3(uint8_t addr, uint8_t klass, uint16_t enc){
  if(!_ser) return;
  uint8_t b0 = (uint8_t)((addr & 0x0F) << 4) | (klass & 0x0F);
  uint8_t b1 = (uint8_t)(enc & 0xFF);
  uint8_t b2 = (uint8_t)((enc >> 8) & 0xFF);
  _ser->write(b0); _ser->write(b1); _ser->write(b2);
  drainEcho(3);
}

void Mpx_Msb::replyIfAsked(uint8_t a){
  // Vbat
  if ((_addrV & 0x0F) == a){
    float vb = vbat();
    send3(_addrV, MPX_VOLT, encodeDeci(vb * 10.0f, false));
    return;
  }
  // Temp1
  if ((_addrT1 & 0x0F) == a){
    float t = t1();
    send3(_addrT1, MPX_TMP, encodeDeci(t * 10.0f, false));
    return;
  }
  // Temp2
  if ((_addrT2 & 0x0F) == a){
    float t = t2();
    send3(_addrT2, MPX_TMP, encodeDeci(t * 10.0f, false));
    return;
  }
  
  // --- AJOUTS: GPS/VARIO ---
  // Altitude (partagée GPS/VARIO)
  if ((_addrAlt != 0xFF) && ((_addrAlt & 0x0F) == a)) {
    float alt = (_vario_alt_m != 0.0f) ? _vario_alt_m : _gps_alt_m; // préfère vario si fourni récemment
    send3(_addrAlt, MPX_HEIGHT, encodeDeci(alt * 1.0f, false)); // échelle 1 m
    return;
  }

  // Vitesse verticale (VARIO)
  if ((_addrVSpd != 0xFF) && ((_addrVSpd & 0x0F) == a)) {
    send3(_addrVSpd, MPX_VSPEED, encodeDeci(_vario_vspd_ms * 10.0f, false)); // 0.1 m/s
    return;
  }

  // Vitesse sol (GPS)
  if ((_addrSpd != 0xFF) && ((_addrSpd & 0x0F) == a)) {
    send3(_addrSpd, MPX_SPEED, encodeDeci(_gps_spd_kmh * 10.0f, false)); // 0.1 km/h
    return;
  }

  // Cap (GPS)
  if ((_addrCog != 0xFF) && ((_addrCog & 0x0F) == a)) {
    send3(_addrCog, MPX_DIR, encodeDeci(_gps_cog_deg * 10.0f, false)); // 0.1°
    return;
  }
  
  
  // Digital alarms
  for (uint8_t i=0;i<_daCount;i++){
    const DAlarm &d = _da[i];
    if (!d.inUse || ((d.addr & 0x0F) != a)) continue;
    int s = digitalRead(d.pin);
    bool active = d.activeLow ? (s==LOW) : (s==HIGH);
    float v = active ? d.onValue : d.offValue;
    send3(d.addr, d.classId, encodeDeci(v * d.scale, active)); // LSB=alarm
    return;
  }
}

void Mpx_Msb::Gps(double /*lat_deg*/, double /*lon_deg*/,
                   float alt_m, float speed_m_s, float course_deg,
                   uint8_t yy, uint8_t mm, uint8_t dd,
                   uint8_t hh, uint8_t mi, uint8_t ss)
{
  // On stocke les paramètres utiles au MSB (lat/lon non transmis dans ce format compact)
  _gps_alt_m   = alt_m;
  _gps_spd_kmh = speed_m_s * 3.6f;               // MPX_SPEED = 0.1 km/h
  // normalise le cap 0..360
  while (course_deg < 0)   course_deg += 360.0f;
  while (course_deg >=360) course_deg -= 360.0f;
  _gps_cog_deg = course_deg;

  // mémorise la date/heure (non envoyée, mais dispo si besoin)
  _gps_y = yy; _gps_m = mm; _gps_d = dd;
  _gps_h = hh; _gps_min = mi; _gps_s = ss;
}

void Mpx_Msb::Vario(float alt_m, float vspd_m_s)
{
  _vario_alt_m  = alt_m;
  _vario_vspd_ms = vspd_m_s;                     // MPX_VSPEED = 0.1 m/s
  // NB: si _addrAlt est mappée, on pourra publier cette altitude aussi via VARIO
}


void Mpx_Msb::poll(){
  if (!_ser) return;
  while (_ser->available()){
    uint8_t poll = (uint8_t)_ser->read();

    // Respect a small idle delay before answering.
    // Use micros() instead of elapsedMicros so the code remains portable
    // across AVR, Teensy, ESP32 and other Arduino-compatible platforms.
    uint32_t startUs = micros();
    while ((uint32_t)(micros() - startUs) < _idleUs) {
      /* spin */
    }

    replyIfAsked((uint8_t)(poll & 0x0F));
  }
}

} // namespace MPX
