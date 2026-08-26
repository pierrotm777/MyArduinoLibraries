#include <Wire.h>
#include <SoftRcPulseIn.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

#define PWM_RX_PIN  8
#define SDA_PIN     A4
#define SCL_PIN     A5

static SoftRcPulseIn ReceiverPwm;
static RculI2cPotTxClass RculI2cPotTx(DS3502);

void setup()
{
  Serial.begin(115200);

  ReceiverPwm.attach(PWM_RX_PIN, 700, 2300);

  if(!RculI2cPotTx.begin(
      SDA_PIN, SCL_PIN, 100000UL,
      0x28, 18, 1500))
  {
    Serial.println(F("DS3502 ERROR"));
    while(1);
  }

  RculI2cPotTx.setTableStorageEEPROM(0);

  if(!RculI2cPotTx.useStoredRculTable())
  {
    Serial.println(F("No stored table: embedded default selected"));
  }

  Serial.println(F("Commands:"));
  Serial.println(F("  c : learn table from receiver PWM"));
  Serial.println(F("  d : display active table"));
  Serial.println(F("  r : restore embedded default"));
  Serial.println(F("  e : erase stored table"));
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
      Serial.println(F("Embedded default table selected"));
    }
    else if(Cmd == 'e')
    {
      RculI2cPotTx.eraseStoredRculTable();
      Serial.println(F("Stored table erased"));
    }
  }

  if(RculI2cPotTx.isRculTableCalibrationActive())
  {
    /*
      Do not run RcTxSerial while learning: calibration owns the wiper.
    */
    RculI2cPotTx.processRculTableCalibration();
    return;
  }

  RculI2cPotTx.process();

  /*
    Normal RcTxSerial processing goes here.
  */
}
