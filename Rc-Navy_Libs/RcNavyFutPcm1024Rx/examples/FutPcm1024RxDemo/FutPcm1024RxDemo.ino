#include <FutPcm1024Rx.h>

void setup()
{
  Serial.begin(115200);
  Serial.println(F("\nFUTABA PCM1024 RECEIVER DEMO"));
  Serial.println(F("\nImportant: PCM signal SHALL be connected to pin 8 of arduino UNO!"));
  FutPcm1024Rx_begin();
}

void loop()
{

  if(FutPcm1024Rx_available())
  {
    //Serial.print(F("CH1="));Serial.print(FutPcm1024Rx_channelRaw(1));Serial.print(F(" -> "));Serial.print(FutPcm1024Rx_channelWidthUs(1));Serial.println(F(" us"));
    for (uint8_t i=1;i<=8;i++)
    {
      Serial.print(F("CH"));Serial.print(i);Serial.print(F("="));Serial.print(FutPcm1024Rx_channelRaw(i));Serial.print(F(" -> "));Serial.print(FutPcm1024Rx_channelWidthUs(i));Serial.print(F(" us"));Serial.print("\t");
    }
    Serial.println();
  }
}
