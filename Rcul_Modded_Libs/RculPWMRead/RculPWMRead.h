#ifndef RculPWMRead_RX_H
#define RculPWMRead_RX_H
/*
Francais: par RC Navy (2012-2022)
 ========
 RculPWMRead est basé sur la librairie SoftRcPulseIn de P. LOUSSOUARN.
 RculPWMRead n'utilise pas TinyPinChange bloquant pour le RP2040 entre autre.
 <RculPWMRead>: une bibliotheque asynchrone pour lire les largeurs d'impulsions des Radio-Commandes standards.
 Cette bibliotheque est une version non bloquante de pulseIn().
 
 http://p.loussouarn.free.fr
 
*/
#include "Arduino.h"
#include <inttypes.h>
#include <Rcul.h>


#define RculPWMRead_IN_VERSION                    1
#define RculPWMRead_IN_REVISION                   0

typedef struct{
  uint8_t
          VirtualPortIdx  :2,
          Inv             :1,
          Reserved        :5;
}InfoPulseInSt_t;

class RculPWMRead : public Rcul
{
  public:
    RculPWMRead(uint8_t Inv = 0);
	static void       RculPWMReadInterrupt0ISR(void);
    int8_t            attach(uint8_t Pin, uint16_t PulseMin_us = 600, uint16_t PulseMax_us = 2400);
    void              detach(void);
    uint8_t           available(uint8_t ClientIdx = 7);
    uint8_t           isSynchro(uint8_t ClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free for other clients */
    uint8_t           timeout(uint8_t TimeoutMs, uint8_t *State);
    uint16_t          width_us();
    uint32_t          start_us();
    /* Rcul support */
    virtual uint8_t   RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual uint16_t  RculGetWidth_us(uint8_t Ch);
    virtual void      RculSetWidth_us(uint16_t Width_us, uint8_t Ch = RCUL_NO_CH);
    private:
    class  RculPWMRead *prev;
    static RculPWMRead *last;
    uint8_t           _Pin;
    InfoPulseInSt_t   _Info;
    uint16_t          _Min_us;
    uint16_t          _Max_us;
    volatile uint32_t _Start_us;
    volatile uint16_t _Width_us;
    volatile uint8_t  _Available;
    uint8_t           _LastTimeStampMs;
};

/*******************************************************/
/* Application Programming Interface (API) en Francais */
/*******************************************************/

/*      Methodes en Francais                            English native methods */
#define attache                                         attach
#define disponible                                      available
#define largeur_us                                      width_us

#endif
