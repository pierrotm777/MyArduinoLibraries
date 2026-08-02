#include <Arduino.h>
#include <TinyWireM.h>
#include <TinyCppmReader.h>

#define RCUL_I2C_POT_DEVICE DS3502
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

/*
  ATtiny85 synchronized by the transmitter PPM frame.

  PB0 / physical pin 5 : SDA
  PB1 / physical pin 6 : PPM IN
  PB2 / physical pin 7 : SCL
  PB3 / physical pin 2 : PROP analog input

  DS3502   : 0x28
  PCF8574A : 0x38
*/

#define SDA_PIN             0
#define SCL_PIN             2
#define PPM_INPUT_PIN       1
#define PROP_PIN            9      // A3 / ADC3 / PB3
#define PROP_ADC_MAX        1023UL

#define DIGIPOT_ADDRESS     0x28
#define PCF8574A_ADDRESS    0x38
#define I2C_FREQUENCY       RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY

#define RCUL_REPEAT         5
#define RCUL_CHANNEL        8
#define SWITCH_DEBOUNCE_MS  20UL

TinyCppmReader TinyCppmReader; // Object creation

static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    TinyCppmReader
);

static RcTxSerial MyRcTxSerial(
    &RculI2cPotTx,
    RCUL_REPEAT,
    8,
    RCUL_CHANNEL
);

static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint8_t Prop = 0;
static bool HardwareReady = false;

static void pcf8574aReleaseInputs(void)
{
  TinyWireM.beginTransmission(PCF8574A_ADDRESS);
  TinyWireM.send((uint8_t)0xFF);
  (void)TinyWireM.endTransmission();
}

static bool pcf8574aRead(uint8_t &Value)
{
  const uint8_t Error =
      TinyWireM.requestFrom((uint8_t)PCF8574A_ADDRESS, (uint8_t)1);

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

static void updateContacts(void)
{
  uint8_t Raw = 0xFF;

  if(!pcf8574aRead(Raw))
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

static void updateProp(void)
{
  const uint16_t Raw = (uint16_t)analogRead(PROP_PIN);
  Prop = (uint8_t)(((uint32_t)Raw * 255UL + 511UL) / PROP_ADC_MAX);
}

void setup()
{
  pinMode(PROP_PIN, INPUT);
  digitalWrite(PROP_PIN, LOW);

  TinyWireM.begin();

  const bool PotReady = RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  /*
    PB1 replaces the former status LED.
  */
  const bool PpmReady = TinyCppmReader::attach(PPM_INPUT_PIN);

  pcf8574aReleaseInputs();

  uint8_t Raw = 0xFF;
  const bool PcfReady = pcf8574aRead(Raw);

  if(PcfReady)
  {
    Contacts = (uint8_t)~Raw;
    CandidateContacts = Contacts;
    CandidateSinceMs = millis();
  }

  updateProp();
  HardwareReady = PotReady && PpmReady && PcfReady;
}

void loop()
{
  if(!HardwareReady)
  {
    return;
  }

  updateContacts();
  updateProp();

  /*
    In PPM IN mode process() does not create an internal 18 ms top.
    RcTxSerial receives the top directly from TinyCppmReader.
  */
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[2];

    Message[0] = Prop;
    Message[1] = Contacts;

    MyRcTxSerial.sendNibbleMsg(Message, 4, 1);
  }

  RcTxSerial::process();
}
