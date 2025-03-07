/*
 English: by RC Navy (2023)
 =======
 <SoftSBusTx>: a library to generate RC Normal/Inverted SBUS frame. No hardware inverter needed. All implementation done in software.
 http://p.loussouarn.free.fr
 V1.0 (16/07/2023): initial release

 English: by RC Navy (2023)
 =======
 <SoftSBusTx>: une bibliotheque pour generer des trames SBUS RC Normales/Inversees. Pas besoin d'inverseur materiel. Toute l'implementation est faite par logiciel.
 http://p.loussouarn.free.fr
 V1.0 (16/07/2023): release initiale
*/

#ifndef SOFT_SBUS_TX_H
#define SOFT_SBUS_TX_H

#include <Arduino.h>

#define SOFT_SBUS_TX_VERSION                  1
#define SOFT_SBUS_TX_REVISION                 0

/* vvv Library configuration vvv */
#define SOFT_SBUS_TX_PIN_PORT_LETTER          D /* Define here the port letter */
#define SOFT_SBUS_TX_PIN_BIT_IN_PORT          1 /* Define here the bit in port */
//#define SOFT_SBUS_TX_CH_RAW_PROVIDER /* if defined, the user shall define a call back function to provide Channel Values */
/* ^^^ Library configuration ^^^ */


/* Macro for Port and Bit in Port concatenation */
#define CONCAT_PORT_WITH_PORT_BIT_(PortLetter, BitInPort)  PortLetter, BitInPort
#define CONCAT_PORT_WITH_PORT_BIT(PortLetter, BitInPort)   CONCAT_PORT_WITH_PORT_BIT_(PortLetter, BitInPort)

#define SOFT_SBUS_TX_PIN                      CONCAT_PORT_WITH_PORT_BIT(SOFT_SBUS_TX_PIN_PORT_LETTER, SOFT_SBUS_TX_PIN_BIT_IN_PORT)  /* Gives B, 1 for example */

#define SOFT_SBUS_TX_CHANNEL_NB               16 /* /!\ Do NOT touch this /!\ */

#define SOFT_SBUS_TX_HIGH_SPEED_FRAME_RATE_MS  7
#define SOFT_SBUS_TX_NORMAL_FRAME_RATE_MS     14

#define SOFT_SBUS_TX_POLAR_FUTABA              1
#define SOFT_SBUS_TX_POLAR_FUTABA_INV          0

class SoftSBusTxClass
{
  public:
    SoftSBusTxClass(void);
    void                begin(uint8_t TxPeriodMs = SOFT_SBUS_TX_NORMAL_FRAME_RATE_MS, uint8_t FutabaPolar = 1);
    void                rawData(uint8_t ChId, uint16_t RawData);
    void                width_us(uint8_t ChId, uint16_t Width_us);
    uint8_t             process(void);

private:
    size_t              serialWrite(uint8_t b);
    void                sendFrame(void);
    uint8_t            _TxPeriodMs;
    uint8_t            _FutabaPolar;
    uint32_t           _StartFrameMs;
    uint16_t           _SbusChValue[SOFT_SBUS_TX_CHANNEL_NB];
};

extern SoftSBusTxClass SoftSBusTx; /* Object externalisation */

#endif

