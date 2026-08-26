#include <Arduino.h>
#include <TinyWireM.h>

#define RCUL_I2C_POT_DEVICE DS3502
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

static RculI2cPotTxClass RculI2cPotTx(RCUL_I2C_POT_DEVICE);

/*
  Validated ATtiny85 I2C base:

    SDA = PB0 / Arduino pin 0 / physical pin 5
    SCL = PB2 / Arduino pin 2 / physical pin 7

  DS3502:
    address 0x28
    entirely managed by RculI2cPotTx

  PCF8574A:
    base address 0x38
    write 0xFF once to release all quasi-bidirectional pins
    read one byte with TinyWireM.requestFrom()

  Validated TinyWireM read convention:
    requestFrom(...) == 0 : success
    available() >= 1      : one byte can be read

  The raw result of endTransmission() used for the PCF input-release write
  is deliberately not used as a presence test. Presence is confirmed by
  the following one-byte read.
*/

#define SDA_PIN           0
#define SCL_PIN           2
#define DIGIPOT_ADDRESS   0x28
#define I2C_FREQUENCY     RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY

#define RCUL_REPEAT       5
#define RCUL_CHANNEL      8
#define SWITCH_DEBOUNCE_MS 20UL

#define PROP_PIN          9      // A3 / ADC3 / PB3 / physical pin 2
#define ANGLE_PIN         8      // A2 / ADC2 / PB4 / physical pin 3
#define ADC_MAX           1023UL

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint16_t Angle = 0;  // 0..4095
static uint8_t Prop = 0;    // 0..255
static bool HardwareReady = false;

static void updateInputs(void)
{
  const uint16_t RawAngle = (uint16_t)analogRead(ANGLE_PIN);
  const uint16_t RawProp = (uint16_t)analogRead(PROP_PIN);

  Angle = (uint16_t)(((uint32_t)RawAngle * 4095UL +
                      (ADC_MAX / 2UL)) /
                     ADC_MAX);

  Prop = (uint8_t)(((uint32_t)RawProp * 255UL +
                    (ADC_MAX / 2UL)) /
                   ADC_MAX);
}

void setup()
{
  pinMode(ANGLE_PIN, INPUT);
  pinMode(PROP_PIN, INPUT);
  digitalWrite(ANGLE_PIN, LOW);
  digitalWrite(PROP_PIN, LOW);

  TinyWireM.begin();

  HardwareReady = RculI2cPotTx.begin(
      SDA_PIN, SCL_PIN, I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  updateInputs();
}

void loop()
{
  if(!HardwareReady)
  {
    return;
  }

  updateInputs();
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    /*
      ANGLE+PROP payload, seven nibbles:
        Angle[11:8], Angle[7:4], Angle[3:0],
        Prop[7:4], Prop[3:0]

      The unused low nibble of Message[2] is cleared.
    */
    uint8_t Message[3];
    Message[0] = (uint8_t)(Angle >> 4);
    Message[1] = (uint8_t)((Angle << 4) | (Prop >> 4));
    Message[2] = (uint8_t)(Prop << 4);

    MyRcTxSerial.sendNibbleMsg(Message, 5, 1);
  }

  RcTxSerial::process();
}
