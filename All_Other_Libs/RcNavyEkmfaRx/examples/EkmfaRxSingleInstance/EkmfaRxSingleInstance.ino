#include <EkmfaRx.h>
#if not defined(ARDUINO_ARCH_ESP32)
#include <SoftRcPulseIn.h>
#else
#include <RculPWMRead.h>
#endif

#define EKMFA_RX_PIN                   2
#define RC_SIGNAL_ABSENCE_DURATION_MS  1000

#ifdef EKMFA_DEFAULT_DURATIONS
#error This sketch uses user-defined EKMFA durations: comment out the #define EKMFA_DEFAULT_DURATIONS in EkmfaRx.h!
#endif

#if not defined(ARDUINO_ARCH_ESP32)
static SoftRcPulseIn RcSignal;
#else
static RculPWMRead RcSignal;
#endif


EKMFA_FCT_MAP_TBL(MyEkmfaMap) = {
  EKMFA_FCT_POS(1, 1, A_AREA),
  EKMFA_FCT_POS(2, 1, D_AREA),
  EKMFA_FCT_POS(3, 2, A_AREA),
  EKMFA_FCT_POS(4, 2, D_AREA),
};

void setup()
{
  Serial.begin(115200);
  Serial.println(F("EKMFA RX Demo - single instance"));

  // ESP32: Preferences namespace. AVR/Teensy: ignored, so the same sketch remains portable.
  EkmfaRx.setPreferencesNamespace("ekmfa1");
  EkmfaRx.setEepBaseAddr(0);

  EkmfaRx.updateDurationMs(EKMFA_RX_RESET_DURATION_IDX,       200);
  EkmfaRx.updateDurationMs(EKMFA_RX_BURST_DURATION_IDX,       50);
  EkmfaRx.updateDurationMs(EKMFA_RX_INTER_BURST_DURATION_IDX, 50);
  EkmfaRx.updateDurationMs(EKMFA_RX_LAST_RECALL_DURATION_IDX, 250);

  RcSignal.attach(EKMFA_RX_PIN);
  EkmfaRx.begin(&RcSignal, RCUL_NO_CH, TBL_AND_ITEM_NB(MyEkmfaMap));
}

void loop()
{
  static uint16_t RcSignalStartMs = millis16();
  uint8_t FunctionToInvoke = EkmfaRx.process();

  if(FunctionToInvoke)
  {
    Serial.print(F("Calling Function N°"));
    Serial.println(FunctionToInvoke);
  }

  if(RcSignal.available()) RcSignalStartMs = millis16();

  if(ElapsedMs16Since(RcSignalStartMs) >= RC_SIGNAL_ABSENCE_DURATION_MS)
  {
    RcSignalStartMs = millis16();
    Serial.println(F("Failsafe!"));
  }
}
