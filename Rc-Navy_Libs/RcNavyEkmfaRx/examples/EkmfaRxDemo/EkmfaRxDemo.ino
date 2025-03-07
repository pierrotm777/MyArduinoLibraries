#include <EkmfaRx.h>
#include <SoftRcPulseIn.h>

#define EKMFA_RX_PIN                   2 // The pin SHALL support Interrupt Pin Change (All the pins of UNO are suitable)

#define RC_SIGNAL_ABSENCE_DURATION_MS  1000

#ifdef EKMFA_DEFAULT_DURATIONS
#error This sketch uses user-defined EKMFA durations: comment out the #define EKMFA_DEFAULT_DURATIONS in EkmfaRx.h!
#endif

static SoftRcPulseIn RcSignal;

EKMFA_FCT_MAP_TBL(MyEkmfaMap) = {
                                          /* Fnct,Burst,Pos */
                                 EKMFA_FCT_POS( 1,  1, A_AREA), /* Function#1 is invoked when 1 burst in the A aera is received */
                                 EKMFA_FCT_POS( 2,  1, D_AREA),
                                 EKMFA_FCT_POS( 3,  2, A_AREA),
                                 EKMFA_FCT_POS( 4,  2, D_AREA), /* Function#4 is invoked when 2 burst in the D aera is received */
                                 EKMFA_FCT_POS( 5,  3, A_AREA),
                                 EKMFA_FCT_POS( 6,  3, D_AREA),
                                 EKMFA_FCT_POS( 7,  4, A_AREA),
                                 EKMFA_FCT_POS( 8,  4, D_AREA),
                                 EKMFA_FCT_POS( 9,  5, A_AREA),
                                 EKMFA_FCT_POS(10,  5, D_AREA),
                                 EKMFA_FCT_POS(11,  6, A_AREA),
                                 EKMFA_FCT_POS(12,  6, D_AREA),
                                 EKMFA_FCT_POS(13,  7, A_AREA),
                                 EKMFA_FCT_POS(14,  7, D_AREA),
                                 EKMFA_FCT_POS(15,  8, A_AREA),
                                 EKMFA_FCT_POS(16,  8, D_AREA),
                                 EKMFA_FCT_POS(17,  9, A_AREA),
                                 EKMFA_FCT_POS(18,  9, D_AREA),
                                 EKMFA_FCT_POS(19, 10, A_AREA),
                                 EKMFA_FCT_POS(20, 10, D_AREA),
                                 EKMFA_FCT_POS(21, 11, A_AREA),
                                 EKMFA_FCT_POS(22, 11, D_AREA),
                                 EKMFA_FCT_POS(23, 12, A_AREA),
                                 EKMFA_FCT_POS(24, 12, D_AREA),
                                 EKMFA_FCT_POS(25, 13, A_AREA),
                                 EKMFA_FCT_POS(26, 13, D_AREA),
                                 EKMFA_FCT_POS(27, 14, A_AREA),
                                 EKMFA_FCT_POS(28, 14, D_AREA),
                                 EKMFA_FCT_POS(29, 15, A_AREA),
                                 EKMFA_FCT_POS(30, 15, D_AREA),
                                };
                                
void setup()
{
  Serial.begin(115200);
  Serial.println(F("EKMFA RX Demo"));
  EkmfaRxClass::setEepBaseAddr(0);
  EkmfaRxClass::updateDurationMs(EKMFA_RX_RESET_DURATION_IDX,       200);
  EkmfaRxClass::updateDurationMs(EKMFA_RX_BURST_DURATION_IDX,       50);
  EkmfaRxClass::updateDurationMs(EKMFA_RX_INTER_BURST_DURATION_IDX, 50);
  EkmfaRxClass::updateDurationMs(EKMFA_RX_LAST_RECALL_DURATION_IDX, 250);
  RcSignal.attach(EKMFA_RX_PIN);
  EkmfaRx.begin(&RcSignal, RCUL_NO_CH, TBL_AND_ITEM_NB(MyEkmfaMap));
}

void loop()
{
  static uint16_t RcSignalStartMs = millis16();
  uint8_t         FunctionToInvoke;

  /********************/
  /* EKMFA Management */
  /********************/
  FunctionToInvoke = EkmfaRx.process();

  if(FunctionToInvoke) {Serial.print(F("Calling Function N°"));Serial.println(FunctionToInvoke);}
  
  switch(FunctionToInvoke)
  {
    case 1:
    Function1();
    break;
    
    case 2:
    Function2();
    break;

    case 3:
    Function3();
    break;
    
    case 4:
    Function4();
    break;

    // etc... up to Function30()
  }
  /***********************/
  /* Failsafe Management */
  /***********************/
  if(RcSignal.available())
  {
    RcSignalStartMs = millis16(); // If RC signal -> Rearm Timer
  }
  if(ElapsedMs16Since(RcSignalStartMs) >= RC_SIGNAL_ABSENCE_DURATION_MS) /* If no RC signal for at least RC_SIGNAL_ABSENCE_DURATION_MS ms -> Failsafe */
  {
    RcSignalStartMs = millis16();
    FailSafe();
  }
}

void FailSafe()
{
  // Put here your code in case of Failsafe
  Serial.println(F("Failsafe!"));
}

void Function1(void)
{
  Serial.println(F("Executing Function N°1"));
  // Do what you want here
}

void Function2(void)
{
  Serial.println(F("Executing Function N°2"));
  // Do what you want here
}

void Function3(void)
{
  Serial.println(F("Executing Function N°3"));
  // Do what you want here
}

void Function4(void)
{
  Serial.println(F("Executing Function N°4"));
  // Do what you want here
}

// Add below Function3() to Function30(), if needed...
