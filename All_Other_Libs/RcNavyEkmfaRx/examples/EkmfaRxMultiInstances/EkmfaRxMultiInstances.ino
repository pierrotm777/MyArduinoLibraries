#include <EkmfaRx.h>
#if not defined(ARDUINO_ARCH_ESP32)
#include <SoftRcPulseIn.h>
#else
#include <RculPWMRead.h>
#endif

#define EKMFA_RX1_PIN 2
#define EKMFA_RX2_PIN 3

#ifdef EKMFA_DEFAULT_DURATIONS
#error This sketch uses user-defined EKMFA durations: comment out the #define EKMFA_DEFAULT_DURATIONS in EkmfaRx.h!
#endif

#if not defined(ARDUINO_ARCH_ESP32)
static SoftRcPulseIn RcSignal1;
static SoftRcPulseIn RcSignal2;
#else
static RculPWMRead RcSignal1;
static RculPWMRead RcSignal2;
#endif

static EkmfaRxClass EkmfaRx1;
static EkmfaRxClass EkmfaRx2;

EKMFA_FCT_MAP_TBL(MyEkmfaMap) = {
  EKMFA_FCT_POS(1, 1, A_AREA),
  EKMFA_FCT_POS(2, 1, D_AREA),
  EKMFA_FCT_POS(3, 2, A_AREA),
  EKMFA_FCT_POS(4, 2, D_AREA),
};

void setup()
{
  Serial.begin(115200);
  Serial.println(F("EKMFA RX Demo - multi instances"));

  // ESP32: namespaces different = no collision, even with the same base address.
  // AVR/Teensy: namespaces are ignored, so use different EEPROM base addresses.
  EkmfaRx1.setPreferencesNamespace("ekmfa1");
  EkmfaRx1.setEepBaseAddr(0);

  EkmfaRx2.setPreferencesNamespace("ekmfa2");
#if defined(ARDUINO_ARCH_ESP32)
  EkmfaRx2.setEepBaseAddr(0);
#else
  EkmfaRx2.setEepBaseAddr(EkmfaRx1.getEepTotalSize());
#endif

  EkmfaRx1.updateDurationMs(EKMFA_RX_RESET_DURATION_IDX,       200);
  EkmfaRx1.updateDurationMs(EKMFA_RX_BURST_DURATION_IDX,       50);
  EkmfaRx1.updateDurationMs(EKMFA_RX_INTER_BURST_DURATION_IDX, 50);
  EkmfaRx1.updateDurationMs(EKMFA_RX_LAST_RECALL_DURATION_IDX, 250);

  EkmfaRx2.updateDurationMs(EKMFA_RX_RESET_DURATION_IDX,       200);
  EkmfaRx2.updateDurationMs(EKMFA_RX_BURST_DURATION_IDX,       50);
  EkmfaRx2.updateDurationMs(EKMFA_RX_INTER_BURST_DURATION_IDX, 50);
  EkmfaRx2.updateDurationMs(EKMFA_RX_LAST_RECALL_DURATION_IDX, 250);

  RcSignal1.attach(EKMFA_RX1_PIN);
  RcSignal2.attach(EKMFA_RX2_PIN);

  EkmfaRx1.begin(&RcSignal1, RCUL_NO_CH, TBL_AND_ITEM_NB(MyEkmfaMap));
  EkmfaRx2.begin(&RcSignal2, RCUL_NO_CH, TBL_AND_ITEM_NB(MyEkmfaMap));
}

void loop()
{
  uint8_t f1 = EkmfaRx1.process();
  uint8_t f2 = EkmfaRx2.process();

  if(f1)
  {
    Serial.print(F("RX1 Function N°"));
    Serial.println(f1);
  }

  if(f2)
  {
    Serial.print(F("RX2 Function N°"));
    Serial.println(f2);
  }
}
