/*
  RC3 TX compatibility test - SW8 / 2 legacy nibbles
  --------------------------------------------------

  Purpose:
    Verify the ATTiny85 transparent bridge.

  IMPORTANT:
    This test MUST use Rc3TxSerial sendNibbleMsg().
    If sendMsg() is used instead, the ATTiny bridge intentionally ignores
    the native RC3 frame and will output only historical Idle "I".

  Payload:
    one byte counter, treated as 2 legacy nibbles.
*/

#include <Arduino.h>
#include <Wire.h>

#define RCUL_I2C_POT_DEVICE MCP4661
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <Rc3TxSerial.h>

#define SDA_PIN             12
#define SCL_PIN             13
#define GDO0_PIN             5
#define DIGIPOT_ADDRESS     0x2F
#define I2C_FREQUENCY       RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY

static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A,
    GDO0_PIN,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_1
);

static Rc3TxSerial Rc3Tx(
    &RculI2cPotTx,
    RC3_TX_SERIAL_REPEAT2,  // 3 presentations / trit
    8,
    8
);

static uint8_t Counter = 0;

void setup()
{
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500UL) delay(1);

  Serial.println();
  Serial.print(F("Rc3TxSerial V"));
  Serial.println(Rc3TxSerial::version());
  Serial.println(F("RC3 COMPAT SW8 TEST"));
  Serial.println(F("sendNibbleMsg(Message, 2, 1)"));

  if(!RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500))
  {
    Serial.println(F("RculI2cPotTx begin ERROR"));
    while(1);
  }

  RculI2cPotTx.setTableStoragePreferences("RculPotM0");
  RculI2cPotTx.useStoredRculTable();
  RculI2cPotTx.printInfo(Serial);
}

void loop()
{
  RculI2cPotTx.process();

  if(Rc3Tx.isReadyForTx())
  {
    uint8_t Message[1];
    Message[0] = Counter;

    /*
      CRITICAL FOR THE ATTINY BRIDGE:
      2 = useful legacy nibble count for one complete byte.
      1 = rebuild historical RcTxSerial checksum on the bridge.
    */
    if(Rc3Tx.sendNibbleMsg(Message, 2, 1))
    {
      Serial.print(F("TX COMPAT : "));
      if(Counter < 0x10) Serial.print('0');
      Serial.println(Counter, HEX);

      Counter++;
    }
  }

  Rc3TxSerial::process();
}
