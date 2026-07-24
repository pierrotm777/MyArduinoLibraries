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
  #define PROP_PIN     0
  #define PROP_ADC_MAX 4095UL
#else
  #define PROP_PIN     A0
  #define PROP_ADC_MAX 1023UL
#endif

/* Potentiometer wiring:
     one end -> board VCC
     wiper   -> PROP_PIN
     other end -> GND

   PROP is encoded on 8 bits: ADC minimum -> 0, ADC maximum -> 255.
*/

// ============================================================================
// MCP23017 - 16 switches on ports A and B
// ============================================================================
/* MCP23017 7-bit I2C address:
     A2 A1 A0 = 000 -> 0x20 ... 111 -> 0x27

   Register-based device, unlike the PCF8574.

   GPA0 ... GPA7 -> SW1 ... SW8
   GPB0 ... GPB7 -> SW9 ... SW16
   Each switch is connected between its GPIO and GND.
*/
#define MCP23017_ADDRESS 0x24
#define SWITCH_DEBOUNCE_MS 20UL

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPPUA  0x0C
#define MCP23017_GPPUB  0x0D
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

// RCUL payload sizes and resulting MLEN values:
//   SW8          : 2 payload nibbles -> MLEN=4
//   SW8+PROP     : 4 payload nibbles -> MLEN=6
//   SW16         : 4 payload nibbles -> MLEN=6
//   ANGLE+PROP   : 5 payload nibbles -> MLEN=7
//   SW16+PROP    : 6 payload nibbles -> MLEN=8
//
// For SW16+PROP the payload byte order is:
//   Byte[0] = PROP
//   Byte[1] = SW16 high byte (SW9..SW16)
//   Byte[2] = SW16 low byte  (SW1..SW8)
#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint8_t Prop = 0;
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

  const uint16_t Raw = (uint16_t)Ports[0] |
                       ((uint16_t)Ports[1] << 8);
  Value = (uint16_t)~Raw;
  return true;
}

static bool mcp23017Begin(void)
{
  if(!mcp23017WriteRegister(MCP23017_IODIRA, 0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_IODIRB, 0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_GPPUA,  0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_GPPUB,  0xFF)) return false;

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
  Serial.println(F("RculI2cPotTx + MCP23017 - SW16+PROP"));

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

  if(!mcp23017Begin())
  {
    Serial.println(F("MCP23017 not detected or configuration failed"));
    return;
  }

  updateProp();

  RculI2cPotTx.printInfo(Serial);
  Serial.print(F("MCP23017 address: 0x"));
  Serial.println(MCP23017_ADDRESS, HEX);
  Serial.println(F("Payload: PROP + SW16 high byte + SW16 low byte"));
  Serial.println(F("6 payload nibbles; MLEN=8"));
}

void loop()
{
  updateContacts();
  updateProp();
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[3];
    Message[0] = Prop;
    Message[1] = (uint8_t)(Contacts >> 8);  // SW9..SW16
    Message[2] = (uint8_t)(Contacts);       // SW1..SW8

    // SW16+PROP payload = 24 bits = 6 nibbles -> MLEN=8.
    MyRcTxSerial.sendNibbleMsg(Message, 6, 1);
  }

  RcTxSerial::process();

  if((uint32_t)(millis() - LastPrintMs) >= 1000UL)
  {
    LastPrintMs = millis();
    Serial.print(F("PROP="));
    Serial.print(Prop);
    Serial.print(F(" Contacts=0x"));
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
