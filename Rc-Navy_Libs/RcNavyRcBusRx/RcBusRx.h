#ifndef RC_BUS_RX_H
#define RC_BUS_RX_H

/*
   _____     ____      __    _    ____    _    _   _     _ 
  |  __ \   / __ \    |  \  | |  / __ \  | |  | | | |   | |
  | |__| | | /  \_|   | . \ | | / /  \ \ | |  | |  \ \ / /
  |  _  /  | |   _    | |\ \| | | |__| | | |  | |   \ ' /
  | | \ \  | \__/ |   | | \ ' | |  __  |  \ \/ /     | |
  |_|  \_\  \____/    |_|  \__| |_|  |_|   \__/      |_| 2016-2025

                http://p.loussouarn.free.fr

  Version/Revision history:
                V0.1:         Initial release
  (12/04/2025)  V0.1 -> V0.2: Support for SRXL2 added (115200 baud only and Handshake not supported yet: shall be performed with other peripheral)
  (27/12/2025)  V0.2->  V0.3: Support for CRSF  added (115200 baud only) and library reorganization
*/

#include "Arduino.h"
#include <Misclib.h>
#include <Rcul.h>

/* Library Version/Revision */
#define RC_BUS_RX_VERSION         0
#define RC_BUS_RX_REVISION        3

/* Compilation directives to reduce code and ram size in case of small micro-controller (set to 0 all the unused protocols) */
#define RC_BUS_RX_SBUS_SUPPORT    1 // <-- Set here 1 for support or 0 to not support it
#define RC_BUS_RX_SRXL_SUPPORT    1 // <-- Set here 1 for support or 0 to not support it
#define RC_BUS_RX_SRXL2_SUPPORT   1 // <-- Set here 1 for support or 0 to not support it
#define RC_BUS_RX_SUMD_SUPPORT    1 // <-- Set here 1 for support or 0 to not support it
#define RC_BUS_RX_IBUS_SUPPORT    1 // <-- Set here 1 for support or 0 to not support it
#define RC_BUS_RX_JETI_SUPPORT    1 // <-- Set here 1 for support or 0 to not support it
#define RC_BUS_RX_CRSF_SUPPORT    1 // <-- Set here 1 for support or 0 to not support it

enum {RC_BUS_RX_SBUS = 0, RC_BUS_RX_SRXL, RC_BUS_RX_SRXL2, RC_BUS_RX_SUMD, RC_BUS_RX_IBUS, RC_BUS_RX_JETI, RC_BUS_RX_CRSF, RC_BUS_RX_NB};

/* SBUS */
/* /!\ Serial port shall be set to 100000, SERIAL_8E2  and needs often a serial inverter /!\ */
#define SBUS_RX_SERIAL_CFG      100000, SERIAL_8E2
#define SBUS_RX_CH_NB           16
#define SBUS_RX_DATA_NB         ((((SBUS_RX_CH_NB * 11) + 7) / 8) + 1) /* +1 for flags -> 23 for 16 channels */

#define SBUS_RX_CH17            (1 << 0)
#define SBUS_RX_CH18            (1 << 1)
#define SBUS_RX_FRAME_LOST      (1 << 2)
#define SBUS_RX_FAILSAFE        (1 << 3)

/* SRXL */
/* /!\ Serial port shall be set to 115200 /!\ */
#define SRXL_RX_SERIAL_CFG      115200
#define SRXL_RX_A1_CH_NB        12
#define SRXL_RX_A2_CH_NB        16

/* SRXL2 */
/* /!\ Serial port shall be set to 115200 /!\ */
#define SRXL2_RX_SERIAL_CFG     115200

#define SRXL_MAX_RX_CH_NB       SRXL_RX_A2_CH_NB /* Also valid for SRXL2 */

/* SUMD */
#define SUMD_RX_SERIAL_CFG      115200 // Frame send every 10 ms
#define SUMD_RX_FAILSAFE        (1 << 7)

/* IBUS */
#define IBUS_RX_SERIAL_CFG      115200 // Frame send every 7 ms
#define IBUS_RX_CH_NB           14

/* JETI */
#define JETI_RX_SERIAL_CFG      125000 // Frame send every ? ms

/* CRSF */
#define CRSF_RX_SERIAL_CFG      115200 // Frame send every ? ms
#define CRSF_RX_CH_NB           16
#define CRSF_CHANNEL_VALUE_1000 191
#define CRSF_CHANNEL_VALUE_2000 1792

typedef struct{
  uint8_t
        Proto     :4,
        FailSafe  :1,
        Reserved  :3; // For SUMD FailSafe
}RcBusInfoSt_t;

enum {RAW_IN_PROGRESS = 0, RAW_AVAILABLE, RAW_BUF_NB};

class RcBusRxClass : public Rcul
{
  public:
    RcBusRxClass(void);
    void                  serialAttach(Stream *RxStream);
    void                  setProto(uint8_t Proto);
    void                  process(void);
    uint8_t               isSynchro(uint8_t SynchroClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free for other clients*/
    uint16_t              rawData(uint8_t Ch);
    uint16_t              width_us(uint8_t Ch);
    uint8_t               channelNb(void);
    uint8_t               flags(uint8_t FlagId);
    /* Rcul support */
    virtual uint8_t       RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual uint16_t      RculGetWidth_us(uint8_t Ch);
    virtual void          RculSetWidth_us(uint16_t Width_us, uint8_t Ch = 255);
  private:
    Stream               *RxSerial;
    RcBusInfoSt_t         Info;
    uint8_t               StartMs;
    uint8_t               RxState;
    uint8_t               RxIdx;
    uint8_t               ChNb;
    #if (RC_BUS_RX_CRSF_SUPPORT == 1)
    uint8_t               PayloadLen, Crc8;
    #endif
    #if (RC_BUS_RX_SRXL_SUPPORT == 1) || (RC_BUS_RX_SRXL2_SUPPORT == 1) || (RC_BUS_RX_SUMD_SUPPORT == 1) || (RC_BUS_RX_IBUS_SUPPORT == 1) || (RC_BUS_RX_JETI_SUPPORT == 1)
    uint16_t              ComputedCrc; // CRC or Checksum
    #endif
    uint8_t               RawData[RAW_BUF_NB][SRXL_MAX_RX_CH_NB * 2]; // Max size
    uint8_t               Synchro;
};

extern RcBusRxClass RcBusRx; /* Object externalisation */

#endif
