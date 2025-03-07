#ifndef GEN_CLI_H
#define GEN_CLI_H

/* Libray Version/Revision */
#define GEN_CLI_VER    1
#define GEN_CLI_REV    4

/*
  Version/Revision history:
  V1.0:         Initial release
  V1.0 -> V1.1: Bug fixed when '.' (field separator) is present in the value of a command (at the right of '=')
  V1.1 -> V1.2: Support for ESP32 added
  V1.2 -> V1.3: Support of custom error at a specific index of the argument
  V1.3 -> V1.4: GenInter library renamed as GenCli. Support of Rubbout to erase the last character during input in the Serial Terminal
*/

#include <Misclib.h>

#define GEN_CLI_CMD_FIELD_MAX_NB              2 // <-- Define here the command field number max

#define GEN_CLI_FLD_TBL_ATTR(FldTbl)          {FldTbl, TBL_ITEM_NB(FldTbl)}
#define GEN_CLI_FLD_TBL_LIST(CmdFldListTbl)   DECL_FLASH_TBL(GenCliFldTblDescSt_t, CmdFldListTbl)
#define GEN_CLI_CMD_TBL(CmdTbl)               DECL_FLASH_TBL(GenCliCmdSt_t, CmdTbl)

enum {GEN_CLI_ERR_UNKNOWN_FIELD = 0, GEN_CLI_ERR_ID, GEN_CLI_ERR_ARG};

typedef struct{
  char   *Ptr[GEN_CLI_CMD_FIELD_MAX_NB + 1]; // +1 to detect over field
  int8_t  Id[GEN_CLI_CMD_FIELD_MAX_NB];
  uint8_t MatchLen[GEN_CLI_CMD_FIELD_MAX_NB];
  char   *Action;
}GenCliCmdFldSt_t;

typedef struct{
  uint8_t      Code;
  int8_t       Field[GEN_CLI_CMD_FIELD_MAX_NB];
}GenCliCmdSt_t;

typedef struct{
  const char * const *FldStrTbl;
  uint8_t             FldStrTblItemNb;
}GenCliFldTblDescSt_t;

int8_t GenCli_getCmd(Stream *Term, uint8_t &WrIdx, char *TxRxBuf, uint8_t TxRxBufLen);
int8_t GenCli_getStrIdxFromTbl(char *Str, const char * const StrTbl[], uint8_t TblItemNb, uint8_t *MatchLen = NULL);
int8_t GenCli_getCmdCodeIdx(char *CmdStr, char *Sep, const GenCliFldTblDescSt_t *FldTblDesc, const GenCliCmdSt_t *CmdTbl, uint8_t CmdTblItemNb, GenCliCmdFldSt_t *Fld, int8_t FldMaxNb); // Do NOT use this method, rather, use the macro below
void   GenCli_displayCmdErr(Stream *Term, char *CmdStr, GenCliCmdFldSt_t *Fld, uint8_t FldIdx, uint8_t IdErr = 0, char *CustomErrMsg = NULL);
void   GenCli_displayCmdParsedInFld(Stream *Term, GenCliCmdFldSt_t *Fld);

#endif
