/*
  RculI2cPotTx_Nano_CALLBACK_PTR_6A_PCF8574_SW8_Test

  Arduino Nano + DS3502 + PCF8574A + Pro-Tronik PTR-6A.
  Synchronisation RCUL sur GDO0 du CC2500 via le profil PTR-6A.

  Configuration validee:
    - offset PTR-6A : 2750 us, applique automatiquement par la bibliotheque
    - RCUL_REPEAT   : 2
    - filtre X-Any  : F=0, reglage cote recepteur / XanySpy
    - qualite       : 100 % observee
    - I2C           : 100 kHz

  Cablage Nano:
    A4 -> SDA DS3502 + SDA PCF8574A
    A5 -> SCL DS3502 + SCL PCF8574A
    D8 <- GDO0 PTR-6A
    GND commun obligatoire

  Adresses:
    DS3502   : 0x28
    PCF8574A : 0x38

  SW8:
    P0 -> SW1 -> GND
    ...
    P7 -> SW8 -> GND

  Contact ferme = bit logique 1 transmis.

  IMPORTANT:
    Le filtre F=0 est un reglage du recepteur / programme de test.
    Il n'est pas configure par ce sketch.
*/

#include <Arduino.h>
#include <Wire.h>

#define RCUL_I2C_POT_DEVICE DS3502
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

#define SDA_PIN                 A4
#define SCL_PIN                 A5
#define GDO0_PIN                8

#define DIGIPOT_ADDRESS         0x28
#define PCF8574A_ADDRESS        0x38
#define I2C_FREQUENCY           100000UL

#define RCUL_REPEAT             2
#define RCUL_FIFO_SIZE          8
#define RCUL_CHANNEL            8

#define SWITCH_DEBOUNCE_MS      20UL
#define PCF_READ_PERIOD_US      1000UL

#define LOCAL_DEBUG             0

static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A,
    GDO0_PIN
);

static RcTxSerial MyRcTxSerial(
    &RculI2cPotTx,
    RCUL_REPEAT,
    RCUL_FIFO_SIZE,
    RCUL_CHANNEL
);

static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint32_t LastPcfReadUs = 0;
static bool PcfReady = false;

static bool pcf8574aReleaseInputs(void)
{
  Wire.beginTransmission(PCF8574A_ADDRESS);
  Wire.write((uint8_t)0xFF);
  return (Wire.endTransmission() == 0);
}

static bool pcf8574aRead(uint8_t &Value)
{
  const uint8_t Nb =
      Wire.requestFrom((uint8_t)PCF8574A_ADDRESS, (uint8_t)1);

  if((Nb != 1) || (Wire.available() < 1))
  {
    return false;
  }

  Value = (uint8_t)Wire.read();
  return true;
}

static void updateContacts(void)
{
  const uint32_t NowUs = micros();

  if((uint32_t)(NowUs - LastPcfReadUs) < PCF_READ_PERIOD_US)
  {
    return;
  }

  LastPcfReadUs = NowUs;

  uint8_t Raw = 0xFF;

  if(!pcf8574aRead(Raw))
  {
    PcfReady = false;
    return;
  }

  PcfReady = true;

  // PCF8574A relache = 1, contact ferme vers GND = 0.
  // On inverse pour transmettre contact ferme = 1.
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
  Serial.begin(115200);

  if(!RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS, // ignore en CALLBACK PTR-6A
      1500))
  {
    Serial.println(F("RculI2cPotTx begin ERROR"));
    while(1);
  }

  RculI2cPotTx.setTableStorageEEPROM(0);

  if(RculI2cPotTx.useStoredRculTable())
    Serial.println(F("RCUL table: STORED EEPROM"));
  else
    Serial.println(F("RCUL table: EMBEDDED DEFAULT"));

  if(!pcf8574aReleaseInputs())
  {
    Serial.println(F("PCF8574A write ERROR"));
    while(1);
  }

  uint8_t Raw = 0xFF;

  if(!pcf8574aRead(Raw))
  {
    Serial.println(F("PCF8574A read ERROR"));
    while(1);
  }

  Contacts = (uint8_t)~Raw;
  CandidateContacts = Contacts;
  CandidateSinceMs = millis();
  PcfReady = true;

  RculI2cPotTx.printInfo(Serial);

  Serial.print(F("PCF8574A address : 0x"));
  Serial.println(PCF8574A_ADDRESS, HEX);

  Serial.print(F("Initial SW8      : 0x"));
  if(Contacts < 0x10) Serial.print('0');
  Serial.println(Contacts, HEX);

  Serial.print(F("RCUL_REPEAT      : "));
  Serial.println(RCUL_REPEAT);

  Serial.println(F("Validated RX condition: F=0 / quality=100%"));
  Serial.println(F("PTR-6A + PCF8574A SW8 test started"));
}

void loop()
{
  // Priorite au polling GDO0.
  RculI2cPotTx.process();

  // Lecture SW8 cadencee pour ne pas monopoliser le bus I2C.
  updateContacts();

  // Re-poll immediatement apres la transaction I2C du PCF8574A.
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[1];
    Message[0] = Contacts;

    // SW8 = 8 bits = 2 nibbles utiles + checksum RcTxSerial.
    MyRcTxSerial.sendNibbleMsg(Message, 2, 1);
  }

  RcTxSerial::process();

  // Re-poll apres l'eventuelle ecriture du DS3502.
  RculI2cPotTx.process();

#if LOCAL_DEBUG
  static uint8_t PreviousContacts = 0xFF;

  if(Contacts != PreviousContacts)
  {
    PreviousContacts = Contacts;

    Serial.print(F("SW8=0x"));
    if(Contacts < 0x10) Serial.print('0');
    Serial.println(Contacts, HEX);
  }
#endif
}
