#include "EkmfaTx.h"

/*

 English: by RC Navy (2021)
 =======
 Update 05/06/2021: First release of the EkmfaTx library
 <EkmfaTx>: a library to simply generate up to 30 EKMFA commands. Pulse counting is performed automatically by the library.
 http://p.loussouarn.free.fr/contact.html

 
 Francais: par RC Navy (2021)
 ========
 Update 05/06/2021: Premiere release de la bibliotheque EkmfaTx
 <EkmfaTx>: une bibliotheque pour generer simplement jusqu'a 30 commandes EKMFA. Le comptage des impulsions est realise automatiquement par la bibliotheque.
 http://p.loussouarn.free.fr/contact.html
 
*/

/*
                                                      Counter=1xA/D                          Counter=2xA/D
              ________________________ ______________________ _________________ ______________________ ________________________
Duration:           Reset ms          |       Burst ms       |  Inter-Burst ms |      Burst ms        |  Inter-Burst ms 
Pulse Width: <-------With=1500us-----><---Width=A/D_Width---><--With=1500us---><---Width=A/D_Width---><-------With=1500us----->
                                      <-------------------------------------------------------------->
                                      |                                                              |
                               Start of sequence                                                End of sequence
*/

static uint16_t GlobEepBaseAddr = 0;

/* Private function prototypes */
static uint8_t BufIsBusy(uint8_t *Buf, uint8_t BufByteSize);

/* Global variables */
EkmfaTxClass    EkmfaTx = EkmfaTxClass();


#define NEUTRAL_US                            1500
#define EXC_MAX_US                            500

#define EKMFA_A_D_EXC_PERCENT                 80
#define EKMFA_B_C_EXC_PERCENT                 40

#define EKMFA_A_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US + ((EKMFA_A_D_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_B_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US + ((EKMFA_B_C_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_C_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US - ((EKMFA_B_C_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100))
#define EKMFA_D_PULSE_WIDTH_US                (uint16_t)(NEUTRAL_US - ((EKMFA_A_D_EXC_PERCENT * (uint32_t)EXC_MAX_US) / 100)) /* Unused: redundant with EKMFA_B_PULSE_US */

#define EKMFA_TX_RESET_DURATION_MS            EkmfaTxClass::getEepDurationMs(EKMFA_TX_RESET_DURATION_IDX)
#define EKMFA_TX_BURST_DURATION_MS            EkmfaTxClass::getEepDurationMs(EKMFA_TX_BURST_DURATION_IDX)
#define EKMFA_TX_INTER_BURST_DURATION_MS      EkmfaTxClass::getEepDurationMs(EKMFA_TX_INTER_BURST_DURATION_IDX)
#define EKMFA_TX_LAST_RECALL_DURATION_MS      EkmfaTxClass::getEepDurationMs(EKMFA_TX_LAST_RECALL_DURATION_IDX)

#define EKMFA_MAX_CMD_NB                      30 /* Do NOT change this */

#define EKMFA_CMD_BYTE_NB(CmdMaxNb)           (((CmdMaxNb) + 7) / 8)
#define EKMFA_CMD_BYTE_MAX_IDX(CmdMaxNb)      (EKMFA_CMD_BYTE_NB(CmdMaxNb) - 1)

#define LAST_CMD_REPEAT_SUPPORT(CmdOpt)       ((CmdOpt) & EKMFA_TX_LAST_CMD_REPEAT_SUPPORT)
#define CMD_OPTIMIZATION_SUPPORT(CmdOpt)      ((CmdOpt) & EKMFA_TX_CMD_OPTIMIZATION_SUPPORT)

#define NORMAL_PULSE_WIDTH_US(CmdIdx)         (((CmdIdx) < (EKMFA_MAX_CMD_NB / 2))? EKMFA_A_PULSE_WIDTH_US: EKMFA_D_PULSE_WIDTH_US)
#define OPTIMIZED_PULSE_WIDTH_US(CmdIdx)      (((CmdIdx) & 1)? EKMFA_D_PULSE_WIDTH_US: EKMFA_A_PULSE_WIDTH_US)

#define NORMAL_BURST_NB(CmdIdx)               (((CmdIdx) < (EKMFA_MAX_CMD_NB / 2))? ((CmdIdx) + 1): ((CmdIdx) - (EKMFA_MAX_CMD_NB / 2) + 1))
#define OPTIMIZED_BURST_NB(CmdIdx)            (((CmdIdx) & 1)? ((((CmdIdx) - 1) / 2) + 1): (((CmdIdx) / 2) + 1))        

#define BURST_PULSE_WIDTH_US(CmdIdx, CmdOpt)  ((CMD_OPTIMIZATION_SUPPORT(CmdOpt))? OPTIMIZED_PULSE_WIDTH_US(CmdIdx): NORMAL_PULSE_WIDTH_US(CmdIdx))
#define BURST_NB(CmdIdx, CmdOpt)              ((CMD_OPTIMIZATION_SUPPORT(CmdOpt))? OPTIMIZED_BURST_NB(CmdIdx): NORMAL_BURST_NB(CmdIdx))

#define READ_CMD_BIT(CmdIdx)                  bitRead(Ekmfa.LatchedInCmd[EKMFA_CMD_BYTE_MAX_IDX(Ekmfa.CmdMaxNb) - ((CmdIdx) / 8)], ((CmdIdx) % 8))
#define CLEAR_CMD_BIT(CmdIdx)                 bitClear(Ekmfa.LatchedInCmd[EKMFA_CMD_BYTE_MAX_IDX(Ekmfa.CmdMaxNb) - ((CmdIdx) / 8)], ((CmdIdx) % 8));\
                                              bitClear(Ekmfa.SrcInCmd[EKMFA_CMD_BYTE_MAX_IDX(Ekmfa.CmdMaxNb) - ((CmdIdx) / 8)], ((CmdIdx) % 8))

enum {RC_GEN_WAIT_FOR_SYNCHRO = 0, RC_GEN_START_OF_PULSE, RC_GEN_WAIT_FOR_END_OF_PULSE};

enum {EKMFA_START_INTER_CMD_SEQ_DELAY = 0,
      EKMFA_WAIT_FOR_END_OF_INTER_CMD_SEQ_DELAY,  EKMFA_STATE_GENERATE_WAITING_NEUTRAL_PULSE,
      EKMFA_STATE_UPDATE_ORDER,   
      EKMFA_STATE_START_BURST,                    EKMFA_STATE_WAIT_FOR_END_OF_BURST,
      EKMFA_STATE_START_INTER_BURST,              EKMFA_STATE_WAIT_FOR_END_OF_INTER_BURST,
      EKMFA_STATE_WAIT_FOR_END_OF_RECALL_LAST_CMD
};

/* Constructor */
EkmfaTxClass::EkmfaTxClass()
{

}

/* EkmfaTx.begin() shall be called in the setup() */
void EkmfaTxClass::begin(uint8_t TxPin, uint8_t *InCmd, uint8_t CmdMaxNb, uint8_t CmdOptimization /*= 0*/)
{
  Ekmfa.TxPin  = TxPin;
  Ekmfa.SrcInCmd = InCmd;
  Ekmfa.CmdMaxNb = (CmdMaxNb > EKMFA_MAX_CMD_NB)? EKMFA_MAX_CMD_NB: CmdMaxNb;
  memset(&Ekmfa.LatchedInCmd, 0, EKMFA_CMD_BYTE_NB(Ekmfa.CmdMaxNb));
  pinMode(Ekmfa.TxPin, OUTPUT);
  Ekmfa.PrevOutCmdIdx         = 255;
  Ekmfa.OutCmdIdx             = 0;
  Ekmfa.MainState             = EKMFA_START_INTER_CMD_SEQ_DELAY;
  Ekmfa.RcGen.State           = RC_GEN_WAIT_FOR_SYNCHRO;
  Ekmfa.RcGen.CmdOptimization = CmdOptimization;
}

/* EkmfaTx.updateOrder() shall be called every #20ms */
void EkmfaTxClass::updateOrder(void) /* This method shall be called every #20 ms */
{
    uint8_t  BitIdx;
    uint16_t NormalCmdTimeMs;
    
    if(Ekmfa.RcGen.State == RC_GEN_WAIT_FOR_SYNCHRO)
    {
        switch(Ekmfa.MainState)
        {
            case EKMFA_START_INTER_CMD_SEQ_DELAY:
            Ekmfa.InitStartMs = millis16();
            Ekmfa.Burst.PulseWidthUs = NEUTRAL_US;
            Ekmfa.MainState = EKMFA_WAIT_FOR_END_OF_INTER_CMD_SEQ_DELAY;
            break;
            
            case EKMFA_WAIT_FOR_END_OF_INTER_CMD_SEQ_DELAY:
            if(ElapsedMs16Since(Ekmfa.InitStartMs) >= EKMFA_TX_RESET_DURATION_MS)
            {
                Ekmfa.MainState = EKMFA_STATE_GENERATE_WAITING_NEUTRAL_PULSE;
            }
            break;
            
            case EKMFA_STATE_GENERATE_WAITING_NEUTRAL_PULSE:
            Ekmfa.Burst.PulseWidthUs = NEUTRAL_US;
            Ekmfa.MainState = EKMFA_STATE_UPDATE_ORDER;
            break;
            
            case EKMFA_STATE_UPDATE_ORDER:
            if(!BufIsBusy(Ekmfa.LatchedInCmd, EKMFA_CMD_BYTE_NB(Ekmfa.CmdMaxNb)))
            {
                memcpy(&Ekmfa.LatchedInCmd, Ekmfa.SrcInCmd, EKMFA_CMD_BYTE_NB(Ekmfa.CmdMaxNb));
            }
            if(BufIsBusy(Ekmfa.LatchedInCmd, EKMFA_CMD_BYTE_NB(Ekmfa.CmdMaxNb)))
            {
#if (EKMFA_TX_DEBUG > 0)
                if(Ekmfa.RcGen.Debug)
                {
                    Serial.print(F("Ekmfa.LatchedInCmd=0x"));
                    for(int8_t Idx = 0; Idx <= EKMFA_CMD_BYTE_MAX_IDX(Ekmfa.CmdMaxNb); Idx++)
                    {
                        if(Ekmfa.LatchedInCmd[Idx] < 16) Serial.print(F("0"));
                        Serial.print(Ekmfa.LatchedInCmd[Idx], HEX);
                    }
                    Serial.println();
                }
#endif
                /* At least one bit is set in Ekmfa.LatchedInCmd[] */
                for(BitIdx = 0; BitIdx < Ekmfa.CmdMaxNb; BitIdx++)
                {
                    if(READ_CMD_BIT(BitIdx)) break;
                }
                Ekmfa.OutCmdIdx = BitIdx;
#if (EKMFA_TX_DEBUG > 0)
                if(Ekmfa.RcGen.Debug)
                {
                    Serial.print(F("Ekmfa.OutCmdIdx="));Serial.println(Ekmfa.OutCmdIdx);
                }
#endif
                Ekmfa.Burst.PulseWidthUs = BURST_PULSE_WIDTH_US(Ekmfa.OutCmdIdx, Ekmfa.RcGen.CmdOptimization);
                Ekmfa.Burst.Nb           = BURST_NB(Ekmfa.OutCmdIdx, Ekmfa.RcGen.CmdOptimization);
                if(LAST_CMD_REPEAT_SUPPORT(Ekmfa.RcGen.CmdOptimization))
                {
                    NormalCmdTimeMs = (Ekmfa.Burst.Nb == 1)? EKMFA_TX_BURST_DURATION_MS: ((EKMFA_TX_BURST_DURATION_MS * Ekmfa.Burst.Nb) + (EKMFA_TX_INTER_BURST_DURATION_MS * (Ekmfa.Burst.Nb - 1)));
                    if(NormalCmdTimeMs < EKMFA_TX_LAST_RECALL_DURATION_MS)
                    {
                        Ekmfa.PrevOutCmdIdx = 255; /* Last cmd recall takes more time than normal command */
                    }
#if (EKMFA_TX_DEBUG > 0)
                    Serial.print(F("NormalCmdTimeMs="));Serial.print(NormalCmdTimeMs);Serial.print(F(" RepeatCmdTimeMs="));Serial.println(EKMFA_TX_LAST_RECALL_DURATION_MS);
#endif
                }
                else
                {
                    Ekmfa.PrevOutCmdIdx = 255;
                }
                
                if(Ekmfa.OutCmdIdx != Ekmfa.PrevOutCmdIdx)
                {
                    Ekmfa.PrevOutCmdIdx      = Ekmfa.OutCmdIdx;
                    Ekmfa.MainState          = EKMFA_STATE_START_BURST;
#if (EKMFA_TX_DEBUG > 0)
                    if(Ekmfa.RcGen.Debug)
                    {
                        Serial.print(millis16());Serial.println(": New burst!");
                    }
#endif
                }
                else
                {
                    /* Recall the last command */
                    Ekmfa.Burst.PulseWidthUs = EKMFA_C_PULSE_WIDTH_US;
                    Ekmfa.InitStartMs        = millis16();
                    Ekmfa.MainState          = EKMFA_STATE_WAIT_FOR_END_OF_RECALL_LAST_CMD;
#if (EKMFA_TX_DEBUG > 0)
                    if(Ekmfa.RcGen.Debug)
                    {
                        Serial.print(millis16());Serial.print(F(": Recall last cmd (Ekmfa.OutCmdIdx="));Serial.print(Ekmfa.OutCmdIdx);Serial.println(F(")"));
                    }
#endif
                }
            }
            else
            {
                Ekmfa.MainState = EKMFA_STATE_GENERATE_WAITING_NEUTRAL_PULSE;
            }
            break;
            
            case EKMFA_STATE_START_BURST:
            Ekmfa.InitStartMs        = millis16();
            Ekmfa.Burst.PulseWidthUs = BURST_PULSE_WIDTH_US(Ekmfa.OutCmdIdx, Ekmfa.RcGen.CmdOptimization);
            Ekmfa.MainState          = EKMFA_STATE_WAIT_FOR_END_OF_BURST;
            break;
            
            case EKMFA_STATE_WAIT_FOR_END_OF_BURST:
            if(ElapsedMs16Since(Ekmfa.InitStartMs) >= EKMFA_TX_BURST_DURATION_MS)
            {
                /* Burst finished */
                Ekmfa.Burst.Nb--;
                if(Ekmfa.Burst.Nb)
                {
                    Ekmfa.InitStartMs = millis16();
                    Ekmfa.Burst.PulseWidthUs = NEUTRAL_US;
                    Ekmfa.MainState = EKMFA_STATE_START_INTER_BURST;
                }
                else
                {
#if (EKMFA_TX_DEBUG > 0)
                    if(Ekmfa.RcGen.Debug)
                    {
                        Serial.println("Last burst done!");
                    }
#endif
                    /* Burst Count OK */
                    CLEAR_CMD_BIT(Ekmfa.OutCmdIdx);
                    //bitClear(Ekmfa.SrcInCmd[(EKMFA_CMD_BYTE_MAX_IDX(Ekmfa.CmdMaxNb) - ((Ekmfa.OutCmdIdx) / 8))], ((Ekmfa.OutCmdIdx) % 8));
                    Ekmfa.MainState = EKMFA_START_INTER_CMD_SEQ_DELAY;
                }
            }
            break;

            case EKMFA_STATE_START_INTER_BURST:
            Ekmfa.Burst.PulseWidthUs = NEUTRAL_US;
            Ekmfa.MainState = EKMFA_STATE_WAIT_FOR_END_OF_INTER_BURST;
            break;
            
            case EKMFA_STATE_WAIT_FOR_END_OF_INTER_BURST:
            if(ElapsedMs16Since(Ekmfa.InitStartMs) >= EKMFA_TX_INTER_BURST_DURATION_MS)
            {
#if (EKMFA_TX_DEBUG > 0)
                    if(Ekmfa.RcGen.Debug)
                    {
                        Serial.print(millis16());Serial.println(": New burst!");
                    }
#endif
                /* Inter-Burst finished -> Generate next burst */
                Ekmfa.MainState          = EKMFA_STATE_START_BURST;
            }
            break;
            
            case EKMFA_STATE_WAIT_FOR_END_OF_RECALL_LAST_CMD:
            if(ElapsedMs16Since(Ekmfa.InitStartMs) >= EKMFA_TX_LAST_RECALL_DURATION_MS)
            {
                CLEAR_CMD_BIT(Ekmfa.OutCmdIdx);
                Ekmfa.MainState = EKMFA_START_INTER_CMD_SEQ_DELAY;
            }
            break;

            default:
            /* Should never arrive here */
            Ekmfa.MainState = EKMFA_START_INTER_CMD_SEQ_DELAY;
            break;
        }
        Ekmfa.RcGen.State = RC_GEN_START_OF_PULSE; /* Generate next pulse */
    }
}

/* EkmfaTx.process() shall be in the loop() (To make it work, blocking functions such as delay() are forbidden in the loop()) */
void EkmfaTxClass::process(void) // This method SHALL be called in the loop()
{
  
  switch(Ekmfa.RcGen.State)
  {
    case RC_GEN_WAIT_FOR_SYNCHRO:
    /* Do nothing, just wait */
    break;
    
    case RC_GEN_START_OF_PULSE:
#if (EKMFA_TX_DEBUG > 0)
    if(Ekmfa.RcGen.Debug)
    {
        static uint16_t PrevPulseWidthUs = 0;
        if(Ekmfa.Burst.PulseWidthUs != PrevPulseWidthUs)
        {
            PrevPulseWidthUs = Ekmfa.Burst.PulseWidthUs;
            Serial.print(millis());Serial.print(F(": W="));Serial.println(Ekmfa.Burst.PulseWidthUs);
        }
    }
#endif
    Ekmfa.RcGen.StartUs = micros16();
    Ekmfa.RcGen.State = RC_GEN_WAIT_FOR_END_OF_PULSE;
    digitalWrite(Ekmfa.TxPin, HIGH);
    break;
    
    case RC_GEN_WAIT_FOR_END_OF_PULSE:
    if(ElapsedUs16Since(Ekmfa.RcGen.StartUs) >= Ekmfa.Burst.PulseWidthUs)
    {
        digitalWrite(Ekmfa.TxPin, LOW);
        Ekmfa.RcGen.State = RC_GEN_WAIT_FOR_SYNCHRO;
    }
    break;
  }
  
}

#if (EKMFA_TX_DEBUG > 0)
void EkmfaTxClass::debugProtocol(uint8_t OffOn)
{
    Ekmfa.RcGen.Debug = !!OffOn;
}
#endif

void EkmfaTxClass::setEepBaseAddr(uint16_t EepBaseAddr)
{
    GlobEepBaseAddr = EepBaseAddr;
}

uint16_t EkmfaTxClass::getEepTotalSize(void)
{
    return(2 * EKMFA_TX_EEP_WORD_NB);
}

#if defined(ARDUINO_ARCH_RP2040)
#include <EEPROM.h>
#endif
uint16_t EkmfaTxClass::getEepDurationMs(uint8_t WordIdx)
{
    uint16_t EepAddr, EepWord;
    
    EepAddr = GlobEepBaseAddr + (WordIdx * 2);
#if not defined(ARDUINO_ARCH_RP2040)
    EepWord = (uint16_t)eeprom_read_word((const uint16_t *)(uint16_t)(EepAddr));
#else
	EepWord = (uint16_t)EEPROM.read((uint16_t)(EepAddr));
#endif    
    return(EepWord);
}

void EkmfaTxClass::updateDurationMs(uint8_t WordIdx, uint16_t WordValue)
{
    uint16_t EepAddr;
    
    EepAddr = GlobEepBaseAddr + (WordIdx * 2);
#if not defined(ARDUINO_ARCH_RP2040)
    eeprom_update_word((uint16_t *)(uint16_t)(EepAddr), WordValue);
#else
	EepWord = EEPROM.update((uint16_t)(EepAddr), WordValue);
	EEPROM.commit();
#endif
}


/*********************/
/* Private functions */
/*********************/

static uint8_t BufIsBusy(uint8_t *Buf, uint8_t BufByteSize)
{
    uint8_t ByteIdx;
    
    for(ByteIdx = 0; ByteIdx < BufByteSize; ByteIdx++)
    {
        if(Buf[ByteIdx]) break;
    }
    return(ByteIdx < BufByteSize);
}
