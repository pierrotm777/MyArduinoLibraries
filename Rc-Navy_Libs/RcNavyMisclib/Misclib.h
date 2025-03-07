#ifndef MISC_LIB_H
#define MISC_LIB_H

#include "Arduino.h"
#include <string.h>

#define STRING_(string)                                #string
#define PRJ_VER_REV(PrjName, Ver, Rev)                 STRING_(PrjName) " V" STRING_(Ver) "." STRING_(Rev)
#define PRJ_VER_REV_COPYRIGHT(PrjName, Ver, Rev, Cop)  STRING_(PrjName) " V" STRING_(Ver) "." STRING_(Rev) " " Cop

#define TBL_ITEM_NB(Tbl)                (sizeof(Tbl) / sizeof(Tbl[0]))

#define DECL_FLASH_TBL(Type, Tbl)       const Type Tbl           [] PROGMEM
#define DECL_FLASH_STR(Str)             const char _ ## Str ## _ [] PROGMEM = #Str
#define DECL_FLASH_STR2(Str,Str2)       const char _ ## Str ## _ [] PROGMEM = #Str2
#define DECL_FLASH_STR_TBL(StrTbl)      DECL_FLASH_TBL(char * const, StrTbl)

#define TBL_AND_ITEM_NB(Tbl)            Tbl, TBL_ITEM_NB(Tbl)
#define BUF_AND_BUF_SIZE(Buf)           Buf, sizeof(Buf)

#define DISPLAY_SPLIT_STR_TBL(OutStreamPtr, Tbl) do{ \
    for(uint8_t Idx = 0; Idx < TBL_ITEM_NB(Tbl); Idx ++)\
    {\
      if(Tbl[Idx])\
      {\
        (Stream *)(OutStreamPtr)->print(#Tbl);(Stream *)(OutStreamPtr)->print(F("["));(Stream *)(OutStreamPtr)->print((int)Idx);(Stream *)(OutStreamPtr)->print(F("] = "));\
        (Stream *)(OutStreamPtr)->println(Tbl[Idx]);\
      }\
    }\
  }while(0)

#define ElapsedMsSince(StartMs)        (millis() - (uint32_t)(StartMs))

#define millis8()                      (uint8_t)(millis() & 0x000000FF)
#define ElapsedMs8Since(StartMs8)      (uint8_t)(millis8() - (uint8_t)(StartMs8))

#define millis16()                     (uint16_t)(millis() & 0x0000FFFF)
#define ElapsedMs16Since(StartMs16)    (uint16_t)(millis16() - (uint16_t)(StartMs16))

#define ElapsedUsSince(StartUs)        (micros() - (uint32_t)(StartUs))

#define micros16()                     (uint16_t)(micros() & 0x0000FFFF)
#define ElapsedUs16Since(StartUs16)    (uint16_t)(micros16() - (uint16_t)(StartUs16))

#define MODULO(Var, Mod)               !((Var) % (Mod))

int8_t  GetKeywordIdFromTbl(char *CmdStr, const char * const* StrTbl, uint8_t StrTblItemNb);
int8_t  GetKeywordFromTbl(const char * const* StrTbl, uint8_t StrTblItemNb, uint8_t Idx, char *Buf, uint8_t BufSize);

int8_t  StrSplit(char *SrcStr, char *Separ,  char **TarStrTbl, uint8_t TblLenMax, uint8_t *SeparFound);
int8_t  StrSplitRestore(char *Separ,  char **TarStrTbl, uint8_t SeparNbToRestore);
char    *ltrim(char *str);
char    *rtrim(char *str);
char    *trim(char *str);
uint16_t HexAsciiToInt16(char* Msg);
uint8_t  HexAsciiNibbleToInt(char HexAsciiNibble, uint8_t *Res);

#endif
