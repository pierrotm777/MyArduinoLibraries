/*
  RculI2cPotTx_ESP32_PTR6A_MCP4661_POT1_MCP23017_SW16

  Meme base que la version PCF8574A SW8 validee a 100 %,
  avec MCP23017 16 voies a la place du PCF8574A.

  PAS DE PROP
  PAS DE LEARN
  PAS D'OLED
  PAS DE DEBUG DANS loop()

  Hardware:
    ESP32-S3

    MCP4661:
      address = 0x2F
      channel = POT1

    MCP23017:
      address = 0x24
      GPA0 -> SW1
      GPA1 -> SW2
      ...
      GPA7 -> SW8
      GPB0 -> SW9
      ...
      GPB7 -> SW16

      Chaque switch:
        MCP23017 GPIO ---- switch ---- GND

      Pull-up internes MCP23017 actifs.
      Switch ferme = RCUL ON.

    SDA  = GPIO12
    SCL  = GPIO13
    GDO0 = GPIO5

    I2C = 100 kHz

  PTR-6A:
    callback phase = 2750 us

  RCUL:
    RCUL_REPEAT = 2
    => 3 presentations physiques de chaque symbole

  Payload SW16:
    Byte[0] = SW9..SW16
    Byte[1] = SW1..SW8

    16 bits = 4 nibbles utiles
    + checksum RcTxSerial

    MLEN attendu = 6
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

#define DIGIPOT_ADDRESS         0x2F
#define MCP23017_ADDRESS        0x24

#define I2C_FREQUENCY           100000UL

// ============================================================================
// MCP23017
// ============================================================================

#define SWITCH_DEBOUNCE_MS      20UL

#define MCP23017_IODIRA         0x00
#define MCP23017_IODIRB         0x01
#define MCP23017_IOCON          0x0A
#define MCP23017_GPPUA          0x0C
#define MCP23017_GPPUB          0x0D
#define MCP23017_GPIOA          0x12
#define MCP23017_GPIOB          0x13

static bool Mcp23017Ready = false;

static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
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
// MCP23017 low level
// ============================================================================

static bool mcp23017WriteRegister(uint8_t Register, uint8_t Value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(Register);
  Wire.write(Value);

  return (Wire.endTransmission() == 0);
}

// ----------------------------------------------------------------------------

static bool mcp23017ReadRegisters(
    uint8_t StartRegister,
    uint8_t *Data,
    uint8_t Length)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(StartRegister);

  if(Wire.endTransmission(false) != 0)
  {
    return false;
  }

  const uint8_t Count =
      Wire.requestFrom(
          (uint8_t)MCP23017_ADDRESS,
          Length);

  if(Count != Length)
  {
    while(Wire.available())
    {
      Wire.read();
    }

    return false;
  }

  for(uint8_t i = 0; i < Length; i++)
  {
    if(!Wire.available())
    {
      return false;
    }

    Data[i] = Wire.read();
  }

  return true;
}

// ============================================================================
// Lecture SW1..SW16
// ============================================================================

static bool mcp23017ReadContacts(uint16_t &Value)
{
  uint8_t Ports[2];

  /*
    Lecture consecutive:
      Ports[0] = GPIOA = SW1..SW8
      Ports[1] = GPIOB = SW9..SW16
  */

  if(!mcp23017ReadRegisters(
      MCP23017_GPIOA,
      Ports,
      2))
  {
    return false;
  }

  const uint16_t Raw =
      (uint16_t)Ports[0] |
      ((uint16_t)Ports[1] << 8);

  /*
    Switch ferme vers GND:
      GPIO = LOW

    On inverse donc:
      LOW -> bit RCUL = 1
  */

  Value = (uint16_t)~Raw;

  return true;
}

// ============================================================================
// Initialisation MCP23017
// ============================================================================

static bool mcp23017Begin(void)
{
  /*
    Force le mode BANK=0 et SEQOP=0:
      registres A/B consecutifs,
      lecture GPIOA puis GPIOB en une transaction.
  */

  if(!mcp23017WriteRegister(
      MCP23017_IOCON,
      0x00))
  {
    return false;
  }

  // GPA0..GPA7 en entrees
  if(!mcp23017WriteRegister(
      MCP23017_IODIRA,
      0xFF))
  {
    return false;
  }

  // GPB0..GPB7 en entrees
  if(!mcp23017WriteRegister(
      MCP23017_IODIRB,
      0xFF))
  {
    return false;
  }

  // Pull-up internes GPA
  if(!mcp23017WriteRegister(
      MCP23017_GPPUA,
      0xFF))
  {
    return false;
  }

  // Pull-up internes GPB
  if(!mcp23017WriteRegister(
      MCP23017_GPPUB,
      0xFF))
  {
    return false;
  }

  if(!mcp23017ReadContacts(Contacts))
  {
    return false;
  }

  CandidateContacts = Contacts;
  CandidateSinceMs = millis();

  return true;
}

// ============================================================================
// Debounce SW1..SW16
// ============================================================================

static void updateContacts(void)
{
  uint16_t NewContacts;

  if(!mcp23017ReadContacts(NewContacts))
  {
    Mcp23017Ready = false;
    return;
  }

  Mcp23017Ready = true;

  if(NewContacts != CandidateContacts)
  {
    CandidateContacts = NewContacts;
    CandidateSinceMs = millis();

    return;
  }

  if((Contacts != CandidateContacts) &&
     ((uint32_t)(millis() - CandidateSinceMs)
        >= SWITCH_DEBOUNCE_MS))
  {
    Contacts = CandidateContacts;
  }
}

// ============================================================================
// Affichage initial SW16
// ============================================================================

static void printContacts(void)
{
  Serial.print(F("SW16 initial : 0b"));

  for(int8_t Bit = 15; Bit >= 0; Bit--)
  {
    Serial.print(
        (Contacts >> Bit) & 0x01);
  }

  Serial.print(F("  0x"));

  if(Contacts < 0x1000) Serial.print('0');
  if(Contacts < 0x0100) Serial.print('0');
  if(Contacts < 0x0010) Serial.print('0');

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
  Serial.println(F("RCUL SW16 REAL MESSAGE TEST"));
  Serial.println(F("ESP32-S3 + MCP4661 POT1 + MCP23017"));
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
  // MCP23017
  // --------------------------------------------------------------------------

  if(!mcp23017Begin())
  {
    Serial.println(
        F("*** MCP23017 0x24 NOT DETECTED ***"));

    while(1)
    {
      delay(1000);
    }
  }

  Mcp23017Ready = true;

  // --------------------------------------------------------------------------
  // Informations
  // --------------------------------------------------------------------------

  RculI2cPotTx.printInfo(Serial);

  Serial.println();

  Serial.print(F("MCP23017 address : 0x"));
  Serial.println(MCP23017_ADDRESS, HEX);

  Serial.println(F("GPA0=SW1 ... GPA7=SW8"));
  Serial.println(F("GPB0=SW9 ... GPB7=SW16"));
  Serial.println(F("Closed switch = ON"));

  Serial.print(F("PTR-6A phase     : "));
  Serial.print(
      RculI2cPotTx.getCallbackSyncOffsetUs());
  Serial.println(F(" us"));

  Serial.print(F("RCUL_REPEAT      : "));
  Serial.println(RCUL_REPEAT);

  Serial.println(
      F("SW16 payload     : 2 bytes / 4 nibbles"));

  Serial.println(
      F("Checksum         : ON"));

  Serial.println(
      F("Expected MLEN    : 6"));

  printContacts();

  Serial.println();
  Serial.println(
      F("XanySpyV7: normal message decoding"));

  Serial.println(
      F("Toggle SW1..SW16 on MCP23017"));

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
    Meme ordre que la version PCF8574A validee a 100 %.

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
  // Lecture des 16 contacts MCP23017
  // --------------------------------------------------------------------------

  updateContacts();

  // --------------------------------------------------------------------------
  // Re-poll apres transaction I2C MCP23017
  // --------------------------------------------------------------------------

  RculI2cPotTx.process();

  // --------------------------------------------------------------------------
  // Charge un nouveau message SW16 lorsque RcTxSerial est libre
  // --------------------------------------------------------------------------

  if(Mcp23017Ready &&
     MyRcTxSerial.isReadyForTx())
  {
    /*
      Payload SW16:

        Message[0]:
          bit 0 = SW9
          ...
          bit 7 = SW16

        Message[1]:
          bit 0 = SW1
          ...
          bit 7 = SW8

      16 bits = 4 nibbles.

      Le dernier argument = 1:
        RcTxSerial ajoute le checksum.
    */

    uint8_t Message[2];

    Message[0] =
        (uint8_t)(Contacts >> 8);

    Message[1] =
        (uint8_t)(Contacts);

    MyRcTxSerial.sendNibbleMsg(
        Message,
        4,
        1);
  }

  // --------------------------------------------------------------------------
  // Traitement immediat du message nouvellement charge
  // --------------------------------------------------------------------------

  RcTxSerial::process();

  RculI2cPotTx.process();
}
