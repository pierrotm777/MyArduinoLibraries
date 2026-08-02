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

//#define DIGIPOT_ADDRESS RCUL_I2C_POT_TX_AUTO_ADDRESS
#define DIGIPOT_ADDRESS 0x28

// ============================================================================
// MCP23017 - 16 switches on ports A and B
// ============================================================================
/* MCP23017 7-bit I2C address:
     A2 A1 A0 = 0 0 0 -> 0x20
     A2 A1 A0 = 0 0 1 -> 0x21
     ...
     A2 A1 A0 = 1 1 1 -> 0x27

   The MCP23017 is not accessed like a PCF8574. It uses internal registers.
   This example uses BANK=0, which is the power-on default.

   Wiring:
     GPA0 ... GPA7 ---- switch ---- GND  -> SW1  ... SW8
     GPB0 ... GPB7 ---- switch ---- GND  -> SW9  ... SW16

   Internal pull-ups are enabled on both ports. A closed switch reads LOW and
   is converted below to a logical RCUL value of 1.

   Message format:
     SW16: 16 payload bits = 4 nibbles -> MLEN=6
*/
#define MCP23017_ADDRESS 0x24
#define SWITCH_DEBOUNCE_MS 20UL

// MCP23017 register map for IOCON.BANK=0.
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPPUA  0x0C
#define MCP23017_GPPUB  0x0D
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint32_t LastPrintMs = 0;

static bool mcp23017WriteRegister(uint8_t Register, uint8_t Value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(Register);
  Wire.write(Value);
  return (Wire.endTransmission() == 0);
}

static bool mcp23017ReadRegisters(uint8_t StartRegister,
                                  uint8_t *Data,
                                  uint8_t Length)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(StartRegister);
  if(Wire.endTransmission(false) != 0)
  {
    return false;
  }

  const uint8_t Count = Wire.requestFrom((uint8_t)MCP23017_ADDRESS, Length);
  if(Count != Length)
  {
    while(Wire.available()) Wire.read();
    return false;
  }

  for(uint8_t Index = 0; Index < Length; Index++)
  {
    if(!Wire.available()) return false;
    Data[Index] = Wire.read();
  }

  return true;
}

static bool mcp23017ReadContacts(uint16_t &Value)
{
  uint8_t Ports[2];
  if(!mcp23017ReadRegisters(MCP23017_GPIOA, Ports, 2))
  {
    return false;
  }

  // GPA -> low byte (SW1..SW8), GPB -> high byte (SW9..SW16).
  const uint16_t Raw = (uint16_t)Ports[0] |
                       ((uint16_t)Ports[1] << 8);
  Value = (uint16_t)~Raw;
  return true;
}

static bool mcp23017Begin(void)
{
  // Both ports as inputs.
  if(!mcp23017WriteRegister(MCP23017_IODIRA, 0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_IODIRB, 0xFF)) return false;

  // Enable internal pull-ups on all 16 inputs.
  if(!mcp23017WriteRegister(MCP23017_GPPUA, 0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_GPPUB, 0xFF)) return false;

  if(!mcp23017ReadContacts(Contacts)) return false;

  CandidateContacts = Contacts;
  CandidateSinceMs = millis();
  return true;
}

static void updateContacts(void)
{
  uint16_t NewContacts;
  if(!mcp23017ReadContacts(NewContacts))
  {
    return;
  }

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

    Serial.print(F("Contacts=0x"));
    if(Contacts < 0x1000) Serial.print('0');
    if(Contacts < 0x0100) Serial.print('0');
    if(Contacts < 0x0010) Serial.print('0');
    Serial.println(Contacts, HEX);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println(F("RculI2cPotTx + MCP23017 - SW16"));

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

  if(!mcp23017Begin())
  {
    Serial.println(F("MCP23017 not detected or configuration failed"));
    return;
  }

  RculI2cPotTx.printInfo(Serial);
  Serial.print(F("MCP23017 address: 0x"));
  Serial.println(MCP23017_ADDRESS, HEX);
  Serial.println(F("GPA0=SW1 ... GPA7=SW8; GPB0=SW9 ... GPB7=SW16"));
  Serial.println(F("Closed switch=ON; SW16 payload; MLEN=6"));

  MyRcTxSerial.setSweepTest(0);
}

void loop()
{
  updateContacts();

  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    // RcTxSerial expects the most significant payload byte first.
    uint8_t Message[2];
    Message[0] = (uint8_t)(Contacts >> 8);   // SW9..SW16
    Message[1] = (uint8_t)(Contacts);        // SW1..SW8

    // SW16 payload = 16 bits = 4 nibbles -> MLEN=6.
    MyRcTxSerial.sendNibbleMsg(Message, 4, 1);
  }

  RcTxSerial::process();

  // Diagnostic temporaire : affiche chaque position réellement appliquée
  // au potentiomètre numérique. Ce bloc est placé après RcTxSerial::process()
  // car c'est cet appel qui fait avancer le symbole RCUL.
  static uint16_t PreviousWiper = 0xFFFF;
  const uint16_t CurrentWiper = RculI2cPotTx.getWiper();

  // if(CurrentWiper != PreviousWiper)
  // {
  //   PreviousWiper = CurrentWiper;

  //   Serial.print(F("[WIPER_CHANGE] "));
  //   Serial.print(CurrentWiper);
  //   Serial.print('/');
  //   Serial.println(RculI2cPotTx.getMaxWiper());
  // }

  if((uint32_t)(millis() - LastPrintMs) >= 1000UL)
  {
    LastPrintMs = millis();
    Serial.print(F("Contacts=0x"));
    if(Contacts < 0x1000) Serial.print('0');
    if(Contacts < 0x0100) Serial.print('0');
    if(Contacts < 0x0010) Serial.print('0');
    Serial.print(Contacts, HEX);
    Serial.print(F(" Wiper="));
    Serial.print(RculI2cPotTx.getWiper());
    Serial.print('/');
    Serial.println(RculI2cPotTx.getMaxWiper());
  }
}

// void loop()
// {
//   RculI2cPotTx.process();
//   RcTxSerial::process();

//   static uint16_t PreviousWiper = 0xFFFF;
//   const uint16_t CurrentWiper = RculI2cPotTx.getWiper();

//   if(CurrentWiper != PreviousWiper)
//   {
//     PreviousWiper = CurrentWiper;

//     Serial.print(F("[SWEEP] Wiper="));
//     Serial.print(CurrentWiper);
//     Serial.print('/');
//     Serial.println(RculI2cPotTx.getMaxWiper());
//   }
// }