#include <Rcul.h>
#include <RculPWMRead.h>

#define BROCHE_VOIE1  2

RculPWMRead ImpulsionVoie1;


void setup()
{
  Serial.begin(115200);
  while(!Serial);
  ImpulsionVoie1.attach(BROCHE_VOIE1);
  Serial.println("Ready ...");
}

void loop()
{
  if(ImpulsionVoie1.available())
  {
    Serial.print("Pulse=");Serial.println(ImpulsionVoie1.width_us());
  }
}