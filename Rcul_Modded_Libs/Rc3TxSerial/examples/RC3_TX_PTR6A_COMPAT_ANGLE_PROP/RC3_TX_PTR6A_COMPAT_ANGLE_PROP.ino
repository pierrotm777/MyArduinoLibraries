/*
  RC3 TX compatibility test - ANGLE + PROP / 5 legacy nibbles
  --------------------------------------------------

  Purpose:
    Verify the ATTiny85 transparent bridge.

  IMPORTANT:
    This test MUST use Rc3TxSerial sendNibbleMsg().
    If sendMsg() is used instead, the ATTiny bridge intentionally ignores
    the native RC3 frame and will output only historical Idle "I".

  Payload:
    exact historical X-Any ANGLE+PROP packing, 20 useful bits:
      Byte 0: ANGLE[11:4]
      Byte 1: ANGLE[3:0] in bits 7..4, PROP[7:4] in bits 3..0
      Byte 2: PROP[3:0] in bits 7..4; low nibble unused

    For an easy end-to-end check:
      ANGLE = Counter & 0x0FFF
      PROP  = ~(ANGLE low byte)

    Total useful legacy payload = 5 nibbles.
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

static uint16_t Counter = 0;

void setup()
{
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500UL) delay(1);

  Serial.println();
  Serial.print(F("Rc3TxSerial V"));
  Serial.println(Rc3TxSerial::version());
  Serial.println(F("RC3 COMPAT ANGLE + PROP TEST"));
  Serial.println(F("sendNibbleMsg(Message, 5, 1)"));

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
    uint8_t Message[3];

    /*
      Exact historical ANGLE+PROP packing:

        Byte 0 : ANGLE[11:4]
        Byte 1 : ANGLE[3:0] in bits 7..4
                 PROP [7:4] in bits 3..0
        Byte 2 : PROP [3:0] in bits 7..4
                 low nibble = unused / zero

      Only the first FIVE nibbles are useful.  This intentionally exercises
      the odd-nibble compatibility path in both Rc3TxSerial and the ATTiny
      bridge.
    */
    const uint16_t Angle = (uint16_t)(Counter & 0x0FFFU);
    const uint8_t Prop = (uint8_t)~((uint8_t)Angle);

    Message[0] = (uint8_t)((Angle >> 4) & 0xFFU);
    Message[1] = (uint8_t)(((Angle & 0x000FU) << 4) |
                           ((Prop >> 4) & 0x0FU));
    Message[2] = (uint8_t)((Prop & 0x0FU) << 4);

    /*
      CRITICAL FOR THE ATTINY BRIDGE:
      5 = useful legacy nibbles (12-bit ANGLE + 8-bit PROP).
      1 = rebuild historical RcTxSerial checksum on the bridge.
    */
    if(Rc3Tx.sendNibbleMsg(Message, 5, 1))
    {
      Serial.print(F("TX COMPAT : ANGLE="));
      Serial.print(Angle);

      Serial.print(F(" (0x"));
      if(Angle < 0x100) Serial.print('0');
      if(Angle < 0x010) Serial.print('0');
      Serial.print(Angle, HEX);
      Serial.print(F(") PROP="));
      if(Prop < 0x10) Serial.print('0');
      Serial.println(Prop, HEX);

      Counter = (uint16_t)((Counter + 1U) & 0x0FFFU);
    }
  }

  Rc3TxSerial::process();
}
