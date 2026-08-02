#include <Arduino.h>
#include <Wire.h>
#include <ESP32_PPM.h>

#define RCUL_I2C_POT_DEVICE DS3502
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

/*
  ESP32-C3 SW8+PROP synchronized by a received PPM frame.

  The ESP32_PPM RX API exposes rxAvailable(), so the library uses the
  CALLBACK synchronization mode.

  SDA        : GPIO5
  SCL        : GPIO6
  PPM IN     : GPIO4
  PROP       : GPIO0
  DS3502     : 0x28
  PCF8574A   : 0x38

  Never apply a 5 V PPM signal directly to an ESP32 GPIO.
*/

#define SDA_PIN             5
#define SCL_PIN             6
#define PPM_INPUT_PIN       4
#define PROP_PIN            0
#define PROP_ADC_MAX        4095UL

#define DIGIPOT_ADDRESS     0x28
#define PCF8574A_ADDRESS    0x38
#define I2C_FREQUENCY       RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY

#define PPM_CHANNEL_NB      8
#define PPM_SYNC_MIN_US     3500
#define RCUL_REPEAT         5
#define RCUL_CHANNEL        8
#define SWITCH_DEBOUNCE_MS  20UL

static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK
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

static bool pcf8574aReleaseInputs(void)
{
  Wire.beginTransmission(PCF8574A_ADDRESS);
  Wire.write((uint8_t)0xFF);
  return Wire.endTransmission() == 0;
}

static bool pcf8574aRead(uint8_t &Value)
{
  const uint8_t Count =
      Wire.requestFrom((uint8_t)PCF8574A_ADDRESS, (uint8_t)1);

  if((Count != 1U) || !Wire.available())
  {
    return false;
  }

  Value = Wire.read();
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
  const uint32_t Raw = (uint32_t)analogRead(PROP_PIN);
  Prop = (uint8_t)((Raw * 255UL + (PROP_ADC_MAX / 2UL)) / PROP_ADC_MAX);
}

void setup()
{
  Serial.begin(115200);
  delay(300);

  analogReadResolution(12);
  analogSetPinAttenuation(PROP_PIN, ADC_11db);
  pinMode(PROP_PIN, INPUT);

  const bool PotReady = RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  /*
    ESP32_PPM receives the complete PPM frame.
    true selects rising-edge measurement.
  */
  Cppm32.beginRx(
      PPM_INPUT_PIN,
      PPM_CHANNEL_NB,
      true,
      PPM_SYNC_MIN_US,
      800,
      2200);

  const bool PcfWriteReady = pcf8574aReleaseInputs();

  uint8_t Raw = 0xFF;
  const bool PcfReadReady = pcf8574aRead(Raw);

  if(PcfReadReady)
  {
    Contacts = (uint8_t)~Raw;
    CandidateContacts = Contacts;
    CandidateSinceMs = millis();
  }

  updateProp();
  HardwareReady = PotReady && PcfWriteReady && PcfReadReady;

  RculI2cPotTx.printInfo(Serial);
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
    One callback pulse for each received PPM frame.
  */
  if(Cppm32.rxAvailable())
  {
    RculI2cPotTx.syncPulse();
  }

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
