#ifndef CONRAD_7_TX_H
#define CONRAD_7_TX_H

/*

 English: by RC Navy (2021)
 =======
 09/05/2021: V0.0 First release of the Conrad7Tx library
 <Conrad7Tx>: a library to simply control with 2 pins the 7 outputs of the 7-channel multi-switcher Ref 0115 541(Kit)/0231 517(Assembled/Tested) from Conrad
 IMPORTANT: a 100K pulldown resistor is required between K1 Signal and GND and K2 Signal and GND, otherwise, the neutral auto calibration may fail!
 26/06/2021: V1.1 Correction of a problem in case of too close commands
 http://p.loussouarn.free.fr/contact.html

 
 Francais: par RC Navy (2021)
 ========
 09/05/2021: V0.0 Premiere release de la bibliotheque Conrad7Tx
 <Conrad7Tx>: une bibliotheque pour controler simplement a partir de 2 broches les 7 sorties du module Commutateur multiple 7 canaux Ref 0115 541(kit)/0231 517(Assemble/teste) de chez Conrad
 IMPORTANT: une resistance de 100K est requise entre le Signal de K1 et GND ainsi qu'entre Signal de K2 et GND, sinon, l'auto calibration du neutre peut echouer!
 26/06/2021: V1.1 Correction d'un probleme en cas de commandes trop rapprochees
 http://p.loussouarn.free.fr/contact.html
 
*/

#include "Arduino.h"

#include <inttypes.h>
#include <Misclib.h>

#define CONRAD7TX_VERSION          1
#define CONRAD7TX_REVISION         1

#define CONRAD7TX_DEBUG            0 /* Put 0 to remove debug code and put 1 to add it */

typedef struct{
  uint16_t
              State     :3,
              Debug     :1,
              OutputIdx :4,
              ConfirmNb :4,
              Reserved  :4;
  uint16_t    StartUs;
}C7RcGenSt_t;

typedef struct{
  uint8_t    *SrcInCmd;
  uint16_t    InitStartMs;
  uint8_t     MainState;
  uint8_t     InCmd;
  uint8_t     OutCmd;
  uint8_t     Hpin;
  uint8_t     Vpin;
  C7RcGenSt_t RcGen;
}C7St_t;


class Conrad7TxClass
{
  private:
    C7St_t               C7;
  public:
    Conrad7TxClass();
    void                 begin(uint8_t Hpin, uint8_t Vpin, uint8_t *InCmd);
    void                 updateOrder(void);
    void                 process(void);
#if (CONRAD7TX_DEBUG > 0)
    void                 debugProtocol(uint8_t OffOn);
#endif
};

extern Conrad7TxClass    Conrad7Tx;    /* Object externalisation */

#endif
