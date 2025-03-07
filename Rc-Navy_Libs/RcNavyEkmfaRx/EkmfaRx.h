#ifndef EKMFA_RX_H
#define EKMFA_RX_H

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

#include "Arduino.h"

#include <inttypes.h>
#if not defined(ARDUINO_ARCH_RP2040)
#include <avr/eeprom.h>
#else
#include <EEPROM.h>
#endif
#include <Rcul.h>
#include <Misclib.h>

#define EKMFA_RX_VERSION          1
#define EKMFA_RX_REVISION         1

//#define EKMFA_DEFAULT_DURATIONS   // Comment this to allow definition of custom timings (in EEPROM)

#ifdef EKMFA_DEFAULT_DURATIONS
#define EKMFA_RX_DEFAULT_RESET_DURATION_MS        200//(3*100)
#define EKMFA_RX_DEFAULT_BURST_DURATION_MS        50//(1*50)
#define EKMFA_RX_DEFAULT_INTER_BURST_DURATION_MS  50//(1*50)
#define EKMFA_RX_DEFAULT_LAST_RECALL_DURATION_MS  250//(4*150)
#endif

#define EKMFA_RX_DEBUG            1 /* Put 0 to remove debug code and put 1 to add it */

#define A_AREA                                  'A'
#define D_AREA                                  'D'
#define EKMFA_FCT_MAP_TBL(MyMap)                const uint8_t MyMap[] PROGMEM
#define EKMFA_FCT_POS(FctId, BurstNb, AD_area)  (FctId), (BurstNb), (AD_area)

#define EKMFA_A_AREA_x(n)                       (0x80 + (n)) // Returns the EKMFA raw code for the n x pulse(s) in A area
#define EKMFA_D_AREA_x(n)                       (0x00 + (n)) // Returns the EKMFA raw code for the n x pulse(s) in A area

enum {EKMFA_RX_RESET_DURATION_IDX = 0, EKMFA_RX_BURST_DURATION_IDX, EKMFA_RX_INTER_BURST_DURATION_IDX, EKMFA_RX_LAST_RECALL_DURATION_IDX, EKMFA_RX_EEP_WORD_NB};

typedef struct{
  Rcul          *_Rcul;
  const uint8_t *MyMapInFlash;
  uint16_t       MapSize           :5, // 1 to 30 -> 5 bits (0 -> 31)
                 BurstCnt          :5,
                 Ch                :6;
  uint8_t        LastCmd;
  uint16_t       StartMs16;
  uint8_t        State             :4,
                 PosA              :1,
                 InterBurstReached :1,
                 CmdRepeat         :1,
                 Debug             :1;
}EkmfaRxSt_t;


class EkmfaRxClass
{
  private:
    EkmfaRxSt_t          Ekmfa;
  public:
    EkmfaRxClass();
    void                 begin(Rcul *Rcul, uint8_t Ch = RCUL_NO_CH, const uint8_t *MyMapInFlash = NULL, uint8_t MapSize = 0);
    void                 reassignRculSrc(Rcul *Rcul, uint8_t Ch = RCUL_NO_CH); /* Marginal use (do not use it, if you do not know what is it for) */
    uint8_t              process(void);
#if (EKMFA_RX_DEBUG > 0)
    void                 debugProtocol(uint8_t OffOn);
#endif
#ifndef EKMFA_DEFAULT_DURATIONS
    static void          setEepBaseAddr(uint16_t EepBaseAddr);
    static uint16_t      getEepTotalSize(void);
    static uint16_t      getEepDurationMs(uint8_t WordIdx);
    static void          updateDurationMs(uint8_t WordIdx, uint16_t WordValue);
#endif
};

extern EkmfaRxClass    EkmfaRx;    /* Object externalisation */

#endif
