#ifndef RculPWMGen_h
#define RculPWMGen_h

/*
  RculPWMGen
  ----------
  Portable adaptation of RcNavy SoftRcPulseOut for modern Arduino targets.

  Main targets:
    - ESP32 family (including ESP32-C3 / ESP32-S3)
    - Teensy (including Teensy 4.x)

  Design intentionally follows SoftRcPulseOut:
    - linked list of instances
    - common 20 ms refresh()
    - all pulses start together
    - outputs are sorted by pulse width
    - interrupts stay enabled during most of the pulse
    - interrupts are masked only shortly before each falling edge
    - close falling edges are handled while interrupts remain masked

  This avoids the explicit yield() used by the previous RculPWMGen attempt,
  which could add unnecessary scheduling jitter close to a falling edge.

  API kept close to SoftRcPulseOut:
    attach(), detach(), write(), write_us(), read(), read_us(), attached(),
    setMinimumPulse(), setMaximumPulse(), refresh()

  RCUL support:
    RculIsSynchro(), RculSetWidth_us(), RculGetWidth_us()
*/

#include <Arduino.h>
#include <Rcul.h>
#include <inttypes.h>

#define RCUL_PWM_GEN_VERSION            1
#define RCUL_PWM_GEN_REVISION           1
#define RCUL_PWM_GEN_INSTANCE_MAX_NB    15

/* Same default pulse range as SoftRcPulseOut: 34*16=544, 150*16=2400 us. */
#define RCUL_PWM_GEN_DEFAULT_MIN_US      544U
#define RCUL_PWM_GEN_DEFAULT_MAX_US      2400U

/* Same nominal refresh period as SoftRcPulseOut. */
#define RCUL_PWM_GEN_REFRESH_PERIOD_US   20000UL

/*
  SoftRcPulseOut masks interrupts a few timer ticks before each falling edge.
  On modern targets we use an explicit microsecond guard.
  16 us is intentionally conservative and still very short compared with
  a normal 1000..2000 us RC pulse.
*/
#ifndef RCUL_PWM_GEN_EDGE_GUARD_US
#define RCUL_PWM_GEN_EDGE_GUARD_US       16U
#endif

/* Consecutive falling edges separated by <= this value stay interrupt-masked. */
#ifndef RCUL_PWM_GEN_CLOSE_PULSE_US
#define RCUL_PWM_GEN_CLOSE_PULSE_US      5U
#endif

typedef struct
{
  uint8_t
    ItMasked : 1,
    Inverted : 1,
    Reserved : 6;
} RculPWMGenBoolSt_t;

class RculPWMGen : public Rcul
{
  private:
    RculPWMGenBoolSt_t Bool;
    uint8_t            pin;
    uint8_t            angle;
    uint16_t           pulse_us;
    uint16_t           min_us;
    uint16_t           max_us;
    class RculPWMGen  *next;

    static class RculPWMGen *first;

    static inline void writePin(uint8_t Pin, uint8_t Level)
    {
#if defined(TEENSYDUINO)
      digitalWriteFast(Pin, Level);
#else
      digitalWrite(Pin, Level);
#endif
    }

    inline uint8_t activeLevel(void) const
    {
      return (uint8_t)(HIGH ^ Bool.Inverted);
    }

    inline uint8_t inactiveLevel(void) const
    {
      return (uint8_t)(LOW ^ Bool.Inverted);
    }

  public:
    RculPWMGen();

    uint8_t  attach(uint8_t pinArg, uint8_t Inverted = 0);
    void     detach();
    void     write(int angleArg);
    void     write_us(uint16_t PulseWidth_us);
    uint8_t  read();
    uint16_t read_us();
    uint8_t  attached();
    void     setMinimumPulse(uint16_t t);
    void     setMaximumPulse(uint16_t t);

    /* Rcul support */
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = RCUL_NO_CH);
    virtual uint16_t RculGetWidth_us(uint8_t Ch = RCUL_NO_CH);

    static int8_t      createInstance(void);
    static uint8_t     createdInstanceNb(void);
    static RculPWMGen *RculPWMGenById(uint8_t ObjIdx);
    static int8_t      getIdByPin(uint8_t Pin);
    static uint8_t     destroyInstance(uint8_t ObjIdx);

    /*
      Must be called regularly.
      Returns 1 when a 20 ms refresh has actually been performed.
      force=1 forces one refresh immediately.
    */
    static uint8_t refresh(uint8_t force = 0);
};

/* Methodes en Francais */
#define attache                         attach
#define detache                         detach
#define ecrit                           write
#define ecrit_us                        write_us
#define lit                             read
#define lit_us                          read_us
#define estAttache                      attached
#define definitImpulsionMinimum         setMinimumPulse
#define definitImpulsionMaximum         setMaximumPulse
#define rafraichit                      refresh
#define creerInstance                   createInstance
#define nbInstanceCrees                 createdInstanceNb
#define detruireInstance                destroyInstance
#define RculPWMGenParId                 RculPWMGenById
#define idPourBroche                    getIdByPin

#endif
