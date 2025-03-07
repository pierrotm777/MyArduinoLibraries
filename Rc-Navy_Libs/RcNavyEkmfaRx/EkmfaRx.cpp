#include "EkmfaRx.h"
/*

 English: by RC Navy (2021)
 =======
 <EkmfaRx>: a library to simply receive up to 30 EKMFA commands. Pulse counting is performed automatically by the library.
 http://p.loussouarn.free.fr/contact.html
 V1.0: (06/06/2021) initial release
 V1.1: (13/06/2022) Bug fixed when EkmfaRx driven manually from a transmitter stick (Commands in D area did not work)

 
 Francais: par RC Navy (2021)
 ========
 <EkmfaRx>: une bibliotheque pour recevoir simplement jusqu'a 30 commandes EKMFA. Le comptage des impulsions est realise automatiquement par la bibliotheque.
 http://p.loussouarn.free.fr/contact.html
 V1.0: (06/06/2021) release initiale
 V1.1: (13/06/2022) Correction bug quand EkmfaRx etait pilote manuellement depuis le manche d'un emetteur (les commandes en zone D ne fonctionnaient pas)

*/

/*

                ^ --------------
 2.0 ms -->  o  |  A area        ^
             |  x -------------- |
 1.75ms -->  o  |  B area        | The EkmfaTx library counts stick pulses between Neutral area and A area
             |  x -------------- |
 1.5 ms -->  o  |  Neutral area  x
             |  x -------------- |
 1.25ms -->  o  |  C area        | The EkmfaTx library counts stick pulses between Neutral area and D area
             |  x -------------- |
 1.O ms -->  o  |  D area        v
                v -------------- 

                                                      Counter=1xA/D                          Counter=2xA/D
              ________________________ ______________________ _________________ ______________________ ________________________
Duration:           Reset ms          |       Burst ms       |  Inter-Burst ms |      Burst ms        |  Inter-Burst ms 
Pulse Width: <-------With=1500us-----><---Width=A/D_Width---><--With=1500us---><---Width=A/D_Width---><-------With=1500us----->
                                      <-------------------------------------------------------------->
                                      |                                                              |
                               Start of sequence                                                End of sequence
*/

#ifndef EKMFA_DEFAULT_DURATIONS
static uint16_t GlobEepBaseAddr = 0;
#endif

/* Private function prototypes */

/* Global variables */
EkmfaRxClass    EkmfaRx = EkmfaRxClass();


#define NEUTRAL_US                            1500
#define EXC_MAX_US                            500

#define EKMFA_A_D_EXC_PERCENT                 (80 - 10)
#define EKMFA_B_C_EXC_PERCENT                 (40 - 10)
#define EKMFA_N_EXC_PERCENT                   (20 - 10)

#define EKMFA_A_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US + ((EKMFA_A_D_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_B_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US + ((EKMFA_B_C_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_NB_PULSE_WIDTH_US               (uint16_t)(NEUTRAL_US + ((EKMFA_N_EXC_PERCENT   * (uint32_t)EXC_MAX_US) / 100))

#define EKMFA_NC_PULSE_WIDTH_US               (uint16_t)(NEUTRAL_US - ((EKMFA_N_EXC_PERCENT   * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_C_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US - ((EKMFA_B_C_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_D_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US - ((EKMFA_A_D_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))

#ifdef EKMFA_DEFAULT_DURATIONS
#define EKMFA_INV_CORRECTED_DURATION_MS(Val)  ((Val) + ((Val) >> 2) + ((Val) >> 1)) /* 37.5% higher than real values */
#define EKMFA_RX_RAW_RESET_DURATION_MS        EKMFA_INV_CORRECTED_DURATION_MS(EKMFA_RX_DEFAULT_RESET_DURATION_MS)
#define EKMFA_RX_RAW_BURST_DURATION_MS        EKMFA_INV_CORRECTED_DURATION_MS(EKMFA_RX_DEFAULT_BURST_DURATION_MS)
#define EKMFA_RX_RAW_INTER_BURST_DURATION_MS  EKMFA_INV_CORRECTED_DURATION_MS(EKMFA_RX_DEFAULT_INTER_BURST_DURATION_MS)
#define EKMFA_RX_RAW_LAST_RECALL_DURATION_MS  EKMFA_INV_CORRECTED_DURATION_MS(EKMFA_RX_DEFAULT_LAST_RECALL_DURATION_MS)
#else
#define EKMFA_RX_RAW_RESET_DURATION_MS        (EkmfaRxClass::getEepDurationMs(EKMFA_RX_RESET_DURATION_IDX))
#define EKMFA_RX_RAW_BURST_DURATION_MS        (EkmfaRxClass::getEepDurationMs(EKMFA_RX_BURST_DURATION_IDX))
#define EKMFA_RX_RAW_INTER_BURST_DURATION_MS  (EkmfaRxClass::getEepDurationMs(EKMFA_RX_INTER_BURST_DURATION_IDX))
#define EKMFA_RX_RAW_LAST_RECALL_DURATION_MS  (EkmfaRxClass::getEepDurationMs(EKMFA_RX_LAST_RECALL_DURATION_IDX))
#endif

#define EKMFA_CORRECTED_DURATION_MS(Val)      ((Val) - ((Val) >> 2)) /* 25% lower than real values */
#define EKMFA_RX_RESET_DURATION_MS            EKMFA_CORRECTED_DURATION_MS(EKMFA_RX_RAW_RESET_DURATION_MS)
#define EKMFA_RX_BURST_DURATION_MS            EKMFA_CORRECTED_DURATION_MS(EKMFA_RX_RAW_BURST_DURATION_MS)
#define EKMFA_RX_INTER_BURST_DURATION_MS      EKMFA_CORRECTED_DURATION_MS(EKMFA_RX_RAW_INTER_BURST_DURATION_MS)
#define EKMFA_RX_LAST_RECALL_DURATION_MS      EKMFA_CORRECTED_DURATION_MS(EKMFA_RX_RAW_LAST_RECALL_DURATION_MS)

#define EKMFA_RX_TIMEOUT_MS                   2000

#define EKMFA_MAX_CMD_NB                      30 /* Do NOT change this */

enum {EKMFA_RX_IDLE = 0, EKMFA_RX_CONFIRM_REPEAT, EKMFA_RX_CONFIRM_BURST, EKMFA_RX_WAIT_FOR_NEUTRAL, EKMFA_RX_CONFIRM_NEUTRAL, EKMFA_RX_CONFIRM_RECALL};

/* Constructor */
EkmfaRxClass::EkmfaRxClass()
{

}

/* EkmfaTx.begin() shall be called in the setup() */
void EkmfaRxClass::begin(Rcul *Rcul, uint8_t Ch /*= RCUL_NO_CH*/, const uint8_t *MyMapInFlash /*= NULL*/, uint8_t MapSize /*= 0*/)
{
#if 1
  Ekmfa.MyMapInFlash = MyMapInFlash;
  Ekmfa.MapSize      = min(MapSize, EKMFA_MAX_CMD_NB);
  reassignRculSrc(Rcul, Ch);
#else
  Ekmfa._Rcul        = Rcul;
  Ekmfa.MyMapInFlash = MyMapInFlash;
  Ekmfa.MapSize      = MapSize;
  Ekmfa.Ch           = Ch;
  Ekmfa.LastCmd      = 0;
  Ekmfa.CmdRepeat    = 0;
  Ekmfa.State        = EKMFA_RX_IDLE;
#endif
  #if (EKMFA_RX_DEBUG > 0)
  Serial.println();
  Serial.print(F("EKMFA_MAP_SIZE="));Serial.println(Ekmfa.MapSize);
  Serial.print(F("EKMFA_RX_RESET_DURATION_MS="));Serial.println(EKMFA_RX_RESET_DURATION_MS);
  Serial.print(F("EKMFA_RX_BURST_DURATION_MS="));Serial.println(EKMFA_RX_BURST_DURATION_MS);
  Serial.print(F("EKMFA_RX_INTER_BURST_DURATION_MS="));Serial.println(EKMFA_RX_INTER_BURST_DURATION_MS);
  Serial.print(F("EKMFA_RX_LAST_RECALL_DURATION_MS="));Serial.println(EKMFA_RX_LAST_RECALL_DURATION_MS);
  Serial.println();
  Serial.print(F("EKMFA_A_PULSE_WIDTH_US="));Serial.println(EKMFA_A_PULSE_WIDTH_US);
  Serial.print(F("EKMFA_B_PULSE_WIDTH_US="));Serial.println(EKMFA_B_PULSE_WIDTH_US);
  Serial.print(F("EKMFA_NB_PULSE_WIDTH_US="));Serial.println(EKMFA_NB_PULSE_WIDTH_US);
  Serial.println();
  Serial.print(F("EKMFA_NC_PULSE_WIDTH_US="));Serial.println(EKMFA_NC_PULSE_WIDTH_US);
  Serial.print(F("EKMFA_C_PULSE_WIDTH_US="));Serial.println(EKMFA_C_PULSE_WIDTH_US);
  Serial.print(F("EKMFA_D_PULSE_WIDTH_US="));Serial.println(EKMFA_D_PULSE_WIDTH_US);
  Serial.println();
  #endif
}

void EkmfaRxClass::reassignRculSrc(Rcul *Rcul, uint8_t Ch /*= RCUL_NO_CH*/)
{
  Ekmfa._Rcul        = Rcul;
  Ekmfa.Ch           = Ch;

  Ekmfa.LastCmd      = 0;
  Ekmfa.CmdRepeat    = 0;
  Ekmfa.State        = EKMFA_RX_IDLE;
}

/* EkmfaRx.process() shall be in the loop() (To make it work, blocking functions such as delay() are forbidden in the loop()) */
uint8_t EkmfaRxClass::process(void)
{
//  static uint16_t StartMs16 = millis16();  
  uint16_t PulseWidthUs;
  uint8_t  Idx, Ret = 0; /* Ret = (N x A | 0x80) or (N x D) */

  if(Ekmfa._Rcul->RculIsSynchro())
  {
   PulseWidthUs = Ekmfa._Rcul->RculGetWidth_us(Ekmfa.Ch);
    switch(Ekmfa.State)
    {
      case EKMFA_RX_IDLE:
//if(ElapsedMs16Since(StartMs16) >= 2000) {StartMs16 = millis16();Serial.println(F("EKMFA_RX_IDLE"));}
      if((PulseWidthUs > EKMFA_A_PULSE_WIDTH_US) || (PulseWidthUs < EKMFA_D_PULSE_WIDTH_US))
      {
        #if (EKMFA_RX_DEBUG > 0)
        Serial.print(millis16());Serial.print(F(": EKMFA_RX_IDLE -> EKMFA_RX_CONFIRM_BURST Rx WidthUs="));Serial.println(PulseWidthUs);
        #endif
        Ekmfa.StartMs16 = millis16();
        Ekmfa.BurstCnt  = 0;
        Ekmfa.CmdRepeat = 0;
        Ekmfa.InterBurstReached = 0;
        Ekmfa.PosA = (PulseWidthUs > EKMFA_A_PULSE_WIDTH_US);
        Ekmfa.State = EKMFA_RX_CONFIRM_BURST;
      }
      else
      {
        if((PulseWidthUs < EKMFA_C_PULSE_WIDTH_US) && (PulseWidthUs > EKMFA_D_PULSE_WIDTH_US))
        {
          Ekmfa.StartMs16 = millis16();
          #if (EKMFA_RX_DEBUG > 0)
          Serial.print(millis16());Serial.print(F(": EKMFA_RX_IDLE -> EKMFA_RX_CONFIRM_REPEAT Rx WidthUs="));Serial.println(PulseWidthUs);
          #endif
          Ekmfa.State = EKMFA_RX_CONFIRM_REPEAT;
        }
      }
      break;
      
      case EKMFA_RX_CONFIRM_REPEAT:
      if((PulseWidthUs < EKMFA_C_PULSE_WIDTH_US) && (PulseWidthUs > EKMFA_D_PULSE_WIDTH_US))
      {
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_LAST_RECALL_DURATION_MS)
        {
          Ekmfa.CmdRepeat = 1;
          Ekmfa.State = EKMFA_RX_WAIT_FOR_NEUTRAL;          
        }
      }
      else
      {
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_TIMEOUT_MS)
        {
          Ekmfa.State = EKMFA_RX_IDLE;
          #if (EKMFA_RX_DEBUG > 0)
          Serial.print(millis16());Serial.println(F(": EKMFA_RX_CFRM_REPEAT -> EKMFA_RX_IDLE on Timeout"));
          #endif
        }
        else
        {
          if(PulseWidthUs < EKMFA_D_PULSE_WIDTH_US)
          {
            Ekmfa.State = EKMFA_RX_IDLE;
            #if (EKMFA_RX_DEBUG > 0)
            Serial.print(millis16());Serial.println(F(": EKMFA_RX_CFRM_REPEAT -> EKMFA_RX_IDLE on D area criteria"));
            #endif
          }
        }
      }
      break;
      
      case EKMFA_RX_CONFIRM_BURST:
      #if (EKMFA_RX_DEBUG > 0)
      Serial.print(millis16());Serial.print(F(": EKMFA_RX_CFRM_BURST Rx WidthUs="));Serial.print(PulseWidthUs);Serial.print(F(" ElapsedMs="));Serial.println(ElapsedMs16Since(Ekmfa.StartMs16));
      #endif
      if((Ekmfa.PosA && (PulseWidthUs > EKMFA_A_PULSE_WIDTH_US)) || (!Ekmfa.PosA && (PulseWidthUs < EKMFA_D_PULSE_WIDTH_US)))
      {
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_BURST_DURATION_MS)
        {
          Ekmfa.BurstCnt++;
          Ekmfa.State = EKMFA_RX_WAIT_FOR_NEUTRAL;
          #if (EKMFA_RX_DEBUG > 0)
          Serial.print(millis16());Serial.print(F(": EKMFA_RX_CFRM_BURST: Ekmfa.BurstCnt="));Serial.println(Ekmfa.BurstCnt);
          #endif
        }
      }
      else
      {
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_TIMEOUT_MS)
        {
          Ekmfa.State = EKMFA_RX_IDLE;
          #if (EKMFA_RX_DEBUG > 0)
          Serial.print(millis16());Serial.println(F(": EKMFA_RX_CFRM_BURST -> EKMFA_RX_IDLE"));
          #endif
        }        
      }
      break;

      case EKMFA_RX_WAIT_FOR_NEUTRAL:
      if((PulseWidthUs <= EKMFA_NB_PULSE_WIDTH_US) && (PulseWidthUs >= EKMFA_NC_PULSE_WIDTH_US))
      {
        Ekmfa.StartMs16 = millis16();
        Ekmfa.State = EKMFA_RX_CONFIRM_NEUTRAL;
      }
      else
      {
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_TIMEOUT_MS)
        {
          Ekmfa.State = EKMFA_RX_IDLE;
          #if (EKMFA_RX_DEBUG > 0)
          Serial.print(millis16());Serial.println(F(": EKMFA_RX_WAIT_FOR_NEUTRAL -> EKMFA_RX_IDLE"));
          #endif
        }        
      }
      break;

      case EKMFA_RX_CONFIRM_NEUTRAL:
      #if (EKMFA_RX_DEBUG > 0)
      Serial.print(millis16());Serial.print(F(": EKMFA_RX_CFRM_NEUTRAL Rx WidthUs="));Serial.println(PulseWidthUs);
      #endif
      if((PulseWidthUs <= EKMFA_NB_PULSE_WIDTH_US) && (PulseWidthUs >= EKMFA_NC_PULSE_WIDTH_US))
      {
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_INTER_BURST_DURATION_MS)
        {
          #if (EKMFA_RX_DEBUG > 0)
          if(!Ekmfa.InterBurstReached)
          {
            Serial.print(millis16());Serial.print(F(": EKMFA_RX_CFRM_NEUTRAL Rx Inter-burst reached"));Serial.print(F(" ElapsedMs="));Serial.println(ElapsedMs16Since(Ekmfa.StartMs16));
          }
          #endif
          Ekmfa.InterBurstReached = 1;
          if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_RESET_DURATION_MS)
          {
            #if (EKMFA_RX_DEBUG > 0)
            Serial.print(millis16());Serial.print(F(": EKMFA_RX_CONFIRM_NEUTRAL EKMFA_RX_RESET_DURATION_MS reached"));Serial.print(F(" ElapsedMs="));Serial.println(ElapsedMs16Since(Ekmfa.StartMs16));
            #endif
            if(Ekmfa.CmdRepeat)
            {
              Ekmfa.CmdRepeat = 0;
              Ret = Ekmfa.LastCmd;
              #if (EKMFA_RX_DEBUG > 0)
              Serial.print(F("Cmd = 0x"));Serial.print(Ret, HEX);Serial.println(F(" (Repeat Last)"));
              #endif
            }
            else
            {
              /* Reset time reached: Command validated */
              Ret = Ekmfa.BurstCnt | (Ekmfa.PosA? 0x80: 0); /* 0r'ed with 0x80 if type A pulses */
              Ekmfa.LastCmd = Ret;
              #if (EKMFA_RX_DEBUG > 0)
              Serial.print(F("Cmd = 0x"));Serial.println(Ret, HEX);
              #endif
            }
            Ekmfa.State = EKMFA_RX_IDLE;
          }
        }
      }
      else
      {
        if(Ekmfa.InterBurstReached)
        {
            if((Ekmfa.PosA && (PulseWidthUs > EKMFA_A_PULSE_WIDTH_US)) || (!Ekmfa.PosA && (PulseWidthUs < EKMFA_D_PULSE_WIDTH_US)))
            {
              Ekmfa.InterBurstReached = 0;
              Ekmfa.StartMs16 = millis16();
              Ekmfa.State = EKMFA_RX_CONFIRM_BURST;
              #if (EKMFA_RX_DEBUG > 0)
              Serial.print(millis16());Serial.print(F(": EKMFA_RX_CFRM_NEUTRAL Rx New burst WidthUs="));Serial.print(PulseWidthUs);Serial.println(F(" -> EKMFA_RX_CFRM_BURST"));
              #endif
            }
        }
        if(ElapsedMs16Since(Ekmfa.StartMs16) >= EKMFA_RX_TIMEOUT_MS)
        {
          Ekmfa.State = EKMFA_RX_IDLE;
          #if (EKMFA_RX_DEBUG > 0)
          Serial.print(millis16());Serial.println(F(": EKMFA_RX_CFRM_NEUTRAL -> EKMFA_RX_IDLE"));
          #endif
        }        
      }
      break;
    }
  }
  if(Ret && Ekmfa.MyMapInFlash)
  {
    /* An EKMFA Function map is defined: use it to return the invoked Function Id rather than the number of burst */
    for(Idx = 0; Idx < Ekmfa.MapSize; Idx++)
    {
      if((((uint8_t)pgm_read_byte(&Ekmfa.MyMapInFlash[(3 * Idx) + 2]) == 'A') && Ekmfa.PosA) || (((uint8_t)pgm_read_byte(&Ekmfa.MyMapInFlash[(3 * Idx) + 2]) == 'D') && !Ekmfa.PosA))
      {
        if((uint8_t)pgm_read_byte(&Ekmfa.MyMapInFlash[(3 * Idx) + 1]) == (Ret & 0x7F))
        {
          Ret = (uint8_t)pgm_read_byte(&Ekmfa.MyMapInFlash[(3 * Idx) + 0]);
          break;
        }
      }
    }
    if(Idx >= Ekmfa.MapSize) Ret = 0;
  }
  return(Ret);  
}

#if (EKMFA_RX_DEBUG > 0)
void EkmfaRxClass::debugProtocol(uint8_t OffOn)
{
    Ekmfa.Debug = !!OffOn;
}
#endif

#ifndef EKMFA_DEFAULT_DURATIONS
void EkmfaRxClass::setEepBaseAddr(uint16_t EepBaseAddr)
{
    GlobEepBaseAddr = EepBaseAddr;
}

uint16_t EkmfaRxClass::getEepTotalSize(void)
{
    return(2 * EKMFA_RX_EEP_WORD_NB);
}

uint16_t EkmfaRxClass::getEepDurationMs(uint8_t WordIdx)
{
    uint16_t EepAddr, EepWord;
    
    EepAddr = GlobEepBaseAddr + (WordIdx * 2);
#if not defined(ARDUINO_ARCH_RP2040)
    EepWord = (uint16_t)eeprom_read_word((const uint16_t *)(uint16_t)(EepAddr));
#else
	EepWord = EEPROM.read((uint16_t)(EepAddr));
#endif    
    return(EepWord);
}

void EkmfaRxClass::updateDurationMs(uint8_t WordIdx, uint16_t WordValue)
{
    uint16_t EepAddr;
    
    EepAddr = GlobEepBaseAddr + (WordIdx * 2);
#if not defined(ARDUINO_ARCH_RP2040)
    eeprom_update_word((uint16_t *)(uint16_t)(EepAddr), WordValue);
#else
	EEPROM.update((uint16_t)(EepAddr), WordValue);
	EEPROM.commit();
#endif
}
#endif

/*********************/
/* Private functions */
/*********************/
