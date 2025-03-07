#include <EkmfaTx.h>

#define PROJECT_NAME            EKMFA_TX_SIM
#define FW_VERSION              0
#define FW_REVISION             1

#define EKMFA_TX_SIM_VER_REV  PRJ_VER_REV(PROJECT_NAME, FW_VERSION, FW_REVISION) /* ProjectName VVersion.Revision */
#define DBG_PRINT(...)          Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...)        Serial.println(__VA_ARGS__)

#define CARRIAGE_RETURN         0x0D /* '\r' = 0x0D (code ASCII) */
#define RUB_OUT                 0x08
#define CFG_MSG_MAX_LENGTH      42 /* Longest Rx or Tx Message */
static char                     CfgMessage[CFG_MSG_MAX_LENGTH + 1];/* + 1 pour fin de chaine */

#define EKMFA_TX_PIN            2

#define EKMFA_OUTPUT_NB         30 /* Used form optimization: if only 12 commands are needed, set EKMFA_OUTPUT_NB to 12 */

#if (EKMFA_OUTPUT_NB > 30)
#error EKMFA_OUTPUT_NB shall not exceed 30!
#endif
#define EKMFA_CMD_BYTE_NB       ((EKMFA_OUTPUT_NB + 7) / 8)
#define EKMFA_CMD_BYTE_IDX_MAX  (EKMFA_CMD_BYTE_NB - 1)

uint8_t CmdFromXany[EKMFA_CMD_BYTE_NB];

void setup()
{
  Serial.begin(115200);
  DBG_PRINTLN(F(EKMFA_TX_SIM_VER_REV));
  DBG_PRINTLN(F("Hit Enter or H + Enter for Help"));
//  EkmfaTx.begin(EKMFA_TX_PIN, CmdFromXany, EKMFA_OUTPUT_NB); /* Basic behaviour: no optimization */
//  EkmfaTx.begin(EKMFA_TX_PIN, CmdFromXany, EKMFA_OUTPUT_NB, EKMFA_TX_LAST_CMD_REPEAT_SUPPORT); /* Last command repeat support */
  EkmfaTx.begin(EKMFA_TX_PIN, CmdFromXany, EKMFA_OUTPUT_NB, EKMFA_TX_LAST_CMD_REPEAT_SUPPORT | EKMFA_TX_CMD_OPTIMIZATION_SUPPORT); /* Last command repeat support and alternate cmd optimization */
  EkmfaTxClass::setEepBaseAddr(0);
  EkmfaTxClass::updateDurationMs(EKMFA_TX_RESET_DURATION_IDX,       100);
  EkmfaTxClass::updateDurationMs(EKMFA_TX_BURST_DURATION_IDX,       100);
  EkmfaTxClass::updateDurationMs(EKMFA_TX_INTER_BURST_DURATION_IDX, 50);
  EkmfaTxClass::updateDurationMs(EKMFA_TX_LAST_RECALL_DURATION_IDX, 200);
#if (EKMFA_TX_DEBUG > 0) /* Set EKMFA_TX_DEBUG to 1 in EkmfaTx.h to enable Debug */
  EkmfaTx.debugProtocol(1);
#endif
}

void loop()
{
  static uint16_t StartMs = millis16();
  
  if(CfgMessageAvailable() >= 0)
  {
    InterpreteCfgAndExecute();
  }

  if(ElapsedMs16Since(StartMs) >= 20)
  {
    StartMs = millis16();
    EkmfaTx.updateOrder();
  }
  EkmfaTx.process();
  
}

void Uint32ToXanyBuf(uint32_t Uint32Value, uint8_t *XanyBuf, uint8_t EkmfaCmdByteNb)
{
  uint8_t EkmfaCmdByteIdxMax = EkmfaCmdByteNb - 1;
  uint8_t *Ptr = (uint8_t *)&Uint32Value;
  
  for(uint8_t ByteIdx = 0; ByteIdx <= EkmfaCmdByteIdxMax ;ByteIdx++)
  {
    XanyBuf[EkmfaCmdByteIdxMax - ByteIdx] = Ptr[ByteIdx];
  }
}
  
void PrintByteBin(uint8_t Byte)
{
  for(uint8_t Idx = 0; Idx < 8; Idx++)
  {
    Serial.print(bitRead(Byte, 7 - Idx));
  }
}

static char CfgMessageAvailable(void)
{
  char Ret = -1;
  char RxChar;
  static uint8_t Idx = 0;

  if(Serial.available() > 0)
  {
    RxChar = Serial.read();
    switch(RxChar)
    {
      case CARRIAGE_RETURN: /* Si retour chariot: fin de message */
      CfgMessage[Idx] = 0;/* Remplace CR character par fin de chaine */
      Ret = Idx;
      Idx = 0; /* Re-positionne index pour prochain message */
      break;
            
      case RUB_OUT:
      if(Idx) Idx--;
      break;
            
      default:
      if(Idx < CFG_MSG_MAX_LENGTH)
      {
        CfgMessage[Idx] = RxChar;
        Idx++;
      }
      else Idx = 0; /* Re-positionne index pour prochain message */
      break;
    }
  }
  return(Ret); 
}

#define COMMAND       ( CfgMessage[0] )
#define ACTION        ( CfgMessage[1] )
#define ARG           ( CfgMessage[2] )
#define REQUEST       '?'
#define ORDER         '='

#define EMPTY_CMD      0
#define HELP_CMD      'H'
#define MS_OUTPUT     'M'
#define FUNCT_CMD     'F'
#define DBG_CMD       'D'

enum {ACTION_ANSWER_WITH_REPONSE = 0, ACTION_ANSWER_ERROR};

static void InterpreteCfgAndExecute(void)
{
  static uint32_t LastCmd   = 0;
  static uint8_t  LastFunct = 0;
  uint8_t  StrLen, Action = ACTION_ANSWER_ERROR;
  uint16_t ValUint16High, ValUint16Low;
  uint32_t ValUint32;
  char     Memo, HexVal[8 + 1];
  uint8_t *Ptr = (uint8_t *)&LastCmd;
  uint8_t  FunctId, Oor = 0;
  
  switch(COMMAND)
  {
    case EMPTY_CMD: /* No break: Normal */
    case HELP_CMD:
    DisplayHelp();
    CfgMessage[1] = 0; /* Echo */
    Action = ACTION_ANSWER_WITH_REPONSE;
    break;

    case FUNCT_CMD:
    if(ACTION == ORDER)
    {
      FunctId = (uint8_t)atoi(&ARG);
      if(FunctId >= 1 && FunctId <= EKMFA_OUTPUT_NB)
      {
        LastFunct = FunctId;
        ValUint32 = (1UL << (FunctId - 1));
        Serial.print(F("ValUint32=0x"));Serial.println(ValUint32, HEX);
        /* Load CmdFromXany[] in the right order Msb Left/Lsb Right */
        Uint32ToXanyBuf(ValUint32, CmdFromXany, EKMFA_CMD_BYTE_NB);
        CfgMessage[1] = 0;
        Action = ACTION_ANSWER_WITH_REPONSE;
      }
    }
    else
    {
      sprintf_P(CfgMessage + 1, PSTR("=%u"), LastFunct);
      Action = ACTION_ANSWER_WITH_REPONSE;
    }
    break;
    
    case MS_OUTPUT:
    if(ACTION == ORDER)
    {
      if((ARG == '0') && (CfgMessage[3] == 'x'))
      {
        for(uint8_t DigitIdx = 0; DigitIdx < 8; DigitIdx++)
        {
          HexVal[DigitIdx] = '0';
        }
        StrLen = strlen(CfgMessage + 4);
        if(StrLen > 8) StrLen = 8;
        for(uint8_t Idx = 0; Idx < StrLen; Idx++)
        {
          HexVal[7 - Idx] = CfgMessage[4 + (StrLen -1 - Idx)];
        }
        HexVal[8]= 0; /* End of string */
        Memo = HexVal[4];
        HexVal[4] = 0; /* End of string */
        ValUint16High = (uint16_t)HexAsciiToInt16(HexVal);
        HexVal[4] = Memo; /* Retore character */
        ValUint16Low  = (uint16_t)HexAsciiToInt16(HexVal + 4);
        ValUint32     = ((uint32_t)ValUint16High << 16) + (uint32_t)ValUint16Low;
//        Serial.print((uint32_t)(1UL << EKMFA_OUTPUT_NB));
        if(ValUint32 >= (uint32_t)(1UL << EKMFA_OUTPUT_NB)) Oor = 1;
        if(!Oor)
        {
          LastCmd = ValUint32;
          Serial.print(F("ValUint32=0x"));Serial.println(ValUint32, HEX);
          /* Load CmdFromXany[] in the right order Msb Left/Lsb Right */
          Uint32ToXanyBuf(ValUint32, CmdFromXany, EKMFA_CMD_BYTE_NB);
        }
      }
      else
      {
        ValUint32 = (uint32_t)atol(&ARG);
        if(ValUint32 >= (uint32_t)(1UL << EKMFA_OUTPUT_NB)) Oor = 1;
        if(!Oor)
        {
          LastCmd = ValUint32;
          Serial.print(F("ValUint32=0x"));Serial.println(ValUint32, HEX);
          /* Load CmdFromXany[] in the right order Msb Left/Lsb Right */
          Uint32ToXanyBuf(ValUint32, CmdFromXany, EKMFA_CMD_BYTE_NB);
        }
      }
      if(!Oor)
      {
        Serial.print(F("Bit:"));
        for(uint8_t ByteIdx = 0; ByteIdx <= EKMFA_CMD_BYTE_IDX_MAX ;ByteIdx++)
        {
          Serial.print(F("76543210"));
          if(ByteIdx < EKMFA_CMD_BYTE_IDX_MAX) Serial.print(F("."));
        }
        Serial.println();
        Serial.print(F("Val:"));
        for(uint8_t ByteIdx = 0; ByteIdx <= EKMFA_CMD_BYTE_IDX_MAX; ByteIdx++)
        {
          PrintByteBin(CmdFromXany[ByteIdx]);
          if(ByteIdx < EKMFA_CMD_BYTE_IDX_MAX) Serial.print(F("."));
        }
        Serial.println();
        CfgMessage[1] = 0;
        Action = ACTION_ANSWER_WITH_REPONSE;
      }
    }
    else if(ACTION == REQUEST)
    {
#if (EKMFA_CMD_BYTE_NB == 1)
      sprintf_P(CfgMessage + 1, PSTR("=0x%02X (0b"), (uint16_t)(LastCmd & 0x000000FF));
      (LastCmd & 0x000000FF);
#endif
#if (EKMFA_CMD_BYTE_NB == 2)
      sprintf_P(CfgMessage + 1, PSTR("=0x%04X (0b"), (uint16_t)(LastCmd & 0x0000FFFF));
#endif
#if (EKMFA_CMD_BYTE_NB == 3)
      sprintf_P(CfgMessage + 1, PSTR("=0x%06lX (0b"), (uint32_t)(LastCmd & 0x00FFFFFF));
#endif
#if (EKMFA_CMD_BYTE_NB == 4)
      sprintf_P(CfgMessage + 1, PSTR("=0x%08lX (0b"), (uint32_t)(LastCmd));
#endif
      Serial.print(CfgMessage);
      for(uint8_t ByteIdx = 0; ByteIdx <= EKMFA_CMD_BYTE_IDX_MAX;ByteIdx++)
      {
        PrintByteBin(Ptr[EKMFA_CMD_BYTE_IDX_MAX - ByteIdx]);
        if(ByteIdx < EKMFA_CMD_BYTE_IDX_MAX) Serial.print(F("."));
      }
      strcpy_P(CfgMessage, PSTR(")"));
      Action = ACTION_ANSWER_WITH_REPONSE;
    }
    break;
    
#if (EKMFA_TX_DEBUG > 0) /* Set CONRAD7TX_DEBUG to 1 in Conrad7Tx.h to enable Debug */
    case DBG_CMD:
    EkmfaTx.debugProtocol((uint8_t)atoi(&ARG));
    CfgMessage[1] = 0; /* Echo */
    Action = ACTION_ANSWER_WITH_REPONSE;
    break;
#endif
  }
  if(Action != ACTION_ANSWER_WITH_REPONSE)
  {
    strcpy_P(CfgMessage, PSTR("ERR"));
  }
  Serial.println(CfgMessage);
  
}

void DisplayHelp(void)
{
  Serial.println(F("Help:"));
  Serial.println(F("To set the n EKMFA Outputs:"));
  Serial.println(F("F=n  (eg; F=1 for the 1st Function)"));
  Serial.println(F("or"));
  Serial.println(F("M=0xHexaValue  (eg; M=0x41)"));
  Serial.println(F("or"));
  Serial.println(F("M=DecimalValue (eg: M=65)"));
  Serial.println();  
  Serial.println(F("To read the n current EKMFA Outputs:"));
  Serial.println(F("M?"));
  Serial.println();
  Serial.println(F("To display the current pulse width of the current EKMFA frame:"));
  Serial.println(F("D=1"));
  Serial.println();
  Serial.println(F("To disabled the display of the current pulse width of the current EKMFA frame:"));
  Serial.println(F("D=0"));
}
