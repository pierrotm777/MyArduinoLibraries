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

#define PCF8574A_1_ADDRESS 0x38
#define PCF8574A_2_ADDRESS 0x39
#define PROP_PIN              9
#define PROP_ADC_MAX          1023UL

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint8_t Prop = 0;
static bool HardwareReady = false;

static bool readContacts16(uint16_t &Value)
{
  uint8_t RawLow = 0xFF;
  uint8_t RawHigh = 0xFF;

  if(!pcf8574Read(PCF8574A_1_ADDRESS, RawLow))
  {
    return false;
  }

  if(!pcf8574Read(PCF8574A_2_ADDRESS, RawHigh))
  {
    return false;
  }

  Value = (uint16_t)((uint16_t)((uint8_t)~RawHigh) << 8) |
          (uint16_t)((uint8_t)~RawLow);
  return true;
}

static void updateContacts(void)
{
  uint16_t NewContacts = Contacts;

  if(!readContacts16(NewContacts))
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

static void updateProp(void)
{
  const uint16_t Raw = (uint16_t)analogRead(PROP_PIN);
  Prop = (uint8_t)(((uint32_t)Raw * 255UL + 511UL) / 1023UL);
}

void setup()
{
  pinMode(PROP_PIN, INPUT);
  digitalWrite(PROP_PIN, LOW);

  TinyWireM.begin();

  const bool PotReady = RculI2cPotTx.begin(
      SDA_PIN, SCL_PIN, I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  pcf8574ReleaseInputs(PCF8574A_1_ADDRESS);
  pcf8574ReleaseInputs(PCF8574A_2_ADDRESS);

  uint16_t InitialContacts = 0;
  const bool PcfReady = readContacts16(InitialContacts);

  if(PcfReady)
  {
    Contacts = InitialContacts;
    CandidateContacts = Contacts;
    CandidateSinceMs = millis();
  }

  updateProp();
  HardwareReady = PotReady && PcfReady;
}

void loop()
{
  if(!HardwareReady)
  {
    return;
  }

  updateContacts();
  updateProp();
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[3];
    Message[0] = Prop;
    Message[1] = (uint8_t)(Contacts >> 8);  // SW9..SW16
    Message[2] = (uint8_t)Contacts;         // SW1..SW8

    // Three bytes = six payload nibbles.
    MyRcTxSerial.sendNibbleMsg(Message, 6, 1);
  }

  RcTxSerial::process();
}
