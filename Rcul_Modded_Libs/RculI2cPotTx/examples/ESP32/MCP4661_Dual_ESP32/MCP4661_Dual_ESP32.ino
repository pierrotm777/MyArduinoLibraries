#include <Wire.h>
#include <RcTxSerial.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

/*
 * ESP32 / ESP32-C3
 *
 * Un MCP4661, deux sorties RCUL indépendantes.
 * Cet exemple minimal utilise la synchro interne de RculI2cPotTx.
 *
 * Adapter les GPIO ADC si nécessaire pour votre carte.
 */

#define SDA_PIN           12
#define SCL_PIN           13
#define INPUT0_PIN        0
#define INPUT1_PIN        1

#define MCP4661_ADDRESS   0x2F
#define RCUL_REPEAT       5
#define RCUL_FIFO_SIZE    8

static RculI2cPotTxClass Pot0(
    MCP4661,
    RCUL_I2C_POT_SYNCRO_INTERNAL,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_0);

static RculI2cPotTxClass Pot1(
    MCP4661,
    RCUL_I2C_POT_SYNCRO_INTERNAL,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_1);

static RcTxSerial Tx0(&Pot0, RCUL_REPEAT, RCUL_FIFO_SIZE, 5);
static RcTxSerial Tx1(&Pot1, RCUL_REPEAT, RCUL_FIFO_SIZE, 6);

void setup()
{
  Serial.begin(115200);

  /* Attendre le port USB, mais pas indéfiniment */
  uint32_t StartMs = millis();
  while(!Serial && (millis() - StartMs < 3000))
  {
    delay(10);
  }

  delay(100);
  
  Wire.begin(
      SDA_PIN,
      SCL_PIN,
      RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY);

  analogReadResolution(12);

  if(!Pot0.begin(&Wire, MCP4661_ADDRESS, 18, 1500) ||
     !Pot1.begin(&Wire, MCP4661_ADDRESS, 18, 1500))
  {
    Serial.println(F("MCP4661 non detecte"));
    while(1);
  }

  Pot0.printInfo(Serial);
  Pot1.printInfo(Serial);
}

void loop()
{
  const uint8_t Value0 =
      (uint8_t)(((uint32_t)analogRead(INPUT0_PIN) * 255UL) / 4095UL);

  const uint8_t Value1 =
      (uint8_t)(((uint32_t)analogRead(INPUT1_PIN) * 255UL) / 4095UL);

  Pot0.process();
  Pot1.process();

  if(Tx0.isReadyForTx())
  {
    uint8_t Msg = Value0;
    Tx0.sendNibbleMsg(&Msg, 2, 1);
  }

  if(Tx1.isReadyForTx())
  {
    uint8_t Msg = Value1;
    Tx1.sendNibbleMsg(&Msg, 2, 1);
  }

  RcTxSerial::process();
}
