#ifndef CPPMREADER
#define CPPMREADER 1
/* PPMRead2 is a fork of SoftRcPulseIn (Rc-Navy) , a tiny interrupt driven RC CPPM frame reader library
   Features:
   - Uses any input supporting interrupt pin change
   - Positive and negative CPPM modulation supported (don't care)
   - Up to 9 RC channels supported
   RC Navy 2015
   http://p.loussouarn.free.fr
   09/01/2025: Creation

*/
#include <Arduino.h>
#include <Rcul.h>

#define CPPM_READER_CH_MAX  16

/* Public function prototypes */
class RculPPMRead : public Rcul
{
  public:
    RculPPMRead();
    static uint8_t  attach(uint8_t CppmInputPin);
    //static uint8_t  detach(void);
    void            trackChId(uint8_t ChId); // ChId in [1..16]
    static uint8_t  detectedChannelNb(void);
    static uint16_t width_us(uint8_t Ch);
    static uint16_t cppmPeriod_us(void);
    static uint8_t  isSynchro(uint8_t ClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free for other clients*/
    static void     rcChannelCollectorIsr(void);
    /* Rcul support */
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual uint16_t RculGetWidth_us(uint8_t Ch);
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = RCUL_NO_CH);
  private:
    // static data
};

#endif
