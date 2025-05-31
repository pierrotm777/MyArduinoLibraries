/*
 English: by RC Navy (2024-2025)
 =======
 <RcButtonRx> is a library designed to read RC pulse signal to make actions from a keyboard (push-buttons + resistors) connected to a free channel of an RC transmitter.
 This library manages the mandatory calibration phase (an hardware or software serial interface is needed).
 The action associated to each push-button can be set in pulse mode (sometime called memory mode).
 This <RcButtonRx> library is intended to facilitate the design of a decoder placed at RC receiver side.
 With this library, the exploitation of the commands from the push-buttons are greatly facilitated.
 In case of lost signal, all the commands are set to 0 after 2 seconds (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: initial release
 V1.1: (22/12/2024) One wire serial support added and #include <avr/eeprom.h> added and tolerance reduction from -/+20 to -/+15
 V1.2: (08/05/2025) Support for Teensy 4.0 and RP2040 targets, CRLF line terminator fixed (was sending only LF)
 V1.3: (30/05/2025) Added call-back function support when exiting from calibration (can be use to update non-volatile memory checksum).
                    This feature is dependent of the "#define RC_BUTTON_RX_ON_EXIT_CAL" compilation directive in RcButtonRx.h.

 Francais: par RC Navy (2024-2025)
 ========
<RcButtonRx> est une bibliotheque concue pour lire les largeurs d'impulsions RC pour faire des actions à partir d'un clavier (boutons-poussoirs + resistances) connecté
a une voie libre d'un emetteur RC.
Cette bibliotheque gere la phase nécessaire de calibration (une interface serie hardware ou software est requise).
La bibliotheque <RcButtonRx> est destinee a faciliter la conception de decodeur place cote recepteur RC.
Avec cette bibliotheque, l'exploitation des commandes depuis les boutons-poussoirs est grandement facilitee.
En cas de perte de signal, toutes les commandes sont mises à 0 apres 2 secondes (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: release initiale
 V1.1: (22/12/2024) Ajout support pour serial sur un seul fil et ajout #include <avr/eeprom.h> et reduction tolerance de -/+20 à -/+15
 V1.2: (08/05/2025) Ajout support pour cibles Teensy 4.0 et RP2040, le terminateur de ligne CRLF est corrige (il n'envoyait que LF)
 V1.3: (30/05/2025) Ajout support pour fonction call-back a la sortie du mode de calibration (peut être utilisé pour mettre à jour le checksum d'une memoire non volatile).
                    Cette fonctionnalité est dependante de la directive de compilation "#define RC_BUTTON_RX_ON_EXIT_CAL" dans RcButtonRx.h
*/

#ifndef RC_BUTTON_RX_H
#define RC_BUTTON_RX_H

#include "Arduino.h"
#include <inttypes.h>
#include <Rcul.h>
#include <Misclib.h>
#if defined(__AVR__)
#include <avr/eeprom.h>
#endif

/* vvv Library configuration vvv */
//#define RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT     // Uncomment this line to enable serial in one wire mode (tx and rx share the same pin)
#define RC_BUTTON_RX_ON_EXIT_CAL                // Uncomment this line to declare a callback funtion when exiting from calibration
/* ^^^ Library configuration ^^^ */

#define RC_BUTTON_RX_EEPROM_BYTES(ButtonNb)     (((ButtonNb) + 1) * 2) /* Number of bytes to be stored in EPPROM for calibration (pulse width) (+1 for Mode) */

#define RC_BUTTON_RX_VERSION                    1
#define RC_BUTTON_RX_REVISION                   3

typedef struct{
    Stream      *MyStream;
    Rcul        *MyRcul;
#ifdef RC_BUTTON_RX_ON_EXIT_CAL
    void        (*onExitAction)(void); // Callback used for example to update non-volatile checksum if needed
#endif
    #ifdef RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT
    void        (*TxMode)(void);
    void        (*RxMode)(void);
    #endif
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
    uint16_t     Outputs;        /* New staus of the Outputs associated to the buttons (Bit0 is Button1, Bit1 is Button2, etc...) */
}RcButtonRxSt_t;

class RcButtonRx
{
  public:
    RcButtonRx();
    void             begin(Stream *MyStream,
                          #ifdef RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT
                          void (*TxMode)(void), void (*RxMode)(void),
                          #endif
                          uint8_t CrLineTerm, Rcul *MyRcul, uint8_t ChId, uint8_t ButtonNb, uint8_t ClientIdx = 5, uint16_t EepromBaseAddr = 0);
    void             txMode(void);
    void             rxMode(void);
    uint8_t          getStoredEepromBytes(void);
    void             setPulseMap(uint16_t PulseMap); /* Set the Pulse Map Mode (For all the buttons at a time) */
    void             setPulseMode(uint8_t ButtonId, uint8_t PulseMode); /* PulseMode=1 -> Pulse Mode, PulseMode=0 -> Normal Mode */
    uint8_t          isPulseMode(uint8_t ButtonId);
    void             enterInCalibrationMode(void);
    uint8_t          isInCalibrationMode(void);
#ifdef RC_BUTTON_RX_ON_EXIT_CAL
    void             onExitCalibrationMode(void (*onExitAction)(void));
#endif
    uint16_t         process(void);
    void             displayButtonPulseWidth(char *Prefix = NULL); // Ex: prefix pointer on the "BUTTON.WIDTH=" string
    private:
    RcButtonRxSt_t   _Priv;
    void             eol(void);
    uint8_t          getPushButtonIdx(uint16_t WidthUs);
};

#endif
