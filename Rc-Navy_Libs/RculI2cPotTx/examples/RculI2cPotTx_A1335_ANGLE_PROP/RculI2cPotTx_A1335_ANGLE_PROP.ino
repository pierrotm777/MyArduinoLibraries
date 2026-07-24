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
// I2C pins
// ============================================================================
//Uno
//#define SDA_PIN A4
//#define SCL_PIN A5
//ESP32 C3
#define SDA_PIN 5
#define SCL_PIN 6

#define I2C_FREQUENCY RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY
// #define I2C_FREQUENCY RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY

#define DIGIPOT_ADDRESS RCUL_I2C_POT_TX_AUTO_ADDRESS
// #define DIGIPOT_ADDRESS 0x2D

// ============================================================================
// Analog proportional input
// ============================================================================
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  #define PROP_PIN      0
  #define PROP_ADC_MAX 4095UL
  #define ANGLE_PIN     1
  #define ANGLE_ADC_MAX 4095UL
#else
  #define PROP_PIN     A0
  #define PROP_ADC_MAX 1023UL
  #define ANGLE_PIN    A1
  #define ANGLE_ADC_MAX 1023UL
#endif

/* Potentiometer wiring:
     one end -> board VCC
     wiper   -> PROP_PIN
     other end -> GND

   PROP is encoded on 8 bits: ADC minimum -> 0, ADC maximum -> 255.
*/

// ============================================================================
// A1335 magnetic angle sensor
// ============================================================================
/* A1335 I2C/Analog modes:
     ISEL must be tied low.

   Default 7-bit I2C addresses depend on SA1 and SA0:
     SA1 SA0 = 00 -> 0x0C
     SA1 SA0 = 01 -> 0x0D
     SA1 SA0 = 10 -> 0x0E
     SA1 SA0 = 11 -> 0x0F

   The current angle is read from register 0x20:0x21.
   ANGLE occupies bits 11..0 and directly provides a value from 0 to 4095.
*/
#define I2C                 0
#define ANA                 1
#define ANGLE_USE           ANA
#define A1335_ADDRESS       0x0C
#define A1335_ANGLE_REG     0x20
#define A1335_ANGLE_MASK    0x0FFF

// RCUL payload sizes and resulting MLEN values:
//   SW8          : 2 payload nibbles -> MLEN=4
//   SW8+PROP     : 4 payload nibbles -> MLEN=6
//   SW16         : 4 payload nibbles -> MLEN=6
//   ANGLE+PROP   : 5 payload nibbles -> MLEN=7
//   SW16+PROP    : 6 payload nibbles -> MLEN=8
//
// ANGLE+PROP payload order (20 bits):
//   ANGLE[11:0], then PROP[7:0]
#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint16_t Angle = 0;
static uint8_t Prop = 0;
static bool A1335Ok = false;
static uint32_t LastPrintMs = 0;

static bool a1335ReadAngle(uint16_t &Value)
{
  Wire.beginTransmission(A1335_ADDRESS);
  Wire.write(A1335_ANGLE_REG);

  // Repeated START is required before the read transaction.
  if(Wire.endTransmission(false) != 0)
  {
    return false;
  }

  const uint8_t Count = Wire.requestFrom((uint8_t)A1335_ADDRESS,
                                         (uint8_t)2);
  if(Count != 2)
  {
    while(Wire.available()) Wire.read();
    return false;
  }

  const uint8_t Msb = Wire.read();
  const uint8_t Lsb = Wire.read();
  const uint16_t RegisterValue = ((uint16_t)Msb << 8) | Lsb;

  Value = RegisterValue & A1335_ANGLE_MASK;
  return true;
}

static void updateInputs(void)
{
  uint16_t NewAngle;
#if (ANGLE_USE == I2C)
  if(a1335ReadAngle(NewAngle))
  {
    Angle = NewAngle;
    A1335Ok = true;
  }
  else
  {
    A1335Ok = false;
  }
#elif (ANGLE_USE == ANA)
  const uint32_t RawAngle = (uint32_t)analogRead(ANGLE_PIN);
  Angle = (uint16_t)((RawAngle * 4095UL + (ANGLE_ADC_MAX / 2UL)) /
                   ANGLE_ADC_MAX);
  A1335Ok = true;
#endif
  const uint32_t RawProp = (uint32_t)analogRead(PROP_PIN);
  Prop = (uint8_t)((RawProp * 255UL + (PROP_ADC_MAX / 2UL)) /
                   PROP_ADC_MAX);
}

static void buildAnglePropMessage(uint8_t *Message)
{
  /* Exact 20-bit packing used by RcTxSerial ANGLE+PROP mode:

       Byte 0: ANGLE[11:4]
       Byte 1: ANGLE[3:0] in bits 7..4, PROP[7:4] in bits 3..0
       Byte 2: PROP[3:0] in bits 7..4

     Only the first 5 nibbles are sent.
  */
  Message[0] = (uint8_t)((Angle >> 4) & 0xFF);
  Message[1] = (uint8_t)(((Angle & 0x000F) << 4) |
                         ((Prop >> 4) & 0x0F));
  Message[2] = (uint8_t)((Prop & 0x0F) << 4);
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println(F("RculI2cPotTx + A1335 - ANGLE+PROP"));

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  analogReadResolution(12);
#endif
  pinMode(PROP_PIN, INPUT);
  pinMode(ANGLE_PIN, INPUT);
  
  // Optional digital-potentiometer range. Defaults: MCP4561=4..252, DS3502=2..125.
  // RculI2cPotTx.setWiperRange(0, RculI2cPotTx.getMaxWiper()); // full range
  // RculI2cPotTx.setWiperRange(10, 245);  // reduced range

  if(!RculI2cPotTx.begin(SDA_PIN,
                         SCL_PIN,
                         I2C_FREQUENCY,
                         DIGIPOT_ADDRESS,
                         RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
                         1500))
  {
    Serial.println(F("Digital potentiometer not detected or initial write failed"));
    return;
  }

#if (ANGLE_USE == I2C)
  uint16_t InitialAngle;
  if(!a1335ReadAngle(InitialAngle))
  {
    Serial.println(F("A1335 not detected or angle read failed"));
  }
  else
  {
    Angle = InitialAngle;
    A1335Ok = true;
  }
#elif (ANGLE_USE == ANA)
  A1335Ok = true;
#endif

  updateInputs();

  RculI2cPotTx.printInfo(Serial);
  Serial.print(F("A1335 address: 0x"));
  Serial.println(A1335_ADDRESS, HEX);
  Serial.println(F("Payload: ANGLE[11:0] + PROP[7:0]"));
  Serial.println(F("5 payload nibbles; MLEN=7"));
}

void loop()
{
  updateInputs();
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[3] = {0, 0, 0};
    buildAnglePropMessage(Message);

    // ANGLE+PROP payload = 20 bits = 5 nibbles -> MLEN=7.
    MyRcTxSerial.sendNibbleMsg(Message, 5, 1);
  }

  RcTxSerial::process();

  if((uint32_t)(millis() - LastPrintMs) >= 1000UL)
  {
    LastPrintMs = millis();

    Serial.print(F("ANGLE="));
    Serial.print(Angle);
    Serial.print(F(" ("));
    Serial.print((float)Angle * 360.0f / 4096.0f, 1);
    Serial.print(F(" deg) PROP="));
    Serial.print(Prop);
    Serial.print(F(" A1335="));
    Serial.print(A1335Ok ? F("OK") : F("ERROR"));
    Serial.print(F(" Wiper="));
    Serial.print(RculI2cPotTx.getWiper());
    Serial.print('/');
    Serial.println(RculI2cPotTx.getMaxWiper());
  }
}
