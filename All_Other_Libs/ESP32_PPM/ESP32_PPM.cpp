#include "ESP32_PPM.h"

#define MAX_PPM_CHANNELS_COUNT 16//8


static hw_timer_t *timer = NULL;
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;



ESP32_PPM Cppm32 = ESP32_PPM();

//=====================================================================
//=====================================================================
ESP32_PPM::ESP32_PPM(void)
{
}

static uint8_t _tx_pin;
static uint8_t _ChNb;
static int _PPMFrameLengthUS;

static uint16_t _PpmModu ;
static uint16_t _PpmHeader_us;


static volatile int outChannelsIndex = 0;
static volatile uint8_t    _Synchro = 0;

static uint16_t outChannelValues[MAX_PPM_CHANNELS_COUNT];
static uint16_t channelValues[MAX_PPM_CHANNELS_COUNT];

//=====================================================================
//=====================================================================
enum ppmState_e
{
    PPM_STATE_PULSE,
    PPM_STATE_FILL,
    PPM_STATE_SYNC,
    PPM_STATE_FAILSAFE
};


//=====================================================================
//=====================================================================

//https://quadmeup.com/how-to-generate-ppm-signal-with-esp32-and-arduino/
//https://github.com/ps-after-hours/esp32_ppm_output/blob/main/esp32_ppm_output.ino
//https://github.com/RomanLut/hx_espnow_rc/tree/main/lib/hx_ppm_encoder


void IRAM_ATTR onTimerISR()
{
    static volatile uint8_t ppmState = PPM_STATE_PULSE;
    static volatile uint8_t ppmChannelIndex = 0;
    static volatile int usedFrameLengthUS = 0;

    portENTER_CRITICAL(&timerMux);

    switch ( ppmState ) 
    {
    
    case PPM_STATE_PULSE:
        digitalWrite(_tx_pin, !_PpmModu);
        //timerAlarmWrite(timer,PPM_PULSE_LENGTH_US, true);   
		 timerAlarm(timer,_PpmHeader_us, true,0);   
        ppmState = PPM_STATE_FILL;
        break;
    
    case PPM_STATE_FILL:
        digitalWrite(_tx_pin, _PpmModu);

        ppmState = PPM_STATE_PULSE;

        if (ppmChannelIndex == _ChNb)
        {
            //timerAlarmWrite(timer,PPMFrameLengthUS - usedFrameLengthUS, true);   
			
			timerAlarm(timer,_PPMFrameLengthUS - usedFrameLengthUS, true,0);
      
            ppmChannelIndex = 0;
            usedFrameLengthUS = 0;
			_Synchro = 0xFF;

           
        }
        else
        {
            uint16_t currentChannelValue = outChannelValues[ppmChannelIndex];
            //timerAlarmWrite(timer,currentChannelValue - PPM_PULSE_LENGTH_US, true);   
			timerAlarm(timer,currentChannelValue - _PpmHeader_us, true,0);   
            usedFrameLengthUS += currentChannelValue;
            ppmChannelIndex++;
        }
        break;
    }

  portEXIT_CRITICAL(&timerMux);
}




//=====================================================================
//=====================================================================
void ESP32_PPM::begin( bool PpmModu,uint8_t ChNb,  uint16_t PpmPeriod_us, uint16_t PpmHeader_us , uint8_t tx_pin )
{
    _tx_pin = tx_pin;
    _ChNb = ChNb;
	_PpmModu = PpmModu;
	_PpmHeader_us =PpmHeader_us;
	_PPMFrameLengthUS = PpmPeriod_us;
	_Synchro  = 0;

    for ( int i = 0; i < MAX_PPM_CHANNELS_COUNT; i++)
    {
        outChannelValues[i] = 1500;
       
        channelValues[i] = 1500;
    }

    pinMode(_tx_pin, OUTPUT);
    digitalWrite(_tx_pin, LOW);


   /* timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimerISR, true);
    timerAlarmWrite(timer, 12000, true);
    timerAlarmEnable(timer);*/
	timer = timerBegin(1000000);
	timerAttachInterrupt(timer,  &onTimerISR);
	timerAlarm(timer, 12000, true, 0);


}


//=====================================================================
//=====================================================================

void ESP32_PPM::width_us( uint8_t Ch, uint16_t width_us ) 

{
     uint8_t _Ch;
	 _Ch= Ch-1;
	
	if ( Ch >= MAX_PPM_CHANNELS_COUNT ) return;
    channelValues[_Ch] = constrain( width_us, 1000, 2000 );
	
	outChannelValues[_Ch] = channelValues[_Ch];
 
  
}
uint8_t ESP32_PPM::isSynchro(uint8_t SynchroClientIdx /*= 7*/)
{
  uint8_t Ret;
  
  Ret = !!(_Synchro & RCUL_CLIENT_MASK(SynchroClientIdx));
  if(Ret) _Synchro &= ~RCUL_CLIENT_MASK(SynchroClientIdx); /* Clear indicator for the Synchro client */
  
  return(Ret);
}

/* Begin of Rcul support */
uint8_t ESP32_PPM::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  return(isSynchro(ClientIdx));
}

void ESP32_PPM::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= 255*/)
{
  this->width_us(Ch, Width_us); /* Take care about argument order (like that, since Ch will be optional for other pulse generator) */
}

uint16_t ESP32_PPM::RculGetWidth_us(uint8_t Ch)
{
  Ch = Ch; /* To avoid a compilation warning */
  return(0);
}

/* End of Rcul support */
