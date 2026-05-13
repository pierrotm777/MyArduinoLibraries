#ifndef RculPWMGen_h
#define RculPWMGen_h

/*
  RculPWMGen
  -----------
  Adaptation de SoftRcPulseOut / SoftwareServo pour generer des impulsions RC
  en microsecondes sur plateformes modernes.

  Objectif:
  - remplacer SoftRcPulseOut / Servo AVR dans un projet compatible RCUL
  - fonctionner sur ESP32 / ESP32-S3 et Teensy 4.0
  - conserver une API proche de SoftRcPulseOut:
      attach(), detach(), write(), write_us(), read(), read_us(), refresh()

  Note:
  - refresh() est une generation logicielle bloquante pendant la duree max des impulsions
    (~2500 us max). Elle doit etre appelee regulierement, comme SoftRcPulseOut.
  - Pour ESP32/Teensy, on stocke directement les largeurs en microsecondes, sans timer AVR.
*/

#include <Arduino.h>
#include <inttypes.h>

#if __has_include(<Rcul.h>)
  #include <Rcul.h>
  #define RCUL_PWM_GEN_HAS_RCUL 1
#else
  #define RCUL_PWM_GEN_HAS_RCUL 0
  #ifndef RCUL_DEFAULT_CLIENT_IDX
    #define RCUL_DEFAULT_CLIENT_IDX 0
  #endif
  #ifndef RCUL_NO_CH
    #define RCUL_NO_CH 0xff
  #endif
  class Rcul {
  public:
    virtual uint8_t  RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX) { (void)ClientIdx; return 0; }
    virtual void     RculSetWidth_us(uint16_t Width_us, uint8_t Ch = RCUL_NO_CH) { (void)Width_us; (void)Ch; }
    virtual uint16_t RculGetWidth_us(uint8_t Ch = RCUL_NO_CH) { (void)Ch; return 0; }
  };
#endif

#define RCUL_PWM_GEN_VERSION          1
#define RCUL_PWM_GEN_REVISION         0
#define RCUL_PWM_GEN_INSTANCE_MAX_NB  15

typedef struct {
  uint8_t
    ItMasked : 1,
    Inverted : 1,
    Reserved : 6;
} RculPWMGenBoolSt_t;

class RculPWMGen : public Rcul
{
  private:
    RculPWMGenBoolSt_t Bool;       // ItMasked et Inverted flags
    uint8_t            pin;
    uint8_t            angle;      // en degres
    uint16_t           pulse_us;   // largeur d'impulsion en microsecondes
    uint16_t           min_us;     // minimum pulse, default 544 us environ
    uint16_t           max_us;     // maximum pulse, default 2400 us
    class RculPWMGen  *next;

    static class RculPWMGen *first;

    static inline void writePin(uint8_t p, uint8_t level)
    {
#if defined(TEENSYDUINO)
      digitalWriteFast(p, level);
#else
      digitalWrite(p, level);
#endif
    }

    inline uint8_t activeLevel(void) const   { return (uint8_t)(HIGH ^ Bool.Inverted); }
    inline uint8_t inactiveLevel(void) const { return (uint8_t)(LOW  ^ Bool.Inverted); }

  public:
    RculPWMGen();

    uint8_t  attach(uint8_t pinArg, uint8_t Inverted = 0);
    void     detach();
    void     write(int angleArg);          // angle 0..180
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

    static int8_t       createInstance(void);
    static uint8_t      createdInstanceNb(void);
    static RculPWMGen  *RculPWMGenById(uint8_t ObjIdx);
    static int8_t       getIdByPin(uint8_t Pin);
    static uint8_t      destroyInstance(uint8_t ObjIdx);
    static uint8_t      refresh(uint8_t force = 0);
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
