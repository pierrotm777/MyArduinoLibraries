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

static void pcf8574ReleaseInputs(uint8_t Address)
{
  TinyWireM.beginTransmission(Address);
  TinyWireM.send((uint8_t)0xFF);
  (void)TinyWireM.endTransmission();
}

static bool pcf8574Read(uint8_t Address, uint8_t &Value)
{
  const uint8_t Error =
      TinyWireM.requestFrom((uint8_t)Address, (uint8_t)1);

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

#define PCF8574A_ADDRESS 0x38

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static bool HardwareReady = false;

static void updateContacts(void)
{
  uint8_t Raw = 0xFF;

  if(!pcf8574Read(PCF8574A_ADDRESS, Raw))
  {
    HardwareReady = false;
    return;
  }

  const uint8_t NewContacts = (uint8_t)~Raw;
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
      SDA_PIN, SCL_PIN, I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  pcf8574ReleaseInputs(PCF8574A_ADDRESS);

  uint8_t Raw = 0xFF;
  const bool PcfReady = pcf8574Read(PCF8574A_ADDRESS, Raw);

  if(PcfReady)
  {
    Contacts = (uint8_t)~Raw;
    CandidateContacts = Contacts;
    CandidateSinceMs = millis();
  }

  HardwareReady = PotReady && PcfReady;
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
    uint8_t Message[1];
    Message[0] = Contacts;

    // One byte = two payload nibbles.
    MyRcTxSerial.sendNibbleMsg(Message, 2, 1);
  }

  RcTxSerial::process();
}
