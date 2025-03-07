#ifndef EKMFA_TX_H
#define EKMFA_TX_H

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

#include "Arduino.h"

#include <inttypes.h>
#include <Misclib.h>

#define EKMFA_TX_VERSION          1
#define EKMFA_TX_REVISION         0

#define EKMFA_TX_DEBUG            0 /* Put 0 to remove debug code and put 1 to add it */

enum {EKMFA_TX_RESET_DURATION_IDX = 0, EKMFA_TX_BURST_DURATION_IDX, EKMFA_TX_INTER_BURST_DURATION_IDX, EKMFA_TX_LAST_RECALL_DURATION_IDX, EKMFA_TX_EEP_WORD_NB};

/* OPTIMIZED BEHAVIOUR */
#define EKMFA_TX_LAST_CMD_REPEAT_SUPPORT   (1 << 0)
#define EKMFA_TX_CMD_OPTIMIZATION_SUPPORT  (1 << 1)

typedef struct{
  uint8_t
           State                :3,
           Debug                :1,
           CmdOptimization      :2,
           Reserved             :2;
  uint16_t StartUs;
}RcGenSt_t;

typedef struct{
  uint16_t  PulseWidthUs;
  uint8_t   Nb;
}BurstSt_t;

typedef struct{
  uint8_t  *SrcInCmd;
  uint8_t   LatchedInCmd[4];
  uint8_t   CmdMaxNb;
  uint16_t  InitStartMs;
  BurstSt_t Burst;
  uint8_t   MainState;
  uint8_t   PrevOutCmdIdx;
  uint8_t   OutCmdIdx;
  uint8_t   TxPin;
  RcGenSt_t RcGen;
}EkmfaSt_t;


class EkmfaTxClass
{
  private:
    EkmfaSt_t            Ekmfa;
  public:
    EkmfaTxClass();
    void                 begin(uint8_t TxPin, uint8_t *InCmd, uint8_t CmdMaxNb, uint8_t CmdOptimization = 0);
    void                 updateOrder(void);
    void                 process(void);
#if (EKMFA_TX_DEBUG > 0)
    void                 debugProtocol(uint8_t OffOn);
#endif
    static void          setEepBaseAddr(uint16_t EepBaseAddr);
    static uint16_t      getEepTotalSize(void);
    static uint16_t      getEepDurationMs(uint8_t WordIdx);
    static void          updateDurationMs(uint8_t WordIdx, uint16_t WordValue);
};

extern EkmfaTxClass    EkmfaTx;    /* Object externalisation */

#endif
