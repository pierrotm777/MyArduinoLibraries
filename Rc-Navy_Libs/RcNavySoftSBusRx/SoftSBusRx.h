/*
 English: by RC Navy (2023)
 =======
 <SoftSBusRx>: a library to read RC SBUS frame. No hardware inverter needed. All implementation done in software.
 http://p.loussouarn.free.fr
 V1.0 (12/02/2023): initial release

 English: by RC Navy (2023)
 =======
 <SoftSBusRx>: une bibliotheque pour lire les trames SBUS RC. Pas besoin d'inverseur materiel. Toute l'implementation est faite par logiciel.
 http://p.loussouarn.free.fr
 V1.0 (12/02/2023): release initiale
*/

#ifndef SOFT_SBUS_RX_H
#define SOFT_SBUS_RX_H

#include <Arduino.h>
#include <Rcul.h>

#include <TinyPinChange.h>

/* vvv Library configuration vvv */
#define SOFT_SBUS_RX_SINGLE_CHANNEL     0    /* When set, only a single channel is tracked (defined by the trackChId() method), this saves 20 bytes of RAM */
/* ^^^ Library configuration ^^^ */


#define SOFT_SBUS_RX_VERSION   1
#define SOFT_SBUS_RX_REVISION  0


#define SBUS_RX_CH17          (1 << 0)
#define SBUS_RX_CH18          (1 << 1)
#define SBUS_RX_FRAME_LOST    (1 << 2)
#define SBUS_RX_FAILSAFE      (1 << 3)

struct sbusInfo_t{
  uint8_t inv:         1, 
          byteIdx:     5, // 0 to 31
          shiftIdx:    2; // 0 to 3 (0 to 2 required)
};

class SoftSBusRxClass : public Rcul
{
  public:
    SoftSBusRxClass(void);

    void             begin(uint8_t pin, uint8_t Inv = 1);
    void             trackChId(uint8_t ChId); // ChId in [1..16]
    uint8_t          isSynchro(uint8_t SynchroClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free */ 
    uint8_t          flags(uint8_t FlagId);
    
    uint16_t         width_us(uint8_t ChId);
    uint16_t         rawData(uint8_t ChId);
    // public only for easy access by interrupt handlers
    static void      handle_interrupt();
    
    /* Rcul support */
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual uint16_t RculGetWidth_us(uint8_t Ch);
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = 255);

private:
};

extern SoftSBusRxClass SoftSBusRx; /* Object externalisation */

#endif

