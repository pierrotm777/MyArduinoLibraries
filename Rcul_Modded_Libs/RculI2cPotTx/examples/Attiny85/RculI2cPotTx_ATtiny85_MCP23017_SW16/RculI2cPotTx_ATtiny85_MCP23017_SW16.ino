#include <Arduino.h>
#include <TinyWireM.h>

#define RCUL_I2C_POT_DEVICE DS3502
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

static RculI2cPotTxClass RculI2cPotTx(RCUL_I2C_POT_DEVICE);

/*
  ATtiny85 + DS3502 + MCP23017

  ATtiny85 / damellis core:
    SDA = PB0 / Arduino pin 0 / physical pin 5
    SCL = PB2 / Arduino pin 2 / physical pin 7

  DS3502:
    I2C address = 0x28
    Managed by RculI2cPotTx.

  MCP23017:
    Default example address = 0x24.
    Change MCP23017_ADDRESS if A0/A1/A2 are wired differently.

  TinyWireM note:
    MCP23017 GPIOA and GPIOB are read one byte at a time.
*/

#define SDA_PIN              0
#define SCL_PIN              2

#define DIGIPOT_ADDRESS      0x28
#define MCP23017_ADDRESS     0x24

#define I2C_FREQUENCY        RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY
#define RCUL_REPEAT          5
#define RCUL_CHANNEL         8

#define SWITCH_DEBOUNCE_MS   20UL

// MCP23017 registers
#define MCP23017_IODIRA      0x00
#define MCP23017_IODIRB      0x01
#define MCP23017_GPPUA       0x0C
#define MCP23017_GPPUB       0x0D
#define MCP23017_GPIOA       0x12
#define MCP23017_GPIOB       0x13

static bool mcp23017WriteRegister(uint8_t Register, uint8_t Value)
{
  TinyWireM.beginTransmission(MCP23017_ADDRESS);
  TinyWireM.send(Register);
  TinyWireM.send(Value);

  return TinyWireM.endTransmission() == 0;
}

static bool mcp23017ReadRegister(uint8_t Register, uint8_t &Value)
{
  TinyWireM.beginTransmission(MCP23017_ADDRESS);
  TinyWireM.send(Register);

  if(TinyWireM.endTransmission() != 0)
  {
    return false;
  }

  /*
    TinyWireM requestFrom() returns 0 on success in the validated setup.
    Read exactly one byte at a time.
  */
  const uint8_t Error =
      TinyWireM.requestFrom((uint8_t)MCP23017_ADDRESS, (uint8_t)1);

  if(Error != 0)
  {
    return false;
  }

  if(TinyWireM.available() < 1)
  {
    return false;
  }

  Value = TinyWireM.receive();
  return true;
}

static bool mcp23017Begin(void)
{
  // GPA0..GPA7 and GPB0..GPB7 as inputs.
  if(!mcp23017WriteRegister(MCP23017_IODIRA, 0xFF))
  {
    return false;
  }

  if(!mcp23017WriteRegister(MCP23017_IODIRB, 0xFF))
  {
    return false;
  }

  // Enable internal pull-ups on all 16 inputs.
  if(!mcp23017WriteRegister(MCP23017_GPPUA, 0xFF))
  {
    return false;
  }

  if(!mcp23017WriteRegister(MCP23017_GPPUB, 0xFF))
  {
    return false;
  }

  uint8_t RawA = 0xFF;
  uint8_t RawB = 0xFF;

  if(!mcp23017ReadRegister(MCP23017_GPIOA, RawA))
  {
    return false;
  }

  if(!mcp23017ReadRegister(MCP23017_GPIOB, RawB))
  {
    return false;
  }

  return true;
}

static bool mcp23017ReadContacts(uint16_t &Contacts)
{
  uint8_t RawA = 0xFF;
  uint8_t RawB = 0xFF;

  if(!mcp23017ReadRegister(MCP23017_GPIOA, RawA))
  {
    return false;
  }

  if(!mcp23017ReadRegister(MCP23017_GPIOB, RawB))
  {
    return false;
  }

  /*
    Closed switch to GND = logical 1.

    GPA0..GPA7 -> SW1..SW8
    GPB0..GPB7 -> SW9..SW16
  */
  Contacts =
      ((uint16_t)((uint8_t)~RawB) << 8) |
      (uint16_t)((uint8_t)~RawA);

  return true;
}

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static bool HardwareReady = false;

static void updateContacts(void)
{
  uint16_t NewContacts = Contacts;

  if(!mcp23017ReadContacts(NewContacts))
  {
    HardwareReady = false;
    return;
  }

  const uint32_t NowMs = millis();

  if(NewContacts != CandidateContacts)
  {
    CandidateContacts = NewContacts;
    CandidateSinceMs = NowMs;
  }
  else if((Contacts != CandidateContacts) &&
          ((uint32_t)(NowMs - CandidateSinceMs) >= SWITCH_DEBOUNCE_MS))
  {
    Contacts = CandidateContacts;
  }
}

void setup()
{
  TinyWireM.begin();

  const bool PotReady = RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  const bool McpReady = mcp23017Begin();

  if(McpReady)
  {
    mcp23017ReadContacts(Contacts);
    CandidateContacts = Contacts;
    CandidateSinceMs = millis();
  }

  HardwareReady = PotReady && McpReady;
}

void loop()
{
  if(!HardwareReady)
  {
    return;
  }

  updateContacts();

  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[2];

    // RcTxSerial expects the most significant payload byte first.
    Message[0] = (uint8_t)(Contacts >> 8);  // SW9..SW16
    Message[1] = (uint8_t)Contacts;         // SW1..SW8

    // 2 bytes = 4 payload nibbles.
    MyRcTxSerial.sendNibbleMsg(Message, 4, 1);
  }

  RcTxSerial::process();
}
