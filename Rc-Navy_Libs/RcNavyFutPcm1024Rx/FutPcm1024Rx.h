#ifndef FUT_PCM_1024_RX_H
#define FUT_PCM_1024_RX_H

#include "Arduino.h"
#include <Rcul.h>

/*
 Update 06/08/2022: add checksum check for each of the 4 received pcm packets in a frame V0.1 -> V0.2

 English: by RC Navy (2022)
 =======
  <FutPcm1024Rx>: a library for decoding FUTABA PCM1024 protocol
  Input Pin is 8 (to be connected to the PCM1024 signal)
  It should run on all Arduino based on the ATmega328 (UNO, Nano, Pro Mini)

 Francais: par RC Navy (2022)
 ========
  <FutPcm1024Rx>: une bibliotheque pour decoder le protocol PCM1024 de FUTABA
  La broche d'entree est la 8 (a coonecter sur le signal PCM1024)
  Elle devrait fonctionner sur tous les Arduinos bases sur l'ATmega328 (UNO, Nano, Pro Mini)
*/

/* Library Version/Revision */
#define FUT_PCM_1024_RX_VERSION       0
#define FUT_PCM_1024_RX_REVISION      2

void     FutPcm1024Rx_begin(void);
uint8_t  FutPcm1024Rx_available(void);
uint16_t FutPcm1024Rx_channelRaw(uint8_t ChId);
uint16_t FutPcm1024Rx_channelWidthUs(uint8_t ChId);

#endif
