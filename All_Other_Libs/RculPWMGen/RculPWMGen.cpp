#include <RculPWMGen.h>

RculPWMGen *RculPWMGen::first = NULL;

typedef struct {
  uint8_t
    Total       : 4,
    Dynamically : 4;
} RculPWMGenObjectCreatedSt_t;

static RculPWMGenObjectCreatedSt_t ObjectCreated = {0, 0};

#define RCUL_PWM_GEN_NO_ANGLE      (0xff)
#define RCUL_PWM_GEN_NOT_ATTACHED  (0xff)
#define RCUL_PWM_GEN_DEFAULT_MIN_US 544
#define RCUL_PWM_GEN_DEFAULT_MAX_US 2400
#define RCUL_PWM_GEN_DEFAULT_US     1500

RculPWMGen::RculPWMGen()
{
  pin      = RCUL_PWM_GEN_NOT_ATTACHED;
  angle    = RCUL_PWM_GEN_NO_ANGLE;
  pulse_us = RCUL_PWM_GEN_DEFAULT_US;
  min_us   = RCUL_PWM_GEN_DEFAULT_MIN_US;
  max_us   = RCUL_PWM_GEN_DEFAULT_MAX_US;
  Bool.ItMasked = 0;
  Bool.Inverted = 0;
  next     = first;
  first    = this;
  ObjectCreated.Total++;
}

void RculPWMGen::setMinimumPulse(uint16_t t)
{
  min_us = t;
  if (pulse_us < min_us) pulse_us = min_us;
}

void RculPWMGen::setMaximumPulse(uint16_t t)
{
  max_us = t;
  if (pulse_us > max_us) pulse_us = max_us;
}

int8_t RculPWMGen::createInstance(void)
{
  int8_t Ret = -1;

  if (ObjectCreated.Total < RCUL_PWM_GEN_INSTANCE_MAX_NB)
  {
    RculPWMGen *p = new RculPWMGen;
    if (p)
    {
      ObjectCreated.Dynamically++;
      Ret = ObjectCreated.Total - 1;
    }
  }
  return Ret;
}

uint8_t RculPWMGen::createdInstanceNb(void)
{
  return ObjectCreated.Total;
}

uint8_t RculPWMGen::destroyInstance(uint8_t ObjIdx)
{
  uint8_t Ret = 0;

  if ((ObjIdx >= (ObjectCreated.Total - ObjectCreated.Dynamically)) && (ObjIdx < ObjectCreated.Total))
  {
    RculPWMGen *This = RculPWMGenById(ObjIdx);
    if (This && !This->attached())
    {
      for (RculPWMGen **p = &first; *p != NULL; p = &((*p)->next))
      {
        if (*p == This)
        {
          *p = This->next;
          This->next = NULL;
          delete(This);
          ObjectCreated.Dynamically--;
          ObjectCreated.Total--;
          Ret = 1;
          break;
        }
      }
    }
  }
  return Ret;
}

RculPWMGen *RculPWMGen::RculPWMGenById(uint8_t ObjIdx)
{
  if (ObjIdx < ObjectCreated.Total)
  {
    int8_t Idx = ObjectCreated.Total - ObjIdx - 1;
    if (Idx >= 0)
    {
      RculPWMGen *p = first;
      for (uint8_t i = 0; (i < Idx) && p; i++) p = p->next;
      return p;
    }
  }
  return NULL;
}

int8_t RculPWMGen::getIdByPin(uint8_t Pin)
{
  int8_t Idx = 0;
  int8_t Id  = -1;

  for (RculPWMGen *p = first; p != NULL; p = p->next)
  {
    Idx++;
    if (p->pin == Pin)
    {
      Id = ObjectCreated.Total - Idx;
      break;
    }
  }
  return Id;
}

uint8_t RculPWMGen::attach(uint8_t pinArg, uint8_t Inverted /*= 0*/)
{
  pin      = pinArg;
  angle    = RCUL_PWM_GEN_NO_ANGLE;
  min_us   = RCUL_PWM_GEN_DEFAULT_MIN_US;
  max_us   = RCUL_PWM_GEN_DEFAULT_MAX_US;
  pulse_us = RCUL_PWM_GEN_DEFAULT_US;
  Bool.Inverted = Inverted ? 1 : 0;

  pinMode(pin, OUTPUT);
  writePin(pin, inactiveLevel());
  return 1;
}

void RculPWMGen::detach()
{
  if (pin != RCUL_PWM_GEN_NOT_ATTACHED)
  {
    writePin(pin, inactiveLevel());
  }
  pin = RCUL_PWM_GEN_NOT_ATTACHED;
}

void RculPWMGen::write(int angleArg)
{
  if (angleArg < 0)   angleArg = 0;
  if (angleArg > 180) angleArg = 180;

  angle = (uint8_t)angleArg;
  pulse_us = (uint16_t)map(angleArg, 0, 180, min_us, max_us);
}

void RculPWMGen::write_us(uint16_t PulseWidth_us)
{
  if (PulseWidth_us < min_us) PulseWidth_us = min_us;
  if (PulseWidth_us > max_us) PulseWidth_us = max_us;

  pulse_us = PulseWidth_us;
  angle = (uint8_t)map(PulseWidth_us, min_us, max_us, 0, 180);
}

uint8_t RculPWMGen::read()
{
  return angle;
}

uint16_t RculPWMGen::read_us()
{
  return pulse_us;
}

uint8_t RculPWMGen::attached()
{
  return (pin != RCUL_PWM_GEN_NOT_ATTACHED);
}

/* Begin of Rcul support */
uint8_t RculPWMGen::RculIsSynchro(uint8_t ClientIdx /*= RCUL_DEFAULT_CLIENT_IDX*/)
{
  (void)ClientIdx;
  return refresh();
}

void RculPWMGen::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /*= RCUL_NO_CH*/)
{
  (void)Ch;
  write_us(Width_us);
}

uint16_t RculPWMGen::RculGetWidth_us(uint8_t Ch)
{
  (void)Ch;
  return read_us();
}
/* End of Rcul support */

uint8_t RculPWMGen::refresh(uint8_t force /*= 0*/)
{
  static uint32_t lastRefresh = 0;
  uint32_t now = micros();

  if (!force)
  {
    if ((uint32_t)(now - lastRefresh) < 20000UL) return 0;
  }

  lastRefresh = now;

  uint8_t count = 0;
  for (RculPWMGen *p = first; p != NULL; p = p->next)
  {
    if (p->attached() && (count < RCUL_PWM_GEN_INSTANCE_MAX_NB)) count++;
  }
  if (count == 0) return 1;

  RculPWMGen *s[RCUL_PWM_GEN_INSTANCE_MAX_NB];
  uint8_t i = 0;
  for (RculPWMGen *p = first; p != NULL; p = p->next)
  {
    if (p->attached() && (i < RCUL_PWM_GEN_INSTANCE_MAX_NB)) s[i++] = p;
  }

  // Tri par largeur d'impulsion croissante.
  for (;;)
  {
    uint8_t moved = 0;
    for (i = 1; i < count; i++)
    {
      if (s[i]->pulse_us < s[i - 1]->pulse_us)
      {
        RculPWMGen *t = s[i];
        s[i] = s[i - 1];
        s[i - 1] = t;
        moved = 1;
      }
    }
    if (!moved) break;
  }

  // Debut des impulsions: toutes les sorties actives quasiment en meme temps.
  noInterrupts();
  for (i = 0; i < count; i++) s[i]->writePin(s[i]->pin, s[i]->activeLevel());
  interrupts();

  uint32_t start = micros();

  for (i = 0; i < count; i++)
  {
    uint16_t target_us = s[i]->pulse_us;

    while ((uint32_t)(micros() - start) < target_us)
    {
      // Attente courte. Les interruptions restent autorisees pour limiter l'impact
      // sur ESP32/WiFi/Teensy. La precision depend donc de la charge CPU.
      yield();
    }

    noInterrupts();
    s[i]->writePin(s[i]->pin, s[i]->inactiveLevel());

    // Coupe egalement les sorties qui ont exactement la meme largeur ou presque.
    while (((i + 1) < count) && ((int32_t)s[i + 1]->pulse_us - (int32_t)target_us <= 2))
    {
      i++;
      s[i]->writePin(s[i]->pin, s[i]->inactiveLevel());
    }
    interrupts();
  }

  return 1;
}
