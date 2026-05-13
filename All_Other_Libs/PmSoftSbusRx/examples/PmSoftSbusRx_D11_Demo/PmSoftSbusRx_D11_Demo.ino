#include <PmSoftSbusRx.h>

void setup()
{
  Serial.begin(115200);

  // D11, SBUS inversé, réglage trouvé comme le plus exploitable dans les tests.
  PmSoftSbusRx.begin(11, 1, -4);

  // v2.7: accepte la trame si au moins 6 voies parmi CH1..CH8 sont plausibles.
  // Les voies douteuses gardent leur ancienne valeur au lieu de faire rejeter
  // toute la trame. Augmente à 7 ou 8 si trop d'erreurs, baisse à 5 si trop lent.
  PmSoftSbusRx.setMinValidChannels(5);

  // 1 = accepte les bords SBUS 172 et 1811.
  // 0 = plus strict: 180..1803.
  PmSoftSbusRx.setAcceptBorderChannels(0);

  Serial.println(F("PmSoftSbusRx D11 RCUL demo v2.7"));
}

void loop()
{
  static uint32_t lastPrintMs = 0;

  PmSoftSbusRx.process();

  if(millis() - lastPrintMs >= 250)
  {
    lastPrintMs = millis();

    Serial.print(F("edge="));  Serial.print(PmSoftSbusRx.edgeCount());
    Serial.print(F(" valid=")); Serial.print(PmSoftSbusRx.validCount());
    Serial.print(F(" rej="));   Serial.print(PmSoftSbusRx.rejectCount());
    Serial.print(F(" FS="));    Serial.print(PmSoftSbusRx.failsafe());
    Serial.print(F(" | "));

    for(uint8_t ch = 1; ch <= 8; ch++)
    {
      Serial.print(F("C")); Serial.print(ch);
      Serial.print('='); Serial.print(PmSoftSbusRx.raw(ch));
      Serial.print('/'); Serial.print(PmSoftSbusRx.width_us(ch));
      Serial.print(F(" U")); Serial.print(PmSoftSbusRx.channelUpdateCount(ch));
      Serial.print(F(" R")); Serial.print(PmSoftSbusRx.channelRejectCount(ch));
      Serial.print(' ');
    }
    Serial.println();
    PmSoftSbusRx.printFrameHex(Serial);
  }
}
