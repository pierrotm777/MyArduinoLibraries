#include <RculPWMGen.h>

RculPWMGen *RculPWMGen::first = 0;

typedef struct
{
  uint8_t
    Total       : 4,
    Dynamically : 4;
} RculPWMGenObjectCreatedSt_t;

static RculPWMGenObjectCreatedSt_t ObjectCreated = {0, 0};

#define RCUL_PWM_GEN_NO_ANGLE      0xff
#define RCUL_PWM_GEN_NOT_ATTACHED  0xff

RculPWMGen::RculPWMGen()
{
  pin            = RCUL_PWM_GEN_NOT_ATTACHED;
  angle          = RCUL_PWM_GEN_NO_ANGLE;
  pulse_us       = 0;
  min_us         = RCUL_PWM_GEN_DEFAULT_MIN_US;
  max_us         = RCUL_PWM_GEN_DEFAULT_MAX_US;
  Bool.ItMasked  = 0;
  Bool.Inverted  = 0;
  Bool.Reserved  = 0;

  next  = first;
  first = this;

  if(ObjectCreated.Total < RCUL_PWM_GEN_INSTANCE_MAX_NB)
  {
    ObjectCreated.Total++;
  }
}

void RculPWMGen::setMinimumPulse(uint16_t t)
{
  min_us = t;

  if((pulse_us != 0) && (pulse_us < min_us))
  {
    pulse_us = min_us;
  }
}

void RculPWMGen::setMaximumPulse(uint16_t t)
{
  max_us = t;

  if((pulse_us != 0) && (pulse_us > max_us))
  {
    pulse_us = max_us;
  }
}

int8_t RculPWMGen::createInstance(void)
{
  int8_t Ret = -1;

  if(ObjectCreated.Total < RCUL_PWM_GEN_INSTANCE_MAX_NB)
  {
    RculPWMGen *p = new RculPWMGen;

    if(p)
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

  if((ObjIdx >= (ObjectCreated.Total - ObjectCreated.Dynamically)) &&
     (ObjIdx < ObjectCreated.Total))
  {
    RculPWMGen *This = RculPWMGenById(ObjIdx);

    if(This && !This->attached())
    {
      for(RculPWMGen **p = &first; *p != 0; p = &((*p)->next))
      {
        if(*p == This)
        {
          *p = This->next;
          This->next = 0;
          delete This;

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
  if(ObjIdx < ObjectCreated.Total)
  {
    int8_t Idx = (int8_t)ObjectCreated.Total - (int8_t)ObjIdx - 1;
    RculPWMGen *p = first;

    while((Idx > 0) && p)
    {
      p = p->next;
      Idx--;
    }

    return p;
  }

  return 0;
}

int8_t RculPWMGen::getIdByPin(uint8_t Pin)
{
  int8_t Idx = 0;

  for(RculPWMGen *p = first; p != 0; p = p->next)
  {
    Idx++;

    if(p->pin == Pin)
    {
      return (int8_t)ObjectCreated.Total - Idx;
    }
  }

  return -1;
}

uint8_t RculPWMGen::attach(uint8_t pinArg, uint8_t Inverted /* = 0 */)
{
  pin            = pinArg;
  angle          = RCUL_PWM_GEN_NO_ANGLE;
  pulse_us       = 0;
  min_us         = RCUL_PWM_GEN_DEFAULT_MIN_US;
  max_us         = RCUL_PWM_GEN_DEFAULT_MAX_US;
  Bool.ItMasked  = 0;
  Bool.Inverted  = Inverted ? 1 : 0;

  pinMode(pin, OUTPUT);
  writePin(pin, inactiveLevel());

  return 1;
}

void RculPWMGen::detach()
{
  if(pin != RCUL_PWM_GEN_NOT_ATTACHED)
  {
    writePin(pin, inactiveLevel());
  }

  pin = RCUL_PWM_GEN_NOT_ATTACHED;
}

void RculPWMGen::write(int angleArg)
{
  if(angleArg < 0)   angleArg = 0;
  if(angleArg > 180) angleArg = 180;

  angle = (uint8_t)angleArg;
  pulse_us = (uint16_t)map((long)angleArg,
                           0L,
                           180L,
                           (long)min_us,
                           (long)max_us);
}

void RculPWMGen::write_us(uint16_t PulseWidth_us)
{
  if(PulseWidth_us < min_us) PulseWidth_us = min_us;
  if(PulseWidth_us > max_us) PulseWidth_us = max_us;

  pulse_us = PulseWidth_us;
  angle = (uint8_t)map((long)PulseWidth_us,
                       (long)min_us,
                       (long)max_us,
                       0L,
                       180L);
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
uint8_t RculPWMGen::RculIsSynchro(uint8_t ClientIdx /* = RCUL_DEFAULT_CLIENT_IDX */)
{
  (void)ClientIdx;
  return refresh();
}

void RculPWMGen::RculSetWidth_us(uint16_t Width_us, uint8_t Ch /* = RCUL_NO_CH */)
{
  (void)Ch;
  write_us(Width_us);
}

uint16_t RculPWMGen::RculGetWidth_us(uint8_t Ch /* = RCUL_NO_CH */)
{
  (void)Ch;
  return read_us();
}
/* End of Rcul support */

uint8_t RculPWMGen::refresh(uint8_t force /* = 0 */)
{
  uint8_t count = 0;
  uint8_t i = 0;
  static uint32_t lastRefresh = 0;
  const uint32_t NowUs = micros();

  if(!force)
  {
    if((uint32_t)(NowUs - lastRefresh) < RCUL_PWM_GEN_REFRESH_PERIOD_US)
    {
      return 0;
    }
  }

  /* Same SoftRcPulseOut semantics: period is start-to-start. */
  lastRefresh = NowUs;

  /* Gather only attached instances which have received a valid command. */
  RculPWMGen *s[RCUL_PWM_GEN_INSTANCE_MAX_NB];

  for(RculPWMGen *p = first; p != 0; p = p->next)
  {
    if(p->attached() && (p->pulse_us != 0))
    {
      if(count < RCUL_PWM_GEN_INSTANCE_MAX_NB)
      {
        s[count++] = p;
      }
    }
  }

  if(count == 0)
  {
    return 1;
  }

  /* Bubble sort by pulse width, ascending, like SoftRcPulseOut. */
  for(;;)
  {
    uint8_t moved = 0;

    for(i = 1; i < count; i++)
    {
      if(s[i]->pulse_us < s[i - 1]->pulse_us)
      {
        RculPWMGen *t = s[i];
        s[i] = s[i - 1];
        s[i - 1] = t;
        moved = 1;
      }
    }

    if(!moved) break;
  }

  /*
    ItMasked has the same meaning as in SoftRcPulseOut:
    if one falling edge is very close to the previous one, interrupts remain
    masked between those two exact edges.
  */
  for(i = 0; i < count; i++)
  {
    s[i]->Bool.ItMasked = 0;
  }

  for(i = 1; i < count; i++)
  {
    if((uint16_t)(s[i]->pulse_us - s[i - 1]->pulse_us) <= RCUL_PWM_GEN_CLOSE_PULSE_US)
    {
      s[i]->Bool.ItMasked = 1;
    }
  }

  /* Start all pulses almost simultaneously, as SoftRcPulseOut does. */
  noInterrupts();
  for(i = 0; i < count; i++)
  {
    writePin(s[i]->pin, s[i]->activeLevel());
  }
  interrupts();

  const uint32_t StartUs = micros();

  /* Wait for each falling edge in ascending order. */
  for(i = 0; i < count; i++)
  {
    const uint16_t TargetUs = s[i]->pulse_us;
    const uint16_t GuardUs =
      (TargetUs > RCUL_PWM_GEN_EDGE_GUARD_US) ?
      (uint16_t)(TargetUs - RCUL_PWM_GEN_EDGE_GUARD_US) : 0;

    /*
      If this edge is not part of a close-edge cluster, interrupts are enabled
      during most of the pulse. No yield(): explicit task yielding close to the
      edge would unnecessarily increase jitter on ESP32.
    */
    if(!s[i]->Bool.ItMasked)
    {
      while((uint32_t)(micros() - StartUs) < GuardUs)
      {
        /* busy wait with interrupts enabled */
      }

      noInterrupts();
    }

    /* Final guard: edge timing cannot be delayed by an interrupt/task switch. */
    while((uint32_t)(micros() - StartUs) < TargetUs)
    {
      /* short busy wait with interrupts masked */
    }

    writePin(s[i]->pin, s[i]->inactiveLevel());

    /* Re-enable unless the next edge belongs to the same close-edge cluster. */
    if((i + 1) < count)
    {
      if(!s[i + 1]->Bool.ItMasked)
      {
        interrupts();
      }
    }
    else
    {
      interrupts();
    }
  }

  return 1;
}
