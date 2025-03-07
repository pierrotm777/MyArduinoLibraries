#include <EepStruct.h>

/* Declarer ici l'oganisation de tes données en EEPROM dans une structure */
/* Ici, ce n'est que la description de l'organisation de tes données: rien n'est alloué ni en RAM ni en EPPROM: il faut voir ça commme un pochoir */
/* Ta structure peut contenir n'importe quoi, y compris d'autres structures, des tableaux de structures, etc... */
/* C'est cette structure qui va permettre de calculer *automatiquement* les adresses des differents champs en EEPROM */
typedef struct{
  uint8_t    Mode;
  float      Voltage;
  uint16_t   Tbl[4];
}MyEepStructSt_t;

#define EepOffset 0 // <-- Mettre ici l'offset (adresse de debut de structure en EEPROM pas forcment à 0)

//Tes Macros RD/WR --.           Src-.      Dst-.  Mettre ici le nom de ta structure --.      Ne pas toucher le reste!
//                   |               |          |                                      |
//                   V               V          V                                      v
#define MY_STRUCT_RD_EEP_TO_RAM(EepStFieldSrc, RamDst)  EEP_STRUCT_RD_EEP_TO_RAM(MyEepStructSt_t, EepOffset, EepStFieldSrc, RamDst)
#define MY_STRUCT_WR_RAM_TO_EEP(RamSrc, EepStFieldDst)  EEP_STRUCT_WR_RAM_TO_EEP(MyEepStructSt_t, EepOffset, RamSrc, EepStFieldDst)
//                                                                  ^
//                                                                  |
//                         Macros provenant de la lib EepStruct.h --'

/* Maintenant, tu as juste a utilser les 2 macros MY_STRUCT_RD_EEP_TO_RAM() et MY_STRUCT_WR_RAM_TO_EEP pour lire et ecrire depuis/vers l'EEPROM! */

void setup()
{
  uint8_t  Mode;
  float    Voltage;
  uint16_t Un_int16_t;

  Serial.begin(115200);
  
  Serial.println();
  Serial.print(F("Taille de la structure en EEPROM: "));Serial.print(sizeof(MyEepStructSt_t));Serial.println(F(" octets"));
  Serial.println();

  Voltage=12.0; // Initialise le flottant à une valeur ne creant pas d'overflow
  MY_STRUCT_WR_RAM_TO_EEP(&Voltage, Voltage); // Copie de la RAM vers l'EEPROM
  
  DumpEeprom(0, 20);//Affiche les 20 premiers octets de la structure en EEPROM 
  MY_STRUCT_RD_EEP_TO_RAM(Mode, &Mode); // Lit en eeprom le champ Mode et le recopie dans &Mode qui est en RAM (ici dans le stack, car variable locale)
  Serial.print(F("Mode="));Serial.println(Mode);
  Mode+=1; // Modifie la valeur en RAM
  Serial.print(F("Modify Mode to "));Serial.println(Mode);
  MY_STRUCT_WR_RAM_TO_EEP(&Mode, Mode); // Copie de la RAM vers l'EEPROM
  DumpEeprom(EepOffset, 20);
  Serial.println();
  
  MY_STRUCT_RD_EEP_TO_RAM(Voltage, &Voltage); // Lit en eeprom le champ Voltage et le recopie dans &Mode qui est en RAM (ici dans le stack, car variable locale)
  Serial.print(F("Voltage="));Serial.println(Voltage);
  Voltage+=1.0; // Modifie la valeur en RAM
  Serial.print(F("Modify Voltage to "));Serial.println(Voltage);
  MY_STRUCT_WR_RAM_TO_EEP(&Voltage, Voltage); // Copie de la RAM vers l'EEPROM
  DumpEeprom(EepOffset, 20);
  Serial.println();
  
  MY_STRUCT_RD_EEP_TO_RAM(Tbl[2], &Un_int16_t);// Lit en eeprom le champ Tbl[2] et le recopie dans &Un_int16_t qui est en RAM (ici dans le stack, car variable locale)
  Serial.print(F("Tbl[2]=0x"));Serial.println(Un_int16_t, HEX);
  DumpEeprom(EepOffset, 20);
  Un_int16_t+=1; // Modifie la valeur en RAM
  Serial.print(F("Modify Tbl[2] to 0x"));Serial.println(Un_int16_t, HEX);
  MY_STRUCT_WR_RAM_TO_EEP(&Un_int16_t, Tbl[2]);
  DumpEeprom(EepOffset, 20);
}

void loop()
{

}

void DumpEeprom(uint16_t Start, uint16_t Len)
{
  char Str[10];
  Serial.print("Start@=");Serial.print(Start);Serial.print(" Len=");Serial.println(Len);
  for(uint16_t Idx = 0; Idx < (Start + Len); Idx++)
  {
    sprintf(Str, "0x%02X ", eeprom_read_byte((void*)Idx));
    Serial.print(Str);
  }
  Serial.println();
}
