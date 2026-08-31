/*
  RculI2cPotTx_ESP32_PTR6A_MCP4661_POT1_PCF8574_SW8

  TEST RCUL REEL : SW8 uniquement
  PAS DE PROP

  Hardware:
    ESP32-S3

    MCP4661:
      address = 0x2F
      channel = POT1

    PCF8574:
      address = 0x20
      P0 -> SW1
      P1 -> SW2
      ...
      P7 -> SW8

      Chaque switch:
        PCF8574 Px ---- switch ---- GND

      switch ferme = RCUL ON

    SDA  = GPIO12
    SCL  = GPIO13
    GDO0 = GPIO5

    I2C = 100 kHz

  PTR-6A:
    callback phase = 2750 us

  RCUL:
    RCUL_REPEAT = 2
    => 3 presentations physiques de chaque symbole

  Payload SW8:
    1 octet
    2 nibbles utiles
    + checksum RcTxSerial

    MLEN attendu = 4

  Pas de Serial.print() dans loop().
*/

#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// RculI2cPotTx
// ============================================================================

#define RCUL_I2C_POT_DEVICE MCP4661
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE

#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

// ============================================================================
// Hardware
// ============================================================================

#define SDA_PIN                 12
#define SCL_PIN                 13
#define GDO0_PIN                 5

#define DIGIPOT_ADDRESS       0x2F
#define PCF8574_ADDRESS       0x38

#define I2C_FREQUENCY       100000UL

// ============================================================================
// PCF8574
// ============================================================================

#define SWITCH_DEBOUNCE_MS      20UL

static bool Pcf8574Ready = false;

static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;

// ============================================================================
// RCUL
// ============================================================================

#define RCUL_REPEAT               2
#define RCUL_FIFO_SIZE            8
#define RCUL_CHANNEL              8

#define PTR6A_PHASE_US         2750U

// ============================================================================
// MCP4661 POT1
// ============================================================================

static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A,
    GDO0_PIN,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_1
);

// ============================================================================
// RcTxSerial
// ============================================================================

static RcTxSerial MyRcTxSerial(
    &RculI2cPotTx,
    RCUL_REPEAT,
    RCUL_FIFO_SIZE,
    RCUL_CHANNEL
);

// ============================================================================
// PCF8574 low level
// ============================================================================

static bool pcf8574Write(uint8_t Value)
{
  Wire.beginTransmission(PCF8574_ADDRESS);

  Wire.write(Value);

  return (Wire.endTransmission() == 0);
}

// ----------------------------------------------------------------------------

static bool pcf8574Read(uint8_t &Value)
{
  const uint8_t Count =
      Wire.requestFrom(
          (uint8_t)PCF8574_ADDRESS,
          (uint8_t)1);

  if((Count != 1) || !Wire.available())
  {
    return false;
  }

  Value = Wire.read();

  return true;
}

// ----------------------------------------------------------------------------

static bool pcf8574Begin(void)
{
  /*
    PCF8574 quasi-bidirectionnel.

    Ecrire 1 libere chaque pin afin qu'elle
    puisse etre utilisee comme entree.
  */

  if(!pcf8574Write(0xFF))
  {
    return false;
  }

  uint8_t Raw;

  if(!pcf8574Read(Raw))
  {
    return false;
  }

  /*
    Contact ferme:
      pin PCF8574 = LOW

    On inverse donc:
      LOW -> bit RCUL = 1
  */

  Contacts = (uint8_t)~Raw;

  CandidateContacts = Contacts;
  CandidateSinceMs = millis();

  return true;
}

// ============================================================================
// Lecture + debounce SW1..SW8
// ============================================================================

static void updateContacts(void)
{
  uint8_t Raw;

  if(!pcf8574Read(Raw))
  {
    Pcf8574Ready = false;
    return;
  }

  Pcf8574Ready = true;

  const uint8_t NewContacts =
      (uint8_t)~Raw;

  // Nouveau candidat
  if(NewContacts != CandidateContacts)
  {
    CandidateContacts = NewContacts;
    CandidateSinceMs = millis();

    return;
  }

  // Le nouvel etat doit rester stable 20 ms
  if((Contacts != CandidateContacts) &&
     ((uint32_t)(millis() - CandidateSinceMs)
        >= SWITCH_DEBOUNCE_MS))
  {
    Contacts = CandidateContacts;
  }
}

// ============================================================================
// Affichage initial SW8
// ============================================================================

static void printContacts(void)
{
  Serial.print(F("SW8 initial : 0b"));

  for(int8_t Bit = 7; Bit >= 0; Bit--)
  {
    Serial.print(
        (Contacts >> Bit) & 0x01);
  }

  Serial.print(F("  0x"));

  if(Contacts < 0x10)
  {
    Serial.print('0');
  }

  Serial.println(Contacts, HEX);
}

// ============================================================================
// SETUP
// ============================================================================

void setup()
{
  Serial.begin(115200);

  const uint32_t StartMs = millis();

  while(!Serial &&
        ((uint32_t)(millis() - StartMs) < 1500UL))
  {
    delay(1);
  }

  Serial.println();
  Serial.println(F("=========================================================="));
  Serial.println(F("RCUL SW8 REAL MESSAGE TEST"));
  Serial.println(F("ESP32-S3 + MCP4661 POT1 + PCF8574"));
  Serial.println(F("NO PROP"));
  Serial.println(F("=========================================================="));

  // --------------------------------------------------------------------------
  // MCP4661
  // --------------------------------------------------------------------------

  if(!RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500))
  {
    Serial.println(
        F("*** MCP4661 / RculI2cPotTx ERROR ***"));

    while(1)
    {
      delay(1000);
    }
  }

  // --------------------------------------------------------------------------
  // STORED calibration
  // --------------------------------------------------------------------------

  RculI2cPotTx.setTableStoragePreferences(
      "RculPotM0");

  const bool StoredOk =
      RculI2cPotTx.useStoredRculTable();

  Serial.print(F("RCUL table : "));

  if(StoredOk)
  {
    Serial.println(F("STORED"));
  }
  else
  {
    Serial.println(
        F("*** STORED TABLE NOT FOUND ***"));

    while(1)
    {
      delay(1000);
    }
  }

  // --------------------------------------------------------------------------
  // PTR-6A
  // --------------------------------------------------------------------------

  RculI2cPotTx.setCallbackSyncOffsetUs(
      PTR6A_PHASE_US);

  // --------------------------------------------------------------------------
  // PCF8574
  // --------------------------------------------------------------------------

  if(!pcf8574Begin())
  {
    Serial.println(
        F("*** PCF8574 0x20 NOT DETECTED ***"));

    while(1)
    {
      delay(1000);
    }
  }

  Pcf8574Ready = true;

  // --------------------------------------------------------------------------
  // Informations
  // --------------------------------------------------------------------------

  RculI2cPotTx.printInfo(Serial);

  Serial.println();

  Serial.print(F("PCF8574 address : 0x"));
  Serial.println(PCF8574_ADDRESS, HEX);

  Serial.println(
      F("P0=SW1 ... P7=SW8"));

  Serial.println(
      F("Closed switch = ON"));

  Serial.print(F("PTR-6A phase    : "));
  Serial.print(
      RculI2cPotTx.getCallbackSyncOffsetUs());
  Serial.println(F(" us"));

  Serial.print(F("RCUL_REPEAT     : "));
  Serial.println(RCUL_REPEAT);

  Serial.println(
      F("SW8 payload     : 1 byte / 2 nibbles"));

  Serial.println(
      F("Checksum        : ON"));

  Serial.println(
      F("Expected MLEN   : 4"));

  printContacts();

  Serial.println();
  Serial.println(
      F("XanySpyV7: normal message decoding"));
  Serial.println(
      F("Toggle SW1..SW8 on PCF8574"));
  Serial.println(
      F("NO SERIAL PRINT DURING TX"));
  Serial.println(F("=========================================================="));
}

// ============================================================================
// LOOP
// ============================================================================

void loop()
{
  /*
    La priorite reste la synchro PTR-6A.

    Aucun Serial.print ici.
  */

  // --------------------------------------------------------------------------
  // Premier polling GDO0
  // --------------------------------------------------------------------------

  RculI2cPotTx.process();

  // --------------------------------------------------------------------------
  // Avancement du moteur RcTxSerial
  // --------------------------------------------------------------------------

  RcTxSerial::process();

  // --------------------------------------------------------------------------
  // Re-poll immediat apres RcTxSerial
  // --------------------------------------------------------------------------

  RculI2cPotTx.process();

  // --------------------------------------------------------------------------
  // Lecture des 8 contacts
  // --------------------------------------------------------------------------

  updateContacts();

  // --------------------------------------------------------------------------
  // Re-poll apres transaction I2C PCF8574
  // --------------------------------------------------------------------------

  RculI2cPotTx.process();

  // --------------------------------------------------------------------------
  // Charge un nouveau message SW8 lorsque RcTxSerial est libre
  // --------------------------------------------------------------------------

  if(Pcf8574Ready &&
     MyRcTxSerial.isReadyForTx())
  {
    /*
      Payload:

        bit 0 = P0 = SW1
        bit 1 = P1 = SW2
        ...
        bit 7 = P7 = SW8

      8 bits = 2 nibbles.

      Le dernier argument = 1:
        RcTxSerial ajoute le checksum.
    */

    uint8_t Message = Contacts;

    MyRcTxSerial.sendNibbleMsg(
        &Message,
        2,
        1);
  }

  // --------------------------------------------------------------------------
  // Traitement immediat du message nouvellement charge
  // --------------------------------------------------------------------------

  RcTxSerial::process();

  RculI2cPotTx.process();
}