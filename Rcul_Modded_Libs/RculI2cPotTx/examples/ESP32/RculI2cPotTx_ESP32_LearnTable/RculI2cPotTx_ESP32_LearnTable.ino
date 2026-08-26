#include <Wire.h>
#include <RculPWMRead.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

#define PWM_RX_PIN  4
#define SDA_PIN      5
#define SCL_PIN      6

static RculPWMRead ReceiverPwm;
static RculI2cPotTxClass RculI2cPotTx(DS3502);

void setup()
{
  Serial.begin(115200);
  delay(300);

  if(ReceiverPwm.attach(PWM_RX_PIN, 700, 2300) < 0)
  {
    Serial.println("PWM input ERROR");
    while(1);
  }

  if(!RculI2cPotTx.begin(
      SDA_PIN, SCL_PIN, 100000UL,
      0x28, 18, 1500))
  {
    Serial.println("DS3502 ERROR");
    while(1);
  }

  RculI2cPotTx.setTableStoragePreferences("RculPotTx");

  if(!RculI2cPotTx.useStoredRculTable())
  {
    Serial.println("No stored table: embedded default selected");
  }

  Serial.println("Commands: c=learn d=display r=default e=erase");
}

void loop()
{
  if(Serial.available())
  {
    const char Cmd = Serial.read();

    if(Cmd == 'c')
    {
      RculI2cPotTx.startRculTableCalibration(
          ReceiverPwm,
          &Serial,
          6,
          2,
          4);
    }
    else if(Cmd == 'd')
    {
      RculI2cPotTx.displayRculTable(Serial);
    }
    else if(Cmd == 'r')
    {
      RculI2cPotTx.useDefaultRculTable();
      Serial.println("Embedded default table selected");
    }
    else if(Cmd == 'e')
    {
      RculI2cPotTx.eraseStoredRculTable();
      Serial.println("Stored table erased");
    }
  }

  if(RculI2cPotTx.isRculTableCalibrationActive())
  {
    RculI2cPotTx.processRculTableCalibration();
    return;
  }

  RculI2cPotTx.process();

  /*
    Normal RcTxSerial processing goes here.
  */
}
