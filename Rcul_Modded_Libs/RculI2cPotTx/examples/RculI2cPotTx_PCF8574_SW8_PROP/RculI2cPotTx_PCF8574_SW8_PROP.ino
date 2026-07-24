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

// ============================================================================
// DIGITAL POTENTIOMETER
// ============================================================================
#define DIGIPOT_ADDRESS RCUL_I2C_POT_TX_AUTO_ADDRESS
// #define DIGIPOT_ADDRESS 0x2D

// ============================================================================
// PCF8574
// ============================================================================
/* PCF8574 addresses with A2/A1/A0:
      000 -> 0x20
      001 -> 0x21
      010 -> 0x22
      011 -> 0x23
      100 -> 0x24
      101 -> 0x25
      110 -> 0x26
      111 -> 0x27

   For a PCF8574A, the address range is normally 0x38..0x3F.

   Switch wiring:
      P0..P7 ---- switch ---- GND

   P0 = SW1 ... P7 = SW8.
   Closed switch = LOW on the PCF8574, then inverted to logical 1. */
#define PCF8574_ADDRESS 0x20
#define SWITCH_DEBOUNCE_MS 20UL

// ============================================================================
// Analog proportional input
// ============================================================================
/* Potentiometer wiring:
     one end -> board VCC
     wiper   -> PROP_PIN
     other end -> GND

   The analog value is converted to 0..255. */
#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  #define PROP_PIN     0
  #define PROP_ADC_MAX 4095UL
#else
  #define PROP_PIN     A0
  #define PROP_ADC_MAX 1023UL
#endif

// ============================================================================
// RCUL message format
// ============================================================================
/* Known modes:
     SW8          MLEN=4
     SW8+PROP     MLEN=6
     SW16         MLEN=6
     ANGLE+PROP   MLEN=7
     SW16+PROP    MLEN=8

   SW8+PROP payload:
     Byte[0] = PROP
     Byte[1] = SW8

   Payload = 16 bits = 4 nibbles.
   RcTxSerial adds framing, therefore MLEN=6. */
#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint8_t Prop = 0;
static uint32_t LastPrintMs = 0;

// ============================================================================
// PCF8574 low-level access
// ============================================================================
static bool pcf8574Write(uint8_t Value)
{
  Wire.beginTransmission(PCF8574_ADDRESS);
  Wire.write(Value);
  return (Wire.endTransmission() == 0);
}

static bool pcf8574Read(uint8_t &Value)
{
  const uint8_t Count = Wire.requestFrom((uint8_t)PCF8574_ADDRESS,
                                         (uint8_t)1);
  if((Count != 1) || !Wire.available())
  {
    return false;
  }

  Value = Wire.read();
  return true;
}

static bool pcf8574Begin(void)
{
  // Writing 1 releases each quasi-bidirectional pin for input use.
  if(!pcf8574Write(0xFF))
  {
    return false;
  }

  uint8_t Raw;
  if(!pcf8574Read(Raw))
  {
    return false;
  }

  Contacts = (uint8_t)~Raw;
  CandidateContacts = Contacts;
  CandidateSinceMs = millis();
  return true;
}

// ============================================================================
// Inputs
// ============================================================================
static void updateContacts(void)
{
  uint8_t Raw;
  if(!pcf8574Read(Raw))
  {
    return;
  }

  const uint8_t NewContacts = (uint8_t)~Raw;

  if(NewContacts != CandidateContacts)
  {
    CandidateContacts = NewContacts;
    CandidateSinceMs = millis();
    return;
  }

  if((Contacts != CandidateContacts) &&
     ((uint32_t)(millis() - CandidateSinceMs) >= SWITCH_DEBOUNCE_MS))
  {
    Contacts = CandidateContacts;
  }
}

static void updateProp(void)
{
  const uint32_t Raw = (uint32_t)analogRead(PROP_PIN);
  Prop = (uint8_t)((Raw * 255UL + (PROP_ADC_MAX / 2UL)) / PROP_ADC_MAX);
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println(F("RculI2cPotTx + PCF8574 - SW8+PROP"));

#if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
  analogReadResolution(12);
#endif
  pinMode(PROP_PIN, INPUT);

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

  if(!pcf8574Begin())
  {
    Serial.println(F("PCF8574 not detected"));
    return;
  }

  updateProp();

  RculI2cPotTx.printInfo(Serial);
  Serial.print(F("PCF8574 address: 0x"));
  Serial.println(PCF8574_ADDRESS, HEX);
  Serial.println(F("Payload order: PROP then SW8; 4 payload nibbles; MLEN=6"));
}

void loop()
{
  updateContacts();
  updateProp();

  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[2];
    Message[0] = Prop;
    Message[1] = Contacts;

    MyRcTxSerial.sendNibbleMsg(Message, 4, 1);
  }

  RcTxSerial::process();

  if((uint32_t)(millis() - LastPrintMs) >= 1000UL)
  {
    LastPrintMs = millis();

    Serial.print(F("PROP="));
    Serial.print(Prop);
    Serial.print(F(" Contacts=0x"));
    if(Contacts < 0x10) Serial.print('0');
    Serial.print(Contacts, HEX);
    Serial.print(F(" Wiper="));
    Serial.print(RculI2cPotTx.getWiper());
    Serial.print('/');
    Serial.println(RculI2cPotTx.getMaxWiper());
  }
}
