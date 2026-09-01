#include <Arduino.h>
#include <Rcul.h>
#include <TinyPinChange.h>
#include <SoftRcPulseIn.h>
#include <Rc3RxSerial.h>

#define PWM_IN_PIN 2

static SoftRcPulseIn PwmIn;
static Rc3RxSerial MyRc3RxSerial(
    &PwmIn,
    RC3_RX_SERIAL_REPEAT2
);

void setup()
{
  Serial.begin(115200);

  if(PwmIn.attach(PWM_IN_PIN, 1200, 1900) < 0)
    while(1);

  /* Profil mesure sur la voie proportionnelle PTR-6A du test. */
  MyRc3RxSerial.setPulseWindows(
      1280, 1450,
      1540, 1680,
      1740, 1860);
}

void loop()
{
  char Message[RC3_RX_SERIAL_MAX_PAYLOAD];
  const uint8_t Len = MyRc3RxSerial.msgAvailable(Message, sizeof(Message));

  if(Len)
  {
    Serial.print(F("RX len="));
    Serial.print(Len);
    Serial.print(F(" :"));

    for(uint8_t i = 0; i < Len; ++i)
    {
      Serial.print(' ');
      if((uint8_t)Message[i] < 0x10) Serial.print('0');
      Serial.print((uint8_t)Message[i], HEX);
    }

    Serial.print(F(" good="));
    Serial.print(MyRc3RxSerial.goodFrames());
    Serial.print(F(" crcErr="));
    Serial.print(MyRc3RxSerial.crcErrors());
    Serial.print(F(" symErr="));
    Serial.print(MyRc3RxSerial.symbolErrors());
    Serial.print(F(" majFix="));
    Serial.println(MyRc3RxSerial.majorityCorrected());
  }
}
