#include <SoftSBusRx.h>

#define SBUS_PIN 10   // Note: the pin SHALL support pin change interrupt

#if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1) // if SOFT_SBUS_RX_SINGLE_CHANNEL is set to 1 in SoftSBusRx.h
#define SBUS_CH  3    // <- Choose here the Channel you want to track (From 1 to 16)
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println(F("SoftSBusRx demo"));
  SoftSBusRx.begin(SBUS_PIN);  
#if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1)
  SoftSBusRx.trackChId(SBUS_CH); // Needed only when TRACK_A_SINGLE_CHANNEL is set to 1 in SoftSBusRx.h (to reduce RAM consumption)
  Serial.print(F("Note: SOFT_SBUS_RX_SINGLE_CHANNEL is defined in SoftSBusRx.h -> tracking only CH"));Serial.println(SBUS_CH);
#else
  Serial.println(F("Note: SOFT_SBUS_RX_SINGLE_CHANNEL is NOT defined in SoftSBusRx.h -> tracking all Channels"));
#endif
}

void loop()
{
  uint8_t DeltaMs;
  if(SoftSBusRx.isSynchro())
  {
#if (SOFT_SBUS_RX_SINGLE_CHANNEL == 1)
    DeltaMs = millis()%100;
    if(DeltaMs < 10) Serial.print(F("0"));
    Serial.print(DeltaMs);Serial.print(F( ":Ch"));Serial.print(SBUS_CH);Serial.print(F("="));Serial.println(SoftSBusRx.width_us(SBUS_CH)); 
#else
    for (int ChId=1; ChId <= 16; ++ChId)
    {
      Serial.print(SoftSBusRx.width_us(ChId)); 
      Serial.print(" ");
    }
    Serial.print(F( "Ch17="));    Serial.print(SoftSBusRx.flags(SBUS_RX_CH17)); /* Digital Channel#17 */
    Serial.print(F(" Ch18="));    Serial.print(SoftSBusRx.flags(SBUS_RX_CH18)); /* Digital Channel#18 */
    Serial.print(F(" FrmLost=")); Serial.print(SoftSBusRx.flags(SBUS_RX_FRAME_LOST)); /* Switch off the Transmitter to check this */
    Serial.print(F(" FailSafe="));Serial.print(SoftSBusRx.flags(SBUS_RX_FAILSAFE));   /* Switch off the Transmitter to check this */
    Serial.println();
    delay(200); // To avoid flooding serial
#endif
  }
}

