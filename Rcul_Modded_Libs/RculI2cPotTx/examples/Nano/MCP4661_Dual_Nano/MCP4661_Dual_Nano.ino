#include <Wire.h>
#include <RcTxSerial.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

/*
 * Nano / ATmega328P
 *
 * Un MCP4661, deux sorties RCUL indépendantes.
 * Cet exemple minimal utilise la synchro interne de RculI2cPotTx.
 *
 * MCP4661:
 *   VDD -> +5 V
 *   VSS -> GND
 *   SDA -> A4
 *   SCL -> A5
 *   A2/A1/A0 -> GND  => adresse 0x28
 *
 * A0 du Nano pilote les données envoyées par le pot 0.
 * A1 du Nano pilote les données envoyées par le pot 1.
 */

#define MCP4661_ADDRESS  0x28
#define RCUL_REPEAT      5
#define RCUL_FIFO_SIZE   8

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

  Wire.begin();
  Wire.setClock(RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY);

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
  const uint8_t Value0 = (uint8_t)(analogRead(A0) >> 2);
  const uint8_t Value1 = (uint8_t)(analogRead(A1) >> 2);

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
