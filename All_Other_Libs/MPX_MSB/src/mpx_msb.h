
// MPX_MSB.h - v2.0.0 (simplified, independent)
// - 3-byte MSB frame format (no 0x7E preamble)
// - STRICT poll→reply (no periodic broadcast)
// - Only the essentials you asked: Vbat, Temp1, Temp2, Digital alarms
#pragma once
#include <Arduino.h>

// ---- MSB "class" IDs ----
#define MPX_VOLT         1
#define MPX_CURRENT      2
#define MPX_VSPEED       3
#define MPX_SPEED        4
#define MPX_RPM          5
#define MPX_TMP          6
#define MPX_DIR          7
#define MPX_HEIGHT       8
#define MPX_LEVEL        9
#define MPX_LQ          10
#define MPX_CONSUMPTION 11
#define MPX_LIQUID      12
#define MPX_DIST        13

#ifndef MPX_ADC_MAX
#define MPX_ADC_MAX 1023
#endif

namespace MPX {

typedef float (*ValueProvider)();
typedef bool  (*AlarmProvider)();

class Mpx_Msb {
public:
  void begin(HardwareSerial &ser, uint32_t baud = 38400);

  // Map addresses (0..15). Default: V=3, T1=6, T2=7
  void mapVbatAddr(uint8_t addr)  { _addrV = (addr & 0x0F); }
  void mapTemp1Addr(uint8_t addr) { _addrT1 = (addr & 0x0F); }
  void mapTemp2Addr(uint8_t addr) { _addrT2 = (addr & 0x0F); }

  // Direct setters (you feed real-world values already scaled in volts / °C)
  void sendVbat(float volts) { _vbat = volts; }
  void sendTmp1(float t1)    { _t1   = t1;    }
  void sendTmp2(float t2)    { _t2   = t2;    }

  // Digital alarms (0/1), optional pullup & activeLow, optional custom on/off values and scale
  void addAlarmDigital(uint8_t pin, uint8_t addr, uint8_t classId = MPX_LIQUID,
                       bool activeLow = true, bool usePullup = true,
                       float onValue = 1.0f, float offValue = 0.0f,
                       float scale   = 1.0f);

  // Providers (optional) if you prefer callbacks rather than sendVbat()/sendTmpX()
  void setVoltageProvider(ValueProvider p) { _provV = p; }
  void setTemp1Provider(ValueProvider p)   { _provT1 = p; }
  void setTemp2Provider(ValueProvider p)   { _provT2 = p; }

  // Tuning
  void setIdleMicros(uint32_t us)   { _idleUs = us; }      // delay between RX poll and our reply
  void setEchoMasking(bool enable)  { _echoMask = enable; }
  
 // Mapper les adresses utilisées par GPS / VARIO (désactivées par défaut = 0xFF)
 void mapGpsAddrs(uint8_t altAddr, uint8_t spdAddr, uint8_t cogAddr) {
   _addrAlt = (altAddr & 0x0F); _addrSpd = (spdAddr & 0x0F); _addrCog = (cogAddr & 0x0F);
 }
 void mapVarioAddrs(uint8_t altAddr, uint8_t vspdAddr) {
   _addrAlt = (altAddr & 0x0F); _addrVSpd = (vspdAddr & 0x0F);
 }

// Dépôt des valeurs GPS & VARIO (cachées, envoyées à la demande via poll)
void Gps(double lat_deg, double lon_deg,
         float alt_m,
         float speed_m_s,
         float course_deg,
         uint8_t yy, uint8_t mm, uint8_t dd,
         uint8_t hh, uint8_t mi, uint8_t ss);

void Vario(float alt_m, float vspd_m_s);
  

  // Main service
  void poll(); // call often in loop()

private:
  // 3-byte MSB frame: [ (addr<<4) | (class&0x0F) , LSB(value), MSB(value) ]
  void send3(uint8_t addr, uint8_t klass, uint16_t enc);
  static uint16_t encodeDeci(float deci, bool alarmBit=false); // 15-bit signed (0.1 units), LSB=alarm

  // Resolve current values (provider or cached direct)
  float vbat() const { return _provV ? _provV() : _vbat; }
  float t1()   const { return _provT1? _provT1(): _t1;   }
  float t2()   const { return _provT2? _provT2(): _t2;   }

  struct DAlarm {
    uint8_t pin, addr, classId;
    bool inUse, activeLow;
    float onValue, offValue, scale;
  };
  static const uint8_t MAX_DA = 8;
  DAlarm _da[MAX_DA];
  uint8_t _daCount = 0;

  // Helpers
  void drainEcho(uint8_t nbytes);
  void replyIfAsked(uint8_t polledAddr);
  
  // --- AJOUTS (private) ---
  // Adresses (0xFF = désactivé)
  uint8_t _addrAlt = 0xFF;   // classe MPX_HEIGHT (m, échelle 1)
  uint8_t _addrVSpd = 0xFF;  // classe MPX_VSPEED (0.1 m/s)
  uint8_t _addrSpd = 0xFF;   // classe MPX_SPEED  (0.1 km/h)
  uint8_t _addrCog = 0xFF;   // classe MPX_DIR    (0.1 °)

  // Caches valeurs
  float _gps_alt_m = 0.0f;
  float _gps_spd_kmh = 0.0f;   // converti depuis m/s
  float _gps_cog_deg = 0.0f;   // 0..359.9
  float _vario_alt_m = 0.0f;
  float _vario_vspd_ms = 0.0f;

  // Date/heure GPS (stockées mais non transmises; utilitaires éventuels)
  uint8_t _gps_y=0,_gps_m=0,_gps_d=0,_gps_h=0,_gps_min=0,_gps_s=0;

private:
  HardwareSerial* _ser = nullptr;
  uint8_t _addrV=3, _addrT1=6, _addrT2=7;
  float _vbat=0.0f, _t1=0.0f, _t2=0.0f;
  uint32_t _idleUs = 300;
  bool _echoMask = false;

  // Optional providers
  ValueProvider _provV=nullptr, _provT1=nullptr, _provT2=nullptr;
};

} // namespace MPX
