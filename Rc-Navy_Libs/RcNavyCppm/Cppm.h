#ifndef CPPM_H
#define CPPM_H 1
/* An interrupt driven RC PPM frame generator *and* RC PPM frame reader library using compare match of a 16 bits timer

   CPPM Generator (CppmGen object):
   ===============================
   Features:
   - Can generate a PPM Frame containing up to 12 RC Channels (8 channels 600 -> 2000 us with the 20ms default PPM period, up to 12 channels with higher PPM period).
   - Positive or Negative Modulation supported
   - Constant PPM Frame period: configurable from 10 to 40 ms (default = 20 ms)
   - No need to wait the PPM Frame period (usually 20 ms) to set the pulse width order for the channels, can be done at any time
   - Synchronisation indicator for digital data transmission over PPM
   - Blocking fonctions such as delay() can be used in the loop() since it's an interrupt driven PPM generator
   - Supported devices:
       - ATmega328P (Arduino UNO, Nano V3):
         TIMER(1), CHANNEL(A) -> OC1A -> PB1 -> Pin#9
         
   CPPM Reader (CppmReader object):
   ===============================
   Features:
   - Can read a PPM Frame containing up to 12 RC Channels (8 channels 600 -> 2000 us with the 20ms default PPM period, up to 12 channels with higher PPM period).
   - Automatic detection of:
      - Positive or Negative Modulation
      - The PPM frame period
      - Number of transported channels
      - Pulse width of each transported channel
   - Synchronisation indicator for digital data transmission over PPM
   - Blocking fonctions such as delay() can be used in the loop() since it's an interrupt driven PPM reader
   - Supported devices:
       - ATmega328P (Arduino UNO, Nano V3):
         TIMER(1), CHANNEL(A) -> ICP1 -> PB0 -> Pin#8

RC Navy 2015
   http://p.loussouarn.free.fr
   08/11/2016: Creation
*/
#include <Arduino.h>
#include <Rcul.h>

#define CPPM_FORWARD_CH_AS_IS(Ch)            CppmGen.width_us(Ch,    CppmReader.width_us(Ch))
#define CPPM_FORWARD_CH_TO_CH(SrcCh, DstCh)  CppmGen.width_us(DstCh, CppmReader.width_us(SrcCh))


/* PPM Modulation choices: Positive or Negative */
#define CPPM_GEN_POS_MOD                    HIGH
#define CPPM_GEN_NEG_MOD                    LOW

#define DEFAULT_PPM_PERIOD_US               20000
#define DEFAULT_PPM_HEADER_US               300

class CppmGenClass : public Rcul
{
  private:
    // static data
    uint16_t _PpmPeriod_us;
    uint16_t _PpmHeader_us;
    volatile uint16_t  *_Next_Tick_Nb = NULL;
    volatile uint8_t    _Synchro = 0;
    volatile uint8_t    _Modu = 1;
    volatile uint8_t    _Idx = 0;
    volatile uint8_t    _ChMaxNb;
    volatile uint8_t    _EndOfCh = 0;
  public:
    CppmGenClass(void);
    uint16_t begin(uint8_t PpmModu, uint8_t ChNb, uint16_t PpmPeriod_us = DEFAULT_PPM_PERIOD_US, uint16_t PpmHeader_us = DEFAULT_PPM_HEADER_US);
    void     header_us(uint16_t Header_us);
    void     width_us(uint8_t Ch, uint16_t Width_us);
    uint8_t  isSynchro(uint8_t SynchroClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free for other clients */
    void     suspend(void);
    void     resume(void);
    static void rcPpmGenIsr(void);
    /* Rcul support */
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = 255);
    virtual uint16_t RculGetWidth_us(uint8_t Ch);
};

#define CPPM_READER_CH_MAX             12

class CppmReaderClass : public Rcul
{
  private:
    // static data
    volatile uint8_t  _Synchro;
    volatile uint8_t  _Modu;
    volatile uint16_t _ChWidthTicks[CPPM_READER_CH_MAX];
    volatile uint8_t  _ChIdx;
    volatile uint8_t  _ChIdxMax;
    volatile uint16_t _PrevEdgeTicks;
    volatile uint16_t _StartPpmPeriodTicks;
    volatile uint16_t _PpmPeriodTicks;
  public:
    CppmReaderClass(void);
    void     begin(void);
    uint8_t  detectedChannelNb(void);
    uint8_t  modulation(void);
    uint16_t width_us(uint8_t Ch);
    uint16_t ppmPeriod_us(void);
    uint16_t ppmHeader_us(void);
    uint8_t  isSynchro(uint8_t SynchroClientIdx = 7); /* Default value: 8th Synchro client -> 0 to 6 free for other clients */
    void     suspend(void);
    void     resume(void);
    static void rcChannelCollectorIsr(void);
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = 255);
    virtual uint16_t RculGetWidth_us(uint8_t Ch);
};

extern CppmGenClass    CppmGen;    /* Object externalisation */
extern CppmReaderClass CppmReader; /* Object externalisation */

#endif
