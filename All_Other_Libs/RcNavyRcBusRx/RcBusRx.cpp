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

#include <RcBusRx.h>

enum {
           RCBUSRX_IDLE = 0,
/* SBUS */ RCBUSRX_SBUS_WAIT_FOR_0x0F,           RCBUSRX_SBUS_WAIT_FOR_END_OF_DATA,       RCBUSRX_SBUS_WAIT_FOR_0x00,
/* SRXL */ RCBUSRX_SRXL_WAIT_FOR_0xA1_OR_0xA2,   RCBUSRX_SRXL_WAIT_FOR_CHANNELS,          RCBUSRX_SRXL_WAIT_FOR_CHECKSUM,
/* SRXL2*/ RCBUSRX_SRXL2_WAIT_FOR_0xA6,          RCBUSRX_SRXL2_WAIT_FOR_PKT_TYPE_CH_DATA, RCBUSRX_SRXL2_WAIT_FOR_PACKET_LEN,
           RCBUSRX_SRXL2_SKIP_UNTIL_CH_MAP,      RCBUSRX_SRXL2_WAIT_FOR_CH_MAP,
           RCBUSRX_SRXL2_WAIT_FOR_CHANNELS,      RCBUSRX_SRXL2_WAIT_FOR_CHECKSUM,
/* SUMD */ RCBUSRX_SUMD_WAIT_FOR_0xA8,           RCBUSRX_SUMD_WAIT_FOR_ST_0x01_OR_0x81,   RCBUSRX_SUMD_WAIT_FOR_CH_NB,
           RCBUSRX_SUMD_WAIT_FOR_CHANNELS,       RCBUSRX_SUMD_WAIT_FOR_CHECKSUM,
/* IBUS */ RCBUSRX_IBUS_WAIT_FOR_LEN_0x20,       RCBUSRX_IBUS_WAIT_FOR_CMD_0x40,          RCBUSRX_IBUS_WAIT_FOR_CHANNELS,
           RCBUSRX_IBUS_WAIT_FOR_CHECKSUM,
/* JETI */ RCBUSRX_JETI_WAIT_FOR_0x3E,           RCBUSRX_JETI_WAIT_FOR_0x03,              RCBUSRX_JETI_WAIT_FOR_MSG_LEN,
           RCBUSRX_JETI_WAIT_FOR_PKT_ID,         RCBUSRX_JETI_WAIT_FOR_DATA_CH,           RCBUSRX_JETI_WAIT_FOR_DATA_LEN,
           RCBUSRX_JETI_WAIT_FOR_CHANNELS,       RCBUSRX_JETI_WAIT_FOR_CHECKSUM,
/* CRSF */ RCBUSRX_CRSF_WAIT_FOR_ADDR_0xC8,      RCBUSRX_CRSF_WAIT_FOR_PKT_LEN,           RCBUSRX_CRSF_WAIT_FOR_TYPE_0x16,
           RCBUSRX_CRSF_WAIT_FOR_PAYLOAD,        RCBUSRX_CRSF_WAIT_FOR_CRC8
     };

RcBusRxClass RcBusRx = RcBusRxClass();

#if (RC_BUS_RX_SRXL_SUPPORT == 1) || (RC_BUS_RX_SRXL2_SUPPORT == 1) || (RC_BUS_RX_SUMD_SUPPORT == 1)
static uint16_t crc16_CCITT(uint16_t crc, uint8_t value); // For SRXL and SUMD
#endif

#if (RC_BUS_RX_JETI_SUPPORT == 1)
static uint16_t Jeti_crc16_CCITT(uint16_t crc, uint8_t value); // For JETI
#endif

#define MIN_SILENCE_TIME_MS  3 // We have to wait at least 2 ms of silence before listening for the protocol frame header


/*************************************************************************
                              GLOBAL VARIABLES
*************************************************************************/
#if (RC_BUS_RX_SBUS_SUPPORT == 1) || (RC_BUS_RX_CRSF_SUPPORT == 1)
typedef struct
{
    uint16_t ch1    : 11;
    uint16_t ch2    : 11;
    uint16_t ch3    : 11;
    uint16_t ch4    : 11;
    uint16_t ch5    : 11;
    uint16_t ch6    : 11;
    uint16_t ch7    : 11;
    uint16_t ch8    : 11;
    uint16_t ch9    : 11;
    uint16_t ch10   : 11;
    uint16_t ch11   : 11;
    uint16_t ch12   : 11;
    uint16_t ch13   : 11;
    uint16_t ch14   : 11;
    uint16_t ch15   : 11;
    uint16_t ch16   : 11;
} __attribute__((packed)) SbusCrsfChSt_t;
#endif

#if (RC_BUS_RX_CRSF_SUPPORT == 1)
#define USE_CRSF_CRC8_LUT /* Uncomment this to increase CRSF Crc8 computing rapidity */
#ifdef USE_CRSF_CRC8_LUT
const uint8_t CrsfCrc8Lut[256] PROGMEM = { // Generated with Polynome = 0xD5
                                          0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D, 
                                          0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F, 
                                          0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9, 
                                          0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B, 
                                          0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0, 
                                          0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2, 
                                          0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44, 
                                          0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16, 
                                          0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92, 
                                          0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0, 
                                          0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36, 
                                          0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64, 
                                          0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F, 
                                          0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D, 
                                          0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB, 
                                          0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9, 
                                         };
#endif
uint8_t crc8_dvb_s2(uint8_t Crc, uint8_t Byte);
#endif

#if (RC_BUS_RX_SRXL2_SUPPORT == 1)
typedef struct{
  uint32_t ChMap;
  uint8_t  PktLen;
  uint8_t  ChIdx;
}Srxl2St_t;
static Srxl2St_t Srxl2={0L, 0, 0};
#endif

/* Constructor */
RcBusRxClass::RcBusRxClass()
{
  RxSerial = NULL;
  RxState  = RCBUSRX_IDLE; // SBUS by default: skip characters until a silence happened
  ChNb     = 0;            // No channel until a valid frame is received
  Synchro  = 0x00;
}

void RcBusRxClass::serialAttach(Stream *RxStream)
{
  RxSerial = RxStream;
}

void RcBusRxClass::setProto(uint8_t Proto)
{
  Info.Proto = Proto;
}

void RcBusRxClass::process(void)
{
  uint8_t  RxChar, Finished = 0;
  
  if(RxSerial)
  {
    if(millis8() - StartMs > MIN_SILENCE_TIME_MS)
    {
      StartMs = millis8();
      switch(Info.Proto)
      {
#if (RC_BUS_RX_SBUS_SUPPORT == 1)
        case RC_BUS_RX_SBUS:
        ChNb = SBUS_RX_CH_NB;
        RxState = RCBUSRX_SBUS_WAIT_FOR_0x0F;
        break;
#endif        
#if (RC_BUS_RX_SRXL_SUPPORT == 1)
        case RC_BUS_RX_SRXL:
        /* ChNb will be set when header received */
        RxState = RCBUSRX_SRXL_WAIT_FOR_0xA1_OR_0xA2;
        break;
#endif        
#if (RC_BUS_RX_SRXL2_SUPPORT == 1)
        case RC_BUS_RX_SRXL2:
        /* ChNb will depend of ChMap */
        RxState = RCBUSRX_SRXL2_WAIT_FOR_0xA6;
        break;
#endif        
#if (RC_BUS_RX_SUMD_SUPPORT == 1)
        case RC_BUS_RX_SUMD:
        /* ChNb will be set when header received */
        RxState = RCBUSRX_SUMD_WAIT_FOR_0xA8;
        break;
#endif        
#if (RC_BUS_RX_IBUS_SUPPORT == 1)
        case RC_BUS_RX_IBUS:
        ChNb = IBUS_RX_CH_NB;
        RxState = RCBUSRX_IBUS_WAIT_FOR_LEN_0x20;
        break;
#endif        
#if (RC_BUS_RX_JETI_SUPPORT == 1)
        case RC_BUS_RX_JETI:
        RxState = RCBUSRX_JETI_WAIT_FOR_0x3E;
        break;
#endif
#if (RC_BUS_RX_CRSF_SUPPORT == 1)
        case RC_BUS_RX_CRSF:
        ChNb = CRSF_RX_CH_NB;
        RxState = RCBUSRX_CRSF_WAIT_FOR_ADDR_0xC8;
        break;
#endif
      }
    }
    while(RxSerial->available() > 0)
    {
      StartMs = millis8();
      RxChar = RxSerial->read();
      switch(RxState)
      {
#if (RC_BUS_RX_SBUS_SUPPORT == 1)
        /*****************/
        /* SBUS PROTOCOL */
        /*****************/
        case RCBUSRX_SBUS_WAIT_FOR_0x0F:
        if(RxChar == 0x0F)
        {
          RxIdx = 0;
          RxState = RCBUSRX_SBUS_WAIT_FOR_END_OF_DATA;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SBUS_WAIT_FOR_END_OF_DATA:
        RawData[RAW_IN_PROGRESS][RxIdx] = RxChar;
        RxIdx++;
        if(RxIdx == SBUS_RX_DATA_NB - 1)
        {
          /* Check next byte is 0x00 */
          RxState = RCBUSRX_SBUS_WAIT_FOR_0x00;
        }
        break;

        case RCBUSRX_SBUS_WAIT_FOR_0x00:
        if(RxChar == 0x00)
        {
          if(RxIdx == SBUS_RX_DATA_NB - 1)
          {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
          }
        }
        RxState = RCBUSRX_SBUS_WAIT_FOR_0x0F;
        break;
#endif
#if (RC_BUS_RX_SRXL_SUPPORT == 1)
        /*****************/
        /* SRXL PROTOCOL */
        /*****************/
        case RCBUSRX_SRXL_WAIT_FOR_0xA1_OR_0xA2:
        if((RxChar == 0xA1) || (RxChar == 0xA2))
        {
          ChNb = (RxChar == 0xA1)? SRXL_RX_A1_CH_NB: SRXL_RX_A2_CH_NB;
          RxIdx = 0;
          ComputedCrc = 0;
          ComputedCrc = crc16_CCITT(ComputedCrc, RxChar); // 0xA1 or 0xA2 headers are not stored to save RAM
          RxState = RCBUSRX_SRXL_WAIT_FOR_CHANNELS;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SRXL_WAIT_FOR_CHANNELS:
        RawData[RAW_IN_PROGRESS][RxIdx++] = RxChar;
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        if(RxIdx >= (2 * ChNb))
        {
          RxState = RCBUSRX_SRXL_WAIT_FOR_CHECKSUM;
        }
        break;

        case RCBUSRX_SRXL_WAIT_FOR_CHECKSUM:
        RxIdx++; /* Rx Checksum is not stored: just increment index */
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        if(RxIdx >= ((2 * ChNb) + 2))
        {
          if(!ComputedCrc)
          {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
          }
          else RxState = RCBUSRX_IDLE;
        }
        break;
#endif

#if (RC_BUS_RX_SRXL2_SUPPORT == 1)
        /******************/
        /* SRXL2 PROTOCOL */
        /******************/
        case RCBUSRX_SRXL2_WAIT_FOR_0xA6:
        if(RxChar == 0xA6)
        {
          ChNb = SRXL_MAX_RX_CH_NB;
          RxIdx = 0;
          ComputedCrc = 0;
          ComputedCrc = crc16_CCITT(ComputedCrc, RxChar); // 0xA6 header is not stored to save RAM
          RxState = RCBUSRX_SRXL2_WAIT_FOR_PKT_TYPE_CH_DATA;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SRXL2_WAIT_FOR_PKT_TYPE_CH_DATA:
        if(RxChar == 0xCD)
        {
          ComputedCrc = crc16_CCITT(ComputedCrc, RxChar); // 0xA1 or 0xA2 headers are not stored to save RAM
          RxState = RCBUSRX_SRXL2_WAIT_FOR_PACKET_LEN;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SRXL2_WAIT_FOR_PACKET_LEN:
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        Srxl2.PktLen = (uint8_t)RxChar - (1 + 1 + 1 + 1 + 1 + 1 + 2 + 4 + 2); /* 0xA6, Pkt_Type, PktLen, Cmd + ReplyId + Rssi, FrmLoss, ChMap, Crc */
        RxIdx = 0;
        RxState = RCBUSRX_SRXL2_SKIP_UNTIL_CH_MAP;
        break;

        case RCBUSRX_SRXL2_SKIP_UNTIL_CH_MAP: /* Skip: Cmd(1Byte) ReplyId(1Byte), Rssi(1Byte), FrmLoss(2Bytes) */
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        RxIdx++;
        if(RxIdx >= (1 + 1 + 1 + 2))
        {
          RxIdx       = 0;
          Srxl2.ChMap = 0UL;
          Srxl2.ChIdx = 0;
          RxState = RCBUSRX_SRXL2_WAIT_FOR_CH_MAP;
        }
        break;

        case RCBUSRX_SRXL2_WAIT_FOR_CH_MAP:
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        Srxl2.ChMap |= ((uint32_t)RxChar << (8UL * (3 - RxIdx)));
        RxIdx++;
        if(RxIdx >= 4)
        {
          Srxl2.ChMap = NTOHL(Srxl2.ChMap);
          RxIdx   = 0;
          RxState = RCBUSRX_SRXL2_WAIT_FOR_CHANNELS;
        }
        break;

        case RCBUSRX_SRXL2_WAIT_FOR_CHANNELS:
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        if(!RxIdx)
        {
          /* Compute ChIdx */
          while(!(Srxl2.ChMap & 1))
          {
            Srxl2.ChMap >>= 1;
            Srxl2.ChIdx++;
            if(Srxl2.ChIdx >= SRXL_MAX_RX_CH_NB)
            {
              RxState = RCBUSRX_IDLE;
              break;
            }
          }
        }
        if(Srxl2.ChIdx < SRXL_MAX_RX_CH_NB)
        {
          RawData[RAW_IN_PROGRESS][(Srxl2.ChIdx * 2) + (1 - RxIdx++)] = RxChar;
          if(RxIdx >= 2)
          {
            Srxl2.ChMap >>= 1;
            Srxl2.ChIdx++;
            RxIdx = 0;
          }
        }
        Srxl2.PktLen--;
        if(!Srxl2.PktLen)
        {
          RxIdx = 0; /* Normally already null */
          RxState = RCBUSRX_SRXL2_WAIT_FOR_CHECKSUM;
        }
        break;

        case RCBUSRX_SRXL2_WAIT_FOR_CHECKSUM:
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        RxIdx++; /* Rx Checksum is not stored: just increment index */
        if(RxIdx >= 2)
        {
          if(!ComputedCrc)
          {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
          }
          else RxState = RCBUSRX_IDLE;
        }
        break;
#endif

#if (RC_BUS_RX_SUMD_SUPPORT == 1)        
        case RCBUSRX_SUMD_WAIT_FOR_0xA8:
        if(RxChar == 0xA8)
        {
          ComputedCrc = 0;
          ComputedCrc = crc16_CCITT(ComputedCrc, RxChar); // 0xA8 header is not stored to save RAM
          RxState = RCBUSRX_SUMD_WAIT_FOR_ST_0x01_OR_0x81;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SUMD_WAIT_FOR_ST_0x01_OR_0x81:
        if((RxChar == 0x01) || (RxChar == 0x81))
        {
          ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
          Info.FailSafe = (RxChar == 0x81);
          RxState = RCBUSRX_SUMD_WAIT_FOR_CH_NB;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SUMD_WAIT_FOR_CH_NB:
        if((RxChar >= 2) && (RxChar <= 16))
        {
          ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
          ChNb = RxChar;
          RxIdx = 0;
          RxState = RCBUSRX_SUMD_WAIT_FOR_CHANNELS;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_SUMD_WAIT_FOR_CHANNELS:
        RawData[RAW_IN_PROGRESS][RxIdx++] = RxChar;
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        if(RxIdx >= (2 * ChNb))
        {
          RxState = RCBUSRX_SUMD_WAIT_FOR_CHECKSUM;
        }
        break;

        case RCBUSRX_SUMD_WAIT_FOR_CHECKSUM:
        RxIdx++; /* Rx Checksum is not stored: just increment index */
        ComputedCrc = crc16_CCITT(ComputedCrc, RxChar);
        if(RxIdx >= ((2 * ChNb) + 2))
        {
          if(!ComputedCrc)
          {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
          }
          else RxState = RCBUSRX_IDLE;
        }
        break;
#endif
#if (RC_BUS_RX_IBUS_SUPPORT == 1)        
        /*****************/
        /* IBUS PROTOCOL */
        /*****************/
        case RCBUSRX_IBUS_WAIT_FOR_LEN_0x20:
        if(RxChar == 0x20)
        {
          ComputedCrc = 0xFFFF - 0x20;
          RxState = RCBUSRX_IBUS_WAIT_FOR_CMD_0x40;
        }
        break;

        case RCBUSRX_IBUS_WAIT_FOR_CMD_0x40:
        if(RxChar == 0x40)
        {
          ComputedCrc -= 0x40;
          RxIdx = 0;
          RxState = RCBUSRX_IBUS_WAIT_FOR_CHANNELS;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_IBUS_WAIT_FOR_CHANNELS:
        RawData[RAW_IN_PROGRESS][RxIdx++] = RxChar;
        ComputedCrc -= RxChar;
        if(RxIdx >= (2 * ChNb))
        {
          RxState = RCBUSRX_IBUS_WAIT_FOR_CHECKSUM;
        }
        break;

        case RCBUSRX_IBUS_WAIT_FOR_CHECKSUM:
        RawData[RAW_IN_PROGRESS][RxIdx] = RxChar;
        RxIdx++; /* Rx Checksum is not stored: just increment index */
        if(RxIdx >= ((2 * ChNb) + 2))
        {
          if(ComputedCrc == (uint16_t)((RxChar << 8) + RawData[RAW_IN_PROGRESS][(2 * ChNb)])) // If Checksum = 0, frame is OK
          {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
          }
          RxState = RCBUSRX_IDLE;
        }
        break;
#endif
#if (RC_BUS_RX_JETI_SUPPORT == 1)
        case RCBUSRX_JETI_WAIT_FOR_0x3E:
        if(RxChar == 0x3E)
        {
          ComputedCrc = 0;
          ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar); // 0x3E header is not stored to save RAM
          RxState = RCBUSRX_JETI_WAIT_FOR_0x03;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_JETI_WAIT_FOR_0x03:
        if(RxChar == 0x03)
        {
          ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar); // 0x03 header is not stored to save RAM
          RxState = RCBUSRX_JETI_WAIT_FOR_MSG_LEN;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_JETI_WAIT_FOR_MSG_LEN:
        ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar); // not stored to save RAM
        RxState = RCBUSRX_JETI_WAIT_FOR_PKT_ID;
        break;

        case RCBUSRX_JETI_WAIT_FOR_PKT_ID:
        ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar); // not stored to save RAM
        RxState = RCBUSRX_JETI_WAIT_FOR_DATA_CH;
        break;

        case RCBUSRX_JETI_WAIT_FOR_DATA_CH:
        if(RxChar == 0x31) // SHALL be 0x31 (Data ID for channels)
        {
          ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar); // 0x31 header is not stored to save RAM
          RxState = RCBUSRX_JETI_WAIT_FOR_DATA_LEN;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_JETI_WAIT_FOR_DATA_LEN:
        if((RxChar >= (4 * 2)) && (RxChar <= (16 * 2))) // 4 to 16 channels?
        {
          ChNb  = RxChar / 2;
          RxIdx = 0;
          ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar); // not stored to save RAM
          RxState = RCBUSRX_JETI_WAIT_FOR_CHANNELS;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_JETI_WAIT_FOR_CHANNELS:
        RawData[RAW_IN_PROGRESS][RxIdx++] = RxChar;
        ComputedCrc = Jeti_crc16_CCITT(ComputedCrc, RxChar);
        if(RxIdx >= (2 * ChNb))
        {
          RxState = RCBUSRX_JETI_WAIT_FOR_CHECKSUM;
        }
        break;

        case RCBUSRX_JETI_WAIT_FOR_CHECKSUM:
        RawData[RAW_IN_PROGRESS][RxIdx] = RxChar;
        RxIdx++; /* Rx Checksum is not stored: just increment index */
        if(RxIdx >= ((2 * ChNb) + 2))
        {
          if(ComputedCrc == (uint16_t)((RxChar << 8) + RawData[RAW_IN_PROGRESS][(2 * ChNb)])) // If Checksum = 0, frame is OK
          {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
          }
          RxState = RCBUSRX_IDLE;
        }
        break;
#endif

#if (RC_BUS_RX_CRSF_SUPPORT == 1)
        /*****************/
        /* CRSF PROTOCOL */
        /*****************/
        case RCBUSRX_CRSF_WAIT_FOR_ADDR_0xC8:
        if(RxChar == 0xC8)
        {
          RxState = RCBUSRX_CRSF_WAIT_FOR_PKT_LEN;
        }
        else RxState = RCBUSRX_IDLE;
        break;

        case RCBUSRX_CRSF_WAIT_FOR_PKT_LEN:
        if (RxChar == 24)
        {
          PayloadLen = 22; /* To get Payload Len, Remove Type and Crc8: (PacketLen - 2) */
          RxState = RCBUSRX_CRSF_WAIT_FOR_TYPE_0x16;
        }
        else
        {
          RxState = RCBUSRX_IDLE;
        }
        break;

        case RCBUSRX_CRSF_WAIT_FOR_TYPE_0x16:
        if(RxChar == 0x16)
        {
          RxIdx = 0;
          Crc8  = 0; /* Initial Crc8 value shall be 0 */
          Crc8  = crc8_dvb_s2(Crc8, RxChar);
          RxState = RCBUSRX_CRSF_WAIT_FOR_PAYLOAD;
        }
        else
        {
          RxState = RCBUSRX_IDLE;
        }
        break;

        case RCBUSRX_CRSF_WAIT_FOR_PAYLOAD:
        RawData[RAW_IN_PROGRESS][RxIdx++] = RxChar;
        Crc8 = crc8_dvb_s2(Crc8, RxChar);
        PayloadLen--;
        if(!PayloadLen)
        {
          RxState = RCBUSRX_CRSF_WAIT_FOR_CRC8;
        }
        break;

        case RCBUSRX_CRSF_WAIT_FOR_CRC8:
        if(RxChar == Crc8)
        {
            /* Update RawData[RAW_AVAILABLE] */
            memcpy(RawData[RAW_AVAILABLE], RawData[RAW_IN_PROGRESS], sizeof(RawData[0]));
            Synchro  = 0xFF;
            Finished = 1;
        }
        RxState = RCBUSRX_IDLE;
        break;
#endif
      }
      if(Finished) break;
    }
  }
}

uint8_t RcBusRxClass::channelNb(void)
{
  return(ChNb);
}

#define RETURN_RAW_CH(Proto, Ch) case Ch: OneRawData = Proto->ch##Ch; break

uint16_t RcBusRxClass::rawData(uint8_t Ch)
{
  uint16_t       *WordPtr, OneRawData = 0x0800; /* Corresponds to 1500 us for SRXL */
  uint8_t         ChIdx = 0;
  #if (RC_BUS_RX_SBUS_SUPPORT == 1) || (RC_BUS_RX_CRSF_SUPPORT == 1)
  SbusCrsfChSt_t *SbusCrsf = nullptr;
  #endif

  switch(Info.Proto)
  {
    #if (RC_BUS_RX_SBUS_SUPPORT == 1) || (RC_BUS_RX_CRSF_SUPPORT == 1)
    case RC_BUS_RX_SBUS:
    case RC_BUS_RX_CRSF:
    SbusCrsf = (SbusCrsfChSt_t *)&RawData[RAW_AVAILABLE];
    switch(Ch)
    {
      RETURN_RAW_CH(SbusCrsf, 1);
      RETURN_RAW_CH(SbusCrsf, 2);
      RETURN_RAW_CH(SbusCrsf, 3);
      RETURN_RAW_CH(SbusCrsf, 4);
      RETURN_RAW_CH(SbusCrsf, 5);
      RETURN_RAW_CH(SbusCrsf, 6);
      RETURN_RAW_CH(SbusCrsf, 7);
      RETURN_RAW_CH(SbusCrsf, 8);
      RETURN_RAW_CH(SbusCrsf, 9);
      RETURN_RAW_CH(SbusCrsf, 10);
      RETURN_RAW_CH(SbusCrsf, 11);
      RETURN_RAW_CH(SbusCrsf, 12);
      RETURN_RAW_CH(SbusCrsf, 13);
      RETURN_RAW_CH(SbusCrsf, 14);
      RETURN_RAW_CH(SbusCrsf, 15);
      RETURN_RAW_CH(SbusCrsf, 16);
      
      default:
      break;
    }
    break;
    #endif

    case RC_BUS_RX_SRXL:
    case RC_BUS_RX_SRXL2:
    case RC_BUS_RX_SUMD:
    ChIdx = Ch - 1;
    WordPtr = (uint16_t *)&RawData[RAW_AVAILABLE][ChIdx * 2];
    OneRawData = NTOHS(*WordPtr);
    break;

    case RC_BUS_RX_IBUS:
    case RC_BUS_RX_JETI:
    ChIdx = Ch - 1;
    WordPtr = (uint16_t *)&RawData[RAW_AVAILABLE][ChIdx * 2];
    OneRawData = *WordPtr;
    break;

    default:
    return 1024;
    break;
  }

  return(OneRawData);
}

uint16_t RcBusRxClass::width_us(uint8_t Ch)
{
  uint16_t OneRawData, Width_us = 1500;

  if((Ch >= 1) && (Ch <= SBUS_RX_CH_NB))
  {
    OneRawData = rawData(Ch);
    switch(Info.Proto)
    {
      case RC_BUS_RX_SBUS:
      Width_us = map(OneRawData, 0, 2047, 880, 2160);
      break;

      case RC_BUS_RX_SRXL:
      Width_us= map(OneRawData, 0, 0x0FFF, 800, 2200);
      break;

      case RC_BUS_RX_SRXL2:
      Width_us = map(OneRawData, 10912, 54612, 1000, 2000); /* -100% to +100 on Spektrum Transmitter */
      break;

      case RC_BUS_RX_IBUS:
      Width_us = OneRawData;
      break;

      case RC_BUS_RX_JETI:
      Width_us = OneRawData >> 3; // With JETI, RawData is Channel in us x 8
      break;

      case RC_BUS_RX_SUMD:
      Width_us = map(OneRawData, 0x1C20, 0x41A0, 900, 2100);
      break;

      case RC_BUS_RX_CRSF:
      Width_us = map(OneRawData, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);
      break;

      default:
      break;
    }
  }
  return(Width_us);
}

uint8_t RcBusRxClass::isSynchro(uint8_t SynchroClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(Synchro & RCUL_CLIENT_MASK(SynchroClientIdx));
  if(Ret) Synchro &= ~RCUL_CLIENT_MASK(SynchroClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

uint8_t RcBusRxClass::flags(uint8_t FlagId)
{
  uint8_t Flag = 0;

  FlagId = FlagId; /* To avoid compilation warning when neither SBUS nor SUMD */
  switch(Info.Proto)
  {
#if (RC_BUS_RX_SBUS_SUPPORT == 1)
    case RC_BUS_RX_SBUS:
    Flag = !!(RawData[RAW_AVAILABLE][SBUS_RX_DATA_NB - 1] & FlagId);
    break;
#endif
#if (RC_BUS_RX_SUMD_SUPPORT == 1)
    case RC_BUS_RX_SUMD:
    FlagId = FlagId; // To avoid a compilation warning
    Flag = Info.FailSafe;
    break;
#endif
    default:
    break;
  }
  return(Flag);
}

/* Rcul support */
uint8_t RcBusRxClass::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(isSynchro(ClientIdx));  
}

uint16_t RcBusRxClass::RculGetWidth_us(uint8_t Ch)
{
  return(width_us(Ch));
}

void RcBusRxClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  Width_us = Width_us; /* To avoid a compilation warning */
  Ch = Ch;             /* To avoid a compilation warning */
}

#if (RC_BUS_RX_SRXL_SUPPORT == 1) || (RC_BUS_RX_SRXL2_SUPPORT == 1) || (RC_BUS_RX_SUMD_SUPPORT == 1)
static uint16_t crc16_CCITT(uint16_t crc, uint8_t value)
{
  uint8_t i;

  crc = crc ^ (int16_t)value << 8;

  for (i = 0; i < 8; i++)
  {
    if (crc & 0x8000)
    {
      crc = crc << 1 ^ 0x1021;
    } else
    {
      crc = crc << 1;
    }
  }
  return crc;
}
#endif

#if (RC_BUS_RX_JETI_SUPPORT == 1)
static uint16_t Jeti_crc16_CCITT(uint16_t crc, uint8_t data)
{
  uint16_t ret_val;

  data ^= (uint8_t)(crc) & (uint8_t)(0xFF);
  data ^= data << 4;
  ret_val = ((((uint16_t)data << 8) | ((crc & 0xFF00) >> 8))
          ^ (uint8_t)(data >> 4)
          ^ ((uint16_t)data << 3));
  
  return ret_val;
}
#endif

#if (RC_BUS_RX_CRSF_SUPPORT == 1)
uint8_t crc8_dvb_s2(uint8_t Crc, uint8_t Byte)
{
#ifdef USE_CRSF_CRC8_LUT
  return((uint8_t)pgm_read_byte(&CrsfCrc8Lut[Crc ^ Byte]));
#else
  Crc ^= Byte;
  for(uint8_t i = 0; i < 8; i++)
  {
    if (Crc & 0x80) Crc = (Crc << 1) ^ 0xD5; /* Polynome = 0xD5 */
    else            Crc = (Crc << 1);
  }
  return(Crc);
#endif
}
#endif