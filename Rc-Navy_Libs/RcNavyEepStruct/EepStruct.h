#ifndef EEP_STRUCT_H
#define EEP_STRUCT_H

#include <avr/eeprom.h>

#define EEP_STRUCT_RD_EEP_TO_RAM(EepStName, EepOffset, EepStFieldSrc, RamDst)  eeprom_read_block  ((void *)(RamDst), (const void *)(EepOffset + offsetof(EepStName, EepStFieldSrc)), sizeof(((EepStName *)NULL)->EepStFieldSrc))
#define EEP_STRUCT_WR_RAM_TO_EEP(EepStName, EepOffset, RamSrc, EepStFieldDst)  eeprom_update_block((void *)(RamSrc), (      void *)(EepOffset + offsetof(EepStName, EepStFieldDst)), sizeof(((EepStName *)NULL)->EepStFieldDst))


#endif
