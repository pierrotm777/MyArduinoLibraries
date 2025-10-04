
#include "mpx_msb.h"
using namespace MPX;

float MpxSimple::_vbat = 0.0f;
float MpxSimple::_t1   = 0.0f;
float MpxSimple::_t2   = 0.0f;
MpxSimple::DAConf MpxSimple::_da[MpxSimple::MAX_DA] = {};

// ---------- Built-ins ----------
void MpxSimple::begin(HardwareSerial &ser, uint32_t baud){
  _msb.begin(ser, baud);
  _msb.setEchoMasking(true);
  _msb.setIdleMicros(500);
  _msb.setAddresses(_addrV, _addrT1, _addrT2);
  _msb.setCurrentAddress(0xFF);
  _msb.setVoltageProvider(&_provVbatThunk);
  _msb.setTemp1Provider(&_provT1Thunk);
  _msb.setTemp2Provider(&_provT2Thunk);
}
void MpxSimple::mapVbatAddr(uint8_t addr){ _addrV=addr; _msb.setAddresses(_addrV,_addrT1,_addrT2); }
void MpxSimple::mapTemp1Addr(uint8_t addr){ _addrT1=addr; _msb.setAddresses(_addrV,_addrT1,_addrT2); }
void MpxSimple::mapTemp2Addr(uint8_t addr){ _addrT2=addr; _msb.setAddresses(_addrV,_addrT1,_addrT2); }
void MpxSimple::sendVbat(float volts){ _vbat=volts; }
void MpxSimple::sendTmp1(float t1){ _t1=t1; }
void MpxSimple::sendTmp2(float t2){ _t2=t2; }
void MpxSimple::poll(){ _msb.pollOnce(); }
void MpxSimple::setIdleMicros(uint32_t us){ _msb.setIdleMicros(us); }
void MpxSimple::setEchoMasking(bool enable){ _msb.setEchoMasking(enable); }
void MpxSimple::setTxOnly(bool enable, uint16_t periodMs){ _msb.setTxOnly(enable, periodMs); }
MPX::MpxMsb& MpxSimple::raw(){ return _msb; }
float MpxSimple::_provVbatThunk(){ return _vbat; }
float MpxSimple::_provT1Thunk(){ return _t1; }
float MpxSimple::_provT2Thunk(){ return _t2; }

// ---------- Digital alarms ----------
void MpxSimple::addAlarmDigital(uint8_t pin, uint8_t addr, uint8_t classId,
                                bool activeLow, bool usePullup,
                                float onValue, float offValue, float scale){
  // find a free slot
  int idx = -1;
  for (uint8_t i=0;i<MAX_DA;i++){ if(!_da[i].inUse){ idx=i; break; } }
  if (idx < 0) return; // full

  if (usePullup) pinMode(pin, INPUT_PULLUP); else pinMode(pin, INPUT);

  _da[idx] = DAConf{pin, activeLow, onValue, offValue, addr, classId, scale, true};

  // bind thunks
  switch(idx){
    case 0: _msb.addAlarmChannel(addr, classId, &_alarmThunk0, &_valueThunk0, 0,0, scale); break;
    case 1: _msb.addAlarmChannel(addr, classId, &_alarmThunk1, &_valueThunk1, 0,0, scale); break;
    case 2: _msb.addAlarmChannel(addr, classId, &_alarmThunk2, &_valueThunk2, 0,0, scale); break;
    case 3: _msb.addAlarmChannel(addr, classId, &_alarmThunk3, &_valueThunk3, 0,0, scale); break;
    case 4: _msb.addAlarmChannel(addr, classId, &_alarmThunk4, &_valueThunk4, 0,0, scale); break;
    case 5: _msb.addAlarmChannel(addr, classId, &_alarmThunk5, &_valueThunk5, 0,0, scale); break;
    case 6: _msb.addAlarmChannel(addr, classId, &_alarmThunk6, &_valueThunk6, 0,0, scale); break;
    case 7: _msb.addAlarmChannel(addr, classId, &_alarmThunk7, &_valueThunk7, 0,0, scale); break;
  }
}

bool  MpxSimple::_alarmThunkN(uint8_t i){
  const DAConf &c = _da[i];
  if (!c.inUse) return false;
  int s = digitalRead(c.pin);
  return c.activeLow ? (s==LOW) : (s==HIGH);
}
float MpxSimple::_valueThunkN(uint8_t i){
  const DAConf &c = _da[i];
  if (!c.inUse) return 0.0f;
  int s = digitalRead(c.pin);
  bool active = c.activeLow ? (s==LOW) : (s==HIGH);
  return active ? c.onValue : c.offValue;
}

// 8 thunk pairs
bool  MpxSimple::_alarmThunk0(){ return _alarmThunkN(0); }
float MpxSimple::_valueThunk0(){ return _valueThunkN(0); }
bool  MpxSimple::_alarmThunk1(){ return _alarmThunkN(1); }
float MpxSimple::_valueThunk1(){ return _valueThunkN(1); }
bool  MpxSimple::_alarmThunk2(){ return _alarmThunkN(2); }
float MpxSimple::_valueThunk2(){ return _valueThunkN(2); }
bool  MpxSimple::_alarmThunk3(){ return _alarmThunkN(3); }
float MpxSimple::_valueThunk3(){ return _valueThunkN(3); }
bool  MpxSimple::_alarmThunk4(){ return _alarmThunkN(4); }
float MpxSimple::_valueThunk4(){ return _valueThunkN(4); }
bool  MpxSimple::_alarmThunk5(){ return _alarmThunkN(5); }
float MpxSimple::_valueThunk5(){ return _valueThunkN(5); }
bool  MpxSimple::_alarmThunk6(){ return _alarmThunkN(6); }
float MpxSimple::_valueThunk6(){ return _valueThunkN(6); }
bool  MpxSimple::_alarmThunk7(){ return _alarmThunkN(7); }
float MpxSimple::_valueThunk7(){ return _valueThunkN(7); }
