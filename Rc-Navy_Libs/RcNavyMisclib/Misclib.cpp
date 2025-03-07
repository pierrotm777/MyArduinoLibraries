#include "Misclib.h"

/**********************************************************************************************************
Cherche un mot-cle dans une table de pointeur de chaine de caractere situee en FLASH
**********************************************************************************************************/
int8_t GetKeywordIdFromTbl(char *CmdStr, const char * const* StrTbl, uint8_t StrTblItemNb)
{
  uint8_t Idx;
  int8_t  Ret = -1;

  for(Idx = 0; Idx < StrTblItemNb; Idx++)
  {
    if(!strncmp_P(CmdStr, (char*)pgm_read_ptr(&StrTbl[Idx]), strlen_P((char*)pgm_read_ptr(&StrTbl[Idx]))))
    {
      Ret = Idx;
      break;
    }
  }
  return(Ret);
}

int8_t  GetKeywordFromTbl(const char * const* StrTbl, uint8_t StrTblItemNb, uint8_t Idx, char *Buf, uint8_t BufSize)
{
  int8_t  Ret = -1;
  
  if(Idx < StrTblItemNb)
  {
    strncpy_P(Buf, (char*)pgm_read_ptr(&StrTbl[Idx]), BufSize - 1);
    Buf[BufSize - 1] = 0;
  }
  else Buf[0] = 0; //Empty string
  
  return(Ret);
}

/**********************************************************************************************************
Decompose une liste de chaines de caracteres separe par une chaine "separateur" en un tableau de chaines de
caracteres. Si la chaine initiale se termine par le separateur, le dernier element vide n'est pas compte.
Entree:
	SrcStr:		Pointeur sur la chaine a traiter
	Separ:		Pointeur sur la chaine "separateur"
	TarStrTbl:	Pointeur sur le tableau de chaines destinations
	TblLenMax:	Nombre Max de sous-chaines
Sortie:
	Le nombre de sous-chaine
	SeparFound: Le nombre de Separateur trouve ( sera utilise pour StrSplitRestore() )
Attention: cette fonction optimisee pour l'embarque remplace le 1er caractere de chaque separateur par \0
Pour restaurer la chaine d'origine, appeler StrSplitRestore(char *Separ, char **TarStrTbl, uint8_t SeparNbToRestore) apres
utilisation des sous-chaines disponible dans TarStrTbl.
**********************************************************************************************************/
int8_t StrSplit(char *SrcStr, char *Separ,  char **TarStrTbl, uint8_t TblLenMax, uint8_t *SeparFound)
{
  uint8_t SubStrFound, SeparLen;
  char *StartStr, *SeparPos;

  *SeparFound = 0;
  SubStrFound = 0;
  /* Clear Table Entries (secu) in case of empty string */
  for(uint8_t Idx = 0; Idx < TblLenMax; Idx++)
  {
    TarStrTbl[Idx] = NULL; /* NULL pointer */
  }
  if (!strlen(SrcStr)) return(0);
  StartStr = SrcStr; /* Do NOT touch SrcStr */
  SeparLen = strlen(Separ);
  while(TblLenMax)
  {
    SeparPos = strstr(StartStr, Separ);
    if(SeparPos)
    {
      /* OK Substring found */
      *SeparPos = 0;
      TarStrTbl[(*SeparFound)++] = StartStr;
      StartStr = SeparPos + SeparLen;
      SubStrFound++;
    }
    else
    {
      SubStrFound = *SeparFound;
      if(*StartStr)
      {
        TarStrTbl[*SeparFound] = StartStr;
        SubStrFound++;
      }
      break;
    }
    TblLenMax--;
 }
  return(SubStrFound);
}

/**********************************************************************************************************
Cette fonction optimisee pour l'embarque restore la chaine SrcStr splittée a l'aide la fonction
StrSplit(char *SrcStr, char *Separ,  char **TarStrTbl, uint8_t SeparNbToRestore).
Important Note: SeparNbToRestore SHALL be <= TblLenMax passed as argument for StrSplit().
**********************************************************************************************************/
int8_t StrSplitRestore(char *Separ,  char **TarStrTbl, uint8_t SeparNbToRestore)
{
  uint8_t Idx, Restored = 0;
  
  for(Idx = 0; Idx < SeparNbToRestore; Idx++)
  {
    /* Restore \0 with first char of Separator */
    if(TarStrTbl[Idx] != NULL)
    {
      TarStrTbl[Idx][strlen(TarStrTbl[Idx])] = Separ[0];
      Restored++;
    }
  }
  return(Restored);
}

char *ltrim(char *str)
{
  int len = strlen(str);
  char *cur = str;

  while (*cur && isspace(*cur))
  {
    ++cur;
    --len;
  }
  if (str != cur) memmove(str, cur, len + 1);

  return str;
}

char *rtrim(char *str)
{
  int len = strlen(str);
  char *cur = str + len - 1;

  while (cur != str && isspace(*cur)) --cur;
  cur[isspace(*cur) ? 0 : 1] = '\0';

  return str;
}

char *trim(char *str)
{
  rtrim(str);
  ltrim(str);
  return str;
}

uint16_t HexAsciiToInt16(char* Msg)
{
uint8_t  Idx,     Res8;
uint16_t Res16=0, Coef=4096;

  for(Idx=0;Idx<4;Idx++)
  {
    if(HexAsciiNibbleToInt(Msg[Idx], &Res8))
    {
      Res16+=(Coef*Res8);
      Coef>>=4;
    }
  }
  return(Res16);
}

uint8_t HexAsciiNibbleToInt(char HexAsciiNibble, uint8_t *Res)
{
uint8_t Ret=1;

  if(HexAsciiNibble>='0' && HexAsciiNibble<='9')
  {
    *Res=HexAsciiNibble-'0'; /* From 0 to 9 */
  }
  else
  {
    if(toupper(HexAsciiNibble)>='A' && toupper(HexAsciiNibble)<='F')
    {
      *Res=10+HexAsciiNibble-'A'; /* From 10 to 15 */
    }
    else Ret=0;
  }
  return(Ret);
}
