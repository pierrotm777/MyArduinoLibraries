#include <GenCli.h>

#ifdef CR
#undef CR
#endif

#ifdef LF
#undef LF
#endif

#ifdef RUB_OUT
#undef RUB_OUT
#endif

#define CR       13
#define LF       10
#define RUB_OUT  8

int8_t GenCli_getCmd(Stream *Term, uint8_t &WrIdx, char *TxRxBuf, uint8_t TxRxBufLen)
{
  char           RxChar;
  int8_t         Ret = -1;
  
  while(Term->available())
  {
    RxChar = Term->read();
    switch(RxChar)
    {
      case LF:
      /* Just ignore */
      break;

      case CR:
      TxRxBuf[WrIdx] = 0; /* Replace CR by End of String */
      Ret = WrIdx;
      WrIdx = 0;
      break;

      case RUB_OUT:
      if(WrIdx) WrIdx--; /* Remove the last character */
      break;

      default:
      if(WrIdx < (TxRxBufLen - 1))
      {
        TxRxBuf[WrIdx] = RxChar;
        WrIdx++;
      }
      else
      {
        /* Too long -> ignore: the next CR will terminate (and troncate) the sring */
      }
      break;
    }
    if(Ret >= 0)
    {
      break;
    }
  }
  return(Ret);
}

int8_t GenCli_getStrIdxFromTbl(char *Str, const char * const StrTbl[], uint8_t TblItemNb, uint8_t *MatchLen /*= NULL*/)
{
  uint8_t StrLen;
  int8_t Ret = -1;

  if(MatchLen) *MatchLen = 0; /* If MatchLen not optional */
  for(uint8_t Idx = 0; Idx < TblItemNb; Idx++)
  {
    StrLen = strlen_P((const char *)pgm_read_ptr(&StrTbl[Idx]));
    if(!memcmp_P(Str, (const char *)pgm_read_ptr(&StrTbl[Idx]), StrLen))
    {
      if(MatchLen) *MatchLen = StrLen; /* If MatchLen not optional */
      Ret = Idx;
      break;
    }
  }
  return(Ret);
}

int8_t GenCli_getCmdCodeIdx(char *CmdStr, char *Sep, const GenCliFldTblDescSt_t *FldTblDesc, const GenCliCmdSt_t *CmdTbl, uint8_t CmdTblItemNb, GenCliCmdFldSt_t *Fld, int8_t FldMaxNb)
{
  uint8_t             FldNb, DotSeparFound, TblItemNb, MatchLen;
  int8_t              FldMapIdx[FldMaxNb];
  const char * const *FldStrTbl = NULL;
  char                *CharLoc = NULL;
  int8_t              Ret = 0;

  /* 1) Split command into Fields */
  CharLoc = strchr(CmdStr, '=');
  if(CharLoc)
  {
    *CharLoc = 0; /* Replace '=' with End of String (in case Sep is present at the right of '=') */
  }
  FldNb = StrSplit(CmdStr, Sep, Fld->Ptr, FldMaxNb + 1, &DotSeparFound); // +1 to detect over field
  if(CharLoc)
  {
    *CharLoc = '='; /* Restore '=' after StrSplit() */
  }
//DISPLAY_SPLIT_STR_TBL(&Serial, Fld->Ptr);
  if(FldNb <= FldMaxNb)
  {
    /* 2) Init command Field Str Index to -1 (not found) */
    for(uint8_t Idx = 0; Idx < FldMaxNb; Idx++)
    {
      FldMapIdx[Idx]     = -1;
      Fld->Id[Idx]       = -1;
      Fld->MatchLen[Idx] = 0;
      Fld->Action        = NULL;
    }
    if(!CmdStr[0])
    {
      Fld->Ptr[0] = CmdStr; /* if Empty string, set Fld->Ptr[0] to CmdStr to manage error arrow */
    }
    /* 3) Build the Command Field Map and Idx */
    for(uint8_t FieldIdx = 0; FieldIdx < FldNb; FieldIdx++)
    {
      FldStrTbl = (const char * const *)pgm_read_ptr(&FldTblDesc[FieldIdx].FldStrTbl);
      TblItemNb = (uint8_t)pgm_read_byte(&FldTblDesc[FieldIdx].FldStrTblItemNb);
      FldMapIdx[FieldIdx] = GenCli_getStrIdxFromTbl(Fld->Ptr[FieldIdx], FldStrTbl, TblItemNb, &MatchLen);
      if(FldMapIdx[FieldIdx] >= 0)
      {
        Fld->MatchLen[FieldIdx] = MatchLen;
        if(Fld->Ptr[FieldIdx][MatchLen])
        {
          Fld->Id[FieldIdx] = atoi(Fld->Ptr[FieldIdx] + MatchLen);
          if(FieldIdx == (FldNb - 1)) /* word, wordDDD */
          {
            /* Look for '?' or '=' in last field */
            CharLoc = strchr(Fld->Ptr[FieldIdx], '?');
            if(CharLoc)
            {
              Fld->Action = CharLoc;
            }
            else
            {
              CharLoc = strchr(Fld->Ptr[FieldIdx], '=');
              if(CharLoc)
              {
                Fld->Action = CharLoc;
              }
            }
          }
        }
        else
        {
          Fld->Id[FieldIdx] = 0; // For consistency
        }
        if(!CharLoc)
        {
          Fld->Action = Fld->Ptr[FieldIdx] + MatchLen; // By default: Action ptr is placed at the end of the field
        }
      }
      else
      {
        Ret = -(FieldIdx + 1);
        break;
      }
    }
    /* 4) Search global command index matching with the command field map */
    if(Ret >= 0)
    {
      Ret = -1;
      for(uint8_t CmdIdx = 0; CmdIdx < CmdTblItemNb; CmdIdx++)
      {
        if(!memcmp_P((void*)FldMapIdx, (void*)&CmdTbl[CmdIdx].Field[0], FldMaxNb))
        {
          Ret = CmdIdx;
          break;
        }
      }
    }
  }
  else
  {
    Ret = -(FldMaxNb + 1);
  }
  /* 5) Restore initial command string */
  StrSplitRestore(Sep, Fld->Ptr, DotSeparFound);

  return(Ret);
}

void GenCli_displayCmdErr(Stream *Term, char *CmdStr, GenCliCmdFldSt_t *Fld, uint8_t FldIdx, uint8_t ErrIdx, char *CustomErrMsg /*= NULL*/)
{
  uint8_t ErrArrowPos = 0;
  
  switch(ErrIdx)
  {
    case GEN_CLI_ERR_UNKNOWN_FIELD: // UNKNOWN Field
    ErrArrowPos = Fld->Ptr[FldIdx] - CmdStr;
    break;
    
    case GEN_CLI_ERR_ID: // Error ID
    ErrArrowPos = Fld->Ptr[FldIdx] + Fld->MatchLen[FldIdx] - CmdStr;
    break;

    case GEN_CLI_ERR_ARG: // Error Argument
    ErrArrowPos = Fld->Action + 1 - CmdStr;
    break;
    
    default: // When ErrIdx >= 10, this means an error at a specific location in the Argument (to display correct location of the arrow)
    if(ErrIdx >= 10)
    {
      ErrArrowPos = Fld->Action + 1 - CmdStr + (ErrIdx - 10);
      ErrIdx = GEN_CLI_ERR_ARG;
    }
    break;
  }
  if(ErrArrowPos)
  {
    for(uint8_t Idx = 0; Idx < ErrArrowPos; Idx++)
    {
      Term->print(F(" "));
    }
  }
  Term->print(F("^--Err "));
  if(!CustomErrMsg)
  {
    switch(ErrIdx)
    {
      case GEN_CLI_ERR_UNKNOWN_FIELD: // UNKNOWN Field
      Term->print(F("Cmd"));
      break;
      
      case GEN_CLI_ERR_ID: // Error ID
      Term->print(F("Id"));
      break;

      case GEN_CLI_ERR_ARG: // Error Argument
      Term->print(F("Arg"));
      break;
      
      default:
      break;
    }
  }
  else
  {
    Term->print(CustomErrMsg);
  }
  Term->println();
}

void GenCli_displayCmdParsedInFld(Stream *Term, GenCliCmdFldSt_t *Fld)
{
  uint8_t FldIdx = 0, FldLen;
  char Field[20 + 1];
  while(Fld->Ptr[FldIdx])
  {
    if(Fld->Ptr[FldIdx + 1])
    {
      FldLen = (Fld->Ptr[FldIdx + 1] - Fld->Ptr[FldIdx]) - 1;
      strncpy(Field, Fld->Ptr[FldIdx], (FldLen < 20)? FldLen: 20);
      if((FldLen < 20)) Field[FldLen] = 0;
      else              Field[20] = 0;
      Term->print(F("Fld["));Term->print(FldIdx);Term->print(F("]='"));Term->print(Field);
    }
    else
    {
      Term->print(F("Fld["));Term->print(FldIdx);Term->print(F("]='"));Term->print(Fld->Ptr[FldIdx]);
    }
    Term->print(F("'\tMatchLen="));Term->print(Fld->MatchLen[FldIdx]);
    Term->print(F("\tId="));Term->print(Fld->Id[FldIdx]);
    if(!Fld->Ptr[FldIdx + 1])
    {
      if(Fld->Action)
      {
        Term->print(F("\tAction='"));Term->print(Fld->Action[0]);Term->print(F("'"));
        if(Fld->Action[0] == '=')
        {
          Term->print(F("\tArg="));Term->print(Fld->Action + 1);
        }
      }
    }
    Term->println();
    FldIdx++;
  }
}
