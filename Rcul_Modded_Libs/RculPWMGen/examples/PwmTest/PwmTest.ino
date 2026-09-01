#include <RculPWMGen.h>

#define PWM_OUT_PIN 5

RculPWMGen PwmOut;

static const uint16_t WidthUs[] = {1000, 1500, 2000};
static uint8_t Idx = 0;
static uint32_t LastChangeMs = 0;

void setup()
{
  Serial.begin(115200);

  PwmOut.attach(PWM_OUT_PIN);
  PwmOut.write_us(WidthUs[0]);

  Serial.println("RculPWMGen test: 1000 / 1500 / 2000 us");
}

void loop()
{
  /*
    IMPORTANT:
    In normal standalone use call refresh().
    If another library uses PwmOut.RculIsSynchro(), do not also call refresh()
    from the same loop because RculIsSynchro() already performs the refresh.
  */
  RculPWMGen::refresh();

  if((uint32_t)(millis() - LastChangeMs) >= 2000UL)
  {
    LastChangeMs = millis();
    Idx++;
    if(Idx >= 3) Idx = 0;

    PwmOut.write_us(WidthUs[Idx]);

    Serial.print("PWM = ");
    Serial.print(WidthUs[Idx]);
    Serial.println(" us");
  }
}
