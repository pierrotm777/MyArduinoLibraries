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
// I2C BUS SHARED BY THE DIGITAL POTENTIOMETER AND PCF8574
// ============================================================================
//Uno
//#define SDA_PIN A4
//#define SCL_PIN A5
//ESP32 C3
#define SDA_PIN 5
#define SCL_PIN 6

/* Possible speeds:
   RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY = 100000UL
   RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY    = 400000UL

   ESP32: SDA_PIN and SCL_PIN select the I2C pins.
   AVR  : the hardware SDA/SCL pins are fixed by the selected board. */
#define I2C_FREQUENCY RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY
// #define I2C_FREQUENCY RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY

// ============================================================================
// DIGITAL POTENTIOMETER
// ============================================================================
/* Automatic address: MCP4561=0x2C, DS3502=0x28.
   A different address can be forced if required. */
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

   Wiring of each switch:
      PCF8574 P0..P7 ---- switch ---- GND

   The PCF8574 pins are quasi-bidirectional. Writing 1 releases a pin and lets
   it be used as an input. Closed switches therefore read LOW and are inverted
   below so that a closed switch gives a logical RCUL value of 1. */
#define PCF8574_ADDRESS 0x20

// Debounce: a new value must remain unchanged for this duration.
#define SWITCH_DEBOUNCE_MS 20UL

// ============================================================================
// RCUL SERIAL TRANSMITTER
// ============================================================================
#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

// Current debounced state: bit 0 = SW1/P0 ... bit 7 = SW8/P7.
static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint32_t LastPrintMs = 0;

// ============================================================================
// PCF8574 LOW-LEVEL ACCESS
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
  /* A written 1 releases every quasi-bidirectional pin for input use. */
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
// SWITCH READING AND DEBOUNCE
// ============================================================================
static void updateContacts(void)
{
  uint8_t Raw;
  if(!pcf8574Read(Raw))
  {
    return;
  }

  /* Closed switch = LOW on PCF8574 = 1 in the RCUL contact byte. */
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

    Serial.print(F("Contacts = 0b"));
    for(int8_t Bit = 7; Bit >= 0; --Bit)
    {
      Serial.print((Contacts >> Bit) & 0x01);
    }
    Serial.print(F("  0x"));
    if(Contacts < 0x10)
    {
      Serial.print('0');
    }
    Serial.println(Contacts, HEX);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println(F("RculI2cPotTx + PCF8574 8 switches"));

  /* Optional digital-potentiometer wiper range.

     If setWiperRange() is not called: MCP4561=4..252, DS3502=2..125.

     Examples:
       RculI2cPotTx.setWiperRange(0, RculI2cPotTx.getMaxWiper()); // full selected-device range
       RculI2cPotTx.setWiperRange(10, 245);  // reduced custom range

     Channel reversal must be configured in the radio. */
  // RculI2cPotTx.setWiperRange(10, 245);

  /* begin() initializes the shared Wire bus and the selected digital potentiometer. */
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

  RculI2cPotTx.printInfo(Serial);

  Serial.print(F("PCF8574 address: 0x"));
  Serial.println(PCF8574_ADDRESS, HEX);
  Serial.println(F("P0=SW1 ... P7=SW8; closed switch = ON"));
}

void loop()
{
  updateContacts();

  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    /* Eight contacts = 8 bits = 2 nibbles.
       One message byte is generated directly from P0..P7:
         P0 -> bit 0 / SW1
         ...
         P7 -> bit 7 / SW8 */
    MyRcTxSerial.sendNibbleMsg(&Contacts, 2, 1);
  }

  RcTxSerial::process();

  /* Optional periodic debug of the dynamic digital-potentiometer position. */
  if((uint32_t)(millis() - LastPrintMs) >= 1000UL)
  {
    LastPrintMs = millis();
    Serial.print(F("Wiper="));
    Serial.print(RculI2cPotTx.getWiper());
    Serial.print('/');
    Serial.println(RculI2cPotTx.getMaxWiper());
  }
}
