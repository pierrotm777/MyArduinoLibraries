#pragma once


#include <Arduino.h>
#include <stdint.h>
#include <Rcul.h>

#define CPPM_GEN_POS_MOD                    HIGH
#define CPPM_GEN_NEG_MOD                    LOW

#define DEFAULT_PPM_PERIOD_US               22500
#define DEFAULT_PPM_HEADER_US               300
//=====================================================================
//=====================================================================
class ESP32_PPM : public Rcul 
{
private:

public:
    ESP32_PPM(void);

   
    void begin(bool PpmModu = CPPM_GEN_NEG_MOD ,uint8_t ChNb = 8,  uint16_t PpmPeriod_us = DEFAULT_PPM_PERIOD_US, uint16_t PpmHeader_us = DEFAULT_PPM_HEADER_US , uint8_t tx_pin = 13);
    void width_us( uint8_t Ch, uint16_t width_us ); 
	uint8_t  isSynchro(uint8_t SynchroClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free for other clients */
	
	 /* Rcul support */
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = 255);
    virtual uint16_t RculGetWidth_us(uint8_t Ch);
};

extern ESP32_PPM Cppm32;

