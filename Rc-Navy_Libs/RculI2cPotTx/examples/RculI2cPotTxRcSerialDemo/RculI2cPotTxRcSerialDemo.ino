#include <Arduino.h>
#include <Wire.h>
// Select the I2C digital potentiometer used by this example.
// Choose MCP4561 or DS3502.
#define RCUL_I2C_POT_DEVICE DS3502

// Keep the original global RculI2cPotTx object name while selecting its driver.
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
static RculI2cPotTxClass RculI2cPotTx(RCUL_I2C_POT_DEVICE);
#include <RcTxSerial.h>

// ============================================================================
// I2C BUS
// ============================================================================
//Uno
//#define SDA_PIN A4
//#define SCL_PIN A5
//ESP32 C3
#define SDA_PIN 5
#define SCL_PIN 6

/* Possible I2C speeds:
   RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY = 100000UL (default)
   RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY    = 400000UL

   ESP32:
   - SDA_PIN and SCL_PIN select the I2C pins.

   AVR:
   - the hardware SDA/SCL pins are fixed by the selected board;
   - SDA_PIN and SCL_PIN are accepted only to keep the same source code. */
#define I2C_FREQUENCY RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY
// #define I2C_FREQUENCY RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY

// ============================================================================
// DIGITAL POTENTIOMETER
// ============================================================================
/* RCUL_I2C_POT_TX_AUTO_ADDRESS selects the correct default automatically:
     MCP4561 : 0x2C
     DS3502  : 0x28 when A1=A0=0 (0x28..0x2B possible).
   A different valid address can be forced if required. */
#define POT_I2C_ADDRESS RCUL_I2C_POT_TX_AUTO_ADDRESS
// #define POT_I2C_ADDRESS 0x2D

// ============================================================================
// RCUL SERIAL TRANSMITTER
// ============================================================================
#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

void setup()
{
  Serial.begin(115200);
  delay(300);

  /* Optional digital-potentiometer wiper range.

     If setWiperRange() is not called, the library selects the default automatically: MCP4561=4..252, DS3502=2..125.

     Possible examples:
       RculI2cPotTx.setWiperRange(0, RculI2cPotTx.getMaxWiper()); // full selected-device range
       RculI2cPotTx.setWiperRange(10, 245);  // reduced custom range

     The call can be made before or after begin(). Calling it before begin()
     avoids an additional initial wiper write.

     Channel reversal must be configured in the radio, not in this library. */
  // RculI2cPotTx.setWiperRange(10, 245);

  /* begin() initializes Wire itself.

     Parameters:
     - SDA pin
     - SCL pin
     - I2C speed: 100 kHz by default, or 400 kHz
     - default or forced digital-potentiometer I2C address
     - RCUL synchronization period: 14 ms by default
     - initial RC width: 1500 us
  */
  if(!RculI2cPotTx.begin(SDA_PIN,
                         SCL_PIN,
                         I2C_FREQUENCY,
                         POT_I2C_ADDRESS,
                         RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
                         1500))
  {
    Serial.println(F("Digital potentiometer not detected or initial write failed"));
    return;
  }


  /* Diagnostic summary. It also checks that the selected address replies. */
  RculI2cPotTx.printInfo(Serial);
}

void loop()
{
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    /* Fixed test message: eight contacts with SW8 active (0x80).
       Replace this block with the actual message builder when required. */
    uint8_t Msg = 0x80;
    MyRcTxSerial.sendNibbleMsg(&Msg, 2, 1);
  }

  RcTxSerial::process();

  /* Optional debug helpers. Do not print continuously in normal operation:

     RculI2cPotTx.getWiper();       // Current dynamic wiper position
     RculI2cPotTx.getMaxWiper();    // 256 for MCP4561, 127 for DS3502
     RculI2cPotTx.getI2cAddress();  // Effective default or forced address
     RculI2cPotTx.isConnected();    // Tests whether the address replies
     RculI2cPotTx.printInfo(Serial); // Complete diagnostic summary
  */
}
