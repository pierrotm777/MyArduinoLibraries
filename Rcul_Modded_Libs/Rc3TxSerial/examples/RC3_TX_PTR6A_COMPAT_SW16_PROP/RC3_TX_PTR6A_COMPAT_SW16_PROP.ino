/*
  RC3 TX compatibility test - SW16 + PROP / 6 legacy nibbles
  --------------------------------------------------

  Purpose:
    Verify the ATTiny85 transparent bridge.

  IMPORTANT:
    This test MUST use Rc3TxSerial sendNibbleMsg().
    If sendMsg() is used instead, the ATTiny bridge intentionally ignores
    the native RC3 frame and will output only historical Idle "I".

  Payload:
    historical X-Any SW16+PROP packing:
      Message[0] = PROP
      Message[1] = SW16 high byte
      Message[2] = SW16 low byte

    For an easy end-to-end check:
      SW16 = Counter
      PROP = ~(Counter low byte)

    Total useful legacy payload = 6 nibbles.
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
  Serial.println(F("RC3 COMPAT SW16 + PROP TEST"));
  Serial.println(F("sendNibbleMsg(Message, 6, 1)"));

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
      Historical X-Any packing when ANGLE=0 and PROP=1:
        byte 0 = PROP
        byte 1 = SW16 high byte
        byte 2 = SW16 low byte

      SW16 is the incrementing 16-bit counter.
      PROP is the complement of the counter low byte so both fields are
      easy to verify independently at the receiver.
    */
    const uint16_t Switch16 = Counter;
    const uint8_t Prop = (uint8_t)~((uint8_t)Counter);

    Message[0] = Prop;
    Message[1] = (uint8_t)(Switch16 >> 8);
    Message[2] = (uint8_t)(Switch16 & 0xFF);

    /*
      CRITICAL FOR THE ATTINY BRIDGE:
      6 = useful legacy nibbles (PROP byte + SW16 word).
      1 = rebuild historical RcTxSerial checksum on the bridge.

      This is the maximum COMPAT payload:
        META + 3 data bytes = 4 RC3 bytes.
    */
    if(Rc3Tx.sendNibbleMsg(Message, 6, 1))
    {
      Serial.print(F("TX COMPAT : PROP="));
      if(Prop < 0x10) Serial.print('0');
      Serial.print(Prop, HEX);

      Serial.print(F(" SW16="));
      if(Switch16 < 0x1000) Serial.print('0');
      if(Switch16 < 0x0100) Serial.print('0');
      if(Switch16 < 0x0010) Serial.print('0');
      Serial.println(Switch16, HEX);

      Counter++;
    }
  }

  Rc3TxSerial::process();
}
