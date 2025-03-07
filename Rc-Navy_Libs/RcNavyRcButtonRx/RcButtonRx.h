/*
 English: by RC Navy (2024)
 =======
 <RcButtonRx> is a library designed to read RC pulse signal to make actions from a keyboard (push-buttons + resistors) connected to a free channel of an RC transmitter.
 This library manages the mandatory calibration phase (an hardware or software serial interface is needed).
 The action associated to each push-button can be set in pulse mode (sometime called memory mode).
 This <RcButtonRx> library is intended to facilitate the design of a decoder placed at RC receiver side.
 With this library, the exploitation of the commands from the push-buttons are greatly facilitated.
 In case of lost signal, all the commands are set to 0 after 2 seconds (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: initial release
 V1.1: (dd/mm/yyyy) Wiil be the next Release

 Francais: par RC Navy (2024)
 ========
<RcButtonRx> est une bibliotheque concue pour lire les largeurs d'impulsions RC pour faire des actions à partir d'un clavier (boutons-poussoirs + resistances) connecté
a une voie libre d'un emetteur RC.
Cette bibliotheque gere la phase nécessaire de calibration (une interface serie hardware ou software est requise).
La bibliotheque <RcButtonRx> est destinee a faciliter la conception de decodeur place cote recepteur RC.
Avec cette bibliotheque, l'exploitation des commandes depuis les boutons-poussoirs est grandement facilitee.
En cas de perte de signal, toutes les commandes sont mises à 0 apres 2 secondes (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: release initiale
 V1.1: (dd/mm/yyyy) Sera la prochaine Release
*/

#ifndef RC_BUTTON_RX_H
#define RC_BUTTON_RX_H

#include "Arduino.h"
#include <inttypes.h>
#include <Rcul.h>
#include <Misclib.h>

#define RC_BUTTON_RX_EEPROM_BYTES(ButtonNb)     (((ButtonNb) + 1) * 2) /* Number of bytes to be stored in EPPROM for calibration (pulse width) (+1 for Mode) */

#define RC_BUTTON_RX_VERSION                    1
#define RC_BUTTON_RX_REVISION                   0

typedef struct{
    Stream      *MyStream;
    Rcul        *MyRcul;
    uint16_t     EepromBaseAddr;
    uint16_t     ButtonNb          :4, /* 1 to 15 max */
                 ButtonIdx         :4, /* 0 to 14 max */
                 InProgButtonIdx   :4, /* 0 to 14 max */
                 ButAcqState       :2,
                 Calibration       :1,
                 CrLineTerm        :1;
    uint8_t      ChId              :5,
                 ClientIdx         :3; /* 0 to 7  max */
    uint8_t      RcPulseValidNb    :3, /*  3 */
                 RcPulseInterCmdNb :5; /* 20 */
    uint16_t     Outputs;        /* New status of the Outputs associated to the buttons (Bit0 is Button1, Bit1 is Button2, etc...) */
}RcButtonRxSt_t;

class RcButtonRx
{
  public:
    RcButtonRx();
    void             begin(Stream *MyStream, uint8_t CrLineTerm, Rcul *MyRcul, uint8_t ChId, uint8_t ButtonNb, uint8_t ClientIdx = 5, uint16_t EepromBaseAddr = 0);
    uint8_t          getStoredEepromBytes(void);
    void             setPulseMap(uint16_t PulseMap); /* Set the Pulse Map Mode (For all the buttons at a time) */
    void             setPulseMode(uint8_t ButtonId, uint8_t PulseMode); /* PulseMode=1 -> Pulse Mode, PulseMode=0 -> Normal Mode */
    uint8_t          isPulseMode(uint8_t ButtonId);
    void             enterInCalibrationMode(void);
    uint8_t          isInCalibrationMode(void);
    uint16_t         process(void);
    void             displayButtonPulseWidth(void);
    private:
    RcButtonRxSt_t   _Priv;
    void             eol(void);
    uint8_t          getPushButtonIdx(uint16_t WidthUs);
};

#endif
