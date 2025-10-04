
#pragma once
#include <Arduino.h>
#include <mpx_msb.h>  // base library with MPX::MpxMsb, addAlarmChannel, addGenericChannel, ...

/*
  mpx_msb.h  —  Simplified wrapper & helpers around MPX::MpxMsb

  This header provides a light facade "MPX::MpxSimple" that:
    - exposes direct setters:  sendVbat(volts), sendTmp1(C), sendTmp2(C)
    - keeps all advanced features available (addGenericChannel, addAlarmChannel, setTxOnly, ...)
    - adds an easy digital alarm helper: addAlarmDigital(pin, addr, classId, ...)

  Notes:
    * No NTC/ADC here. You push your values directly with sendVbat/sendTmp1/2.
    * For advanced mappings (GPS, vario, etc.), use raw() to access MPX::MpxMsb.
*/

namespace MPX {

/**
 * @brief Wrapper simplifié au-dessus de MPX::MpxMsb
 *
 * Exemple minimal:
 *   MpxSimple mpx;
 *   mpx.begin(Serial3, 38400);
 *   mpx.sendVbat(15.0f);
 *   mpx.sendTmp1(25.0f);
 *   mpx.sendTmp2(30.0f);
 *   mpx.addAlarmDigital(2, 9, MPX_LIQUID);  // D2 en INPUT_PULLUP, LOW => 1 + bit alarme
 *   ...
 *   mpx.poll();  // dans loop()
 */
class MpxSimple {
public:
  /**
   * @brief Initialise la télémétrie Multiplex sur un port série matériel.
   * @param ser   Port série câblé à l’entrée B/D du RX (ex: Serial3 sur Teensy 4.0)
   * @param baud  Baudrate (par défaut 38400)
   *
   * Active par défaut les adresses: Vbat=3, Temp1=6, Temp2=7 (modifiables via map*Addr).
   * Active l'echo masking et fixe l'idle à ~500us.
   */
  void begin(HardwareSerial &ser, uint32_t baud = 38400);

  /** @brief Change l’adresse MSB pour Vbat (défaut 3). */
  void mapVbatAddr(uint8_t addr);

  /** @brief Change l’adresse MSB pour Temp1 (défaut 6). */
  void mapTemp1Addr(uint8_t addr);

  /** @brief Change l’adresse MSB pour Temp2 (défaut 7). */
  void mapTemp2Addr(uint8_t addr);

  /** @brief Met à jour la tension batterie (Volts). */
  void sendVbat(float volts);

  /** @brief Met à jour la température 1 (°C). */
  void sendTmp1(float t1);

  /** @brief Met à jour la température 2 (°C). */
  void sendTmp2(float t2);

  /**
   * @brief Ajoute une alarme digitale simple (0/1) avec bit d’alarme.
   *
   * @param pin        Numéro de pin Arduino à lire.
   * @param addr       Adresse MSB (0..15) où publier.
   * @param classId    Classe MSB à utiliser (ex: MPX_LIQUID pour affichage 0/1 clair).
   * @param activeLow  true: LOW = alarme active ; false: HIGH = alarme active. (défaut true)
   * @param usePullup  true: configure la pin en INPUT_PULLUP ; false: INPUT. (défaut true)
   * @param onValue    Valeur publiée quand l’alarme est active (défaut 1.0).
   * @param offValue   Valeur publiée quand l’alarme est inactive (défaut 0.0).
   * @param scale      Facteur d’échelle MSB (1.0 => unités entières ; 10.0 => dixièmes). (défaut 1.0)
   *
   * La lib gère en interne le digitalRead() et publie la valeur + positionne le bit alarme.
   * Pas besoin d’écrire des fonctions A0(), V0() dans votre sketch.
   */
  void addAlarmDigital(uint8_t pin, uint8_t addr, uint8_t classId,
                       bool activeLow = true, bool usePullup = true,
                       float onValue = 1.0f, float offValue = 0.0f,
                       float scale = 1.0f);

  /** @brief À appeler souvent dans loop() pour répondre aux polls MSB. */
  void poll();

  // ---- Options avancées (pass-through) ----
  void setIdleMicros(uint32_t us);
  void setEchoMasking(bool enable);
  void setTxOnly(bool enable, uint16_t periodMs = 20);

  /** @brief Accès direct à l’objet sous-jacent pour usages avancés. */
  MPX::MpxMsb& raw();

  // -------- Fournisseurs internes (ne pas appeler directement) --------
  static float _provVbatThunk();
  static float _provT1Thunk();
  static float _provT2Thunk();

private:
  MPX::MpxMsb _msb;
  uint8_t _addrV = 3, _addrT1 = 6, _addrT2 = 7;

  static float _vbat, _t1, _t2;

  // --- Gestion interne des alarmes digitales ---
  struct DAConf {
    uint8_t pin; bool activeLow;
    float onValue, offValue;
    uint8_t addr, classId;
    float scale;
    bool inUse;
  };
  static constexpr uint8_t MAX_DA = 8;
  static DAConf _da[MAX_DA];

  // Pour relier des callbacks C à nos entrées, on déclare 8 paires de thunks.
  static bool  _alarmThunk0(); static float _valueThunk0();
  static bool  _alarmThunk1(); static float _valueThunk1();
  static bool  _alarmThunk2(); static float _valueThunk2();
  static bool  _alarmThunk3(); static float _valueThunk3();
  static bool  _alarmThunk4(); static float _valueThunk4();
  static bool  _alarmThunk5(); static float _valueThunk5();
  static bool  _alarmThunk6(); static float _valueThunk6();
  static bool  _alarmThunk7(); static float _valueThunk7();

  static bool  _alarmThunkN(uint8_t i);
  static float _valueThunkN(uint8_t i);
};

} // namespace MPX
