#include <Arduino.h>
#include <Wire.h>
#include <ESP32_PPM.h>
#include <RcTxSerial.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

/*
  ESP32 + AD5282 double sortie RCUL

  AD5282 RDAC0 :
    MCP23017 + PROP1
    format SW16+PROP

  AD5282 RDAC1 :
    ANGLE analogique + PROP2
    format ANGLE+PROP

  Les deux RDAC utilisent :
    - la même adresse I2C 0x2C ;
    - la même entrée PPM Cppm32 ;
    - deux objets RculI2cPotTx indépendants.
*/

// ============================================================================
// Brochage
// ============================================================================
#define SDA_PIN                  5
#define SCL_PIN                  6
#define PPM_INPUT_PIN            4

#define PROP1_PIN                0
#define ANGLE_PIN                1
#define PROP2_PIN                2

// ============================================================================
// I2C
// ============================================================================
#define AD5282_ADDRESS           0x2C
#define MCP23017_ADDRESS         0x20
#define I2C_FREQUENCY            100000UL

// ============================================================================
// Entrée PPM
// ============================================================================
#define PPM_CHANNEL_NB           8
#define PPM_SYNC_MIN_US          3500

// ============================================================================
// RcTxSerial
// ============================================================================
#define RCUL_REPEAT_POT0         5
#define RCUL_REPEAT_POT1         5

#define RCUL_CHANNEL_POT0        8
#define RCUL_CHANNEL_POT1        9

#define RCUL_FIFO_SIZE           8

// ============================================================================
// Entrées analogiques
// ============================================================================
#define ADC_MAX                  4095UL

// ============================================================================
// MCP23017
// ============================================================================
#define MCP23017_IODIRA          0x00
#define MCP23017_IODIRB          0x01
#define MCP23017_GPPUA           0x0C
#define MCP23017_GPPUB           0x0D
#define MCP23017_GPIOA           0x12

#define SWITCH_DEBOUNCE_MS       20UL

// ============================================================================
// AD5282
// ============================================================================

/*
  RDAC0 : SW16+PROP
*/
static RculI2cPotTxClass RculI2cPotTx0(
    AD5282,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_0
);

/*
  RDAC1 : ANGLE+PROP
*/
static RculI2cPotTxClass RculI2cPotTx1(
    AD5282,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_1
);

static RcTxSerial RcTxPot0(
    &RculI2cPotTx0,
    RCUL_REPEAT_POT0,
    RCUL_FIFO_SIZE,
    RCUL_CHANNEL_POT0
);

static RcTxSerial RcTxPot1(
    &RculI2cPotTx1,
    RCUL_REPEAT_POT1,
    RCUL_FIFO_SIZE,
    RCUL_CHANNEL_POT1
);

// ============================================================================
// Variables
// ============================================================================
static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;

static uint8_t Prop1 = 0;
static uint16_t Angle = 0;
static uint8_t Prop2 = 0;

static bool Pot0Ready = false;
static bool Pot1Ready = false;
static bool Mcp23017Ready = false;

// ============================================================================
// MCP23017
// ============================================================================
static bool mcp23017WriteRegister(
    uint8_t Register,
    uint8_t Value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(Register);
  Wire.write(Value);

  return Wire.endTransmission() == 0;
}

static bool mcp23017ReadContacts(uint16_t &Value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_GPIOA);

  if(Wire.endTransmission(false) != 0)
  {
    return false;
  }

  const uint8_t Count =
      Wire.requestFrom(
          (uint8_t)MCP23017_ADDRESS,
          (uint8_t)2);

  if((Count != 2U) || (Wire.available() < 2))
  {
    return false;
  }

  const uint8_t GpioA = Wire.read();
  const uint8_t GpioB = Wire.read();

  /*
    Pull-up actif :
      bouton relâché = 1
      bouton appuyé  = 0

    Pour le message RCUL :
      bouton relâché = 0
      bouton appuyé  = 1
  */
  Value = (uint16_t)~(
      (uint16_t)GpioA |
      ((uint16_t)GpioB << 8));

  return true;
}

static bool mcp23017Begin(void)
{
  Wire.beginTransmission(MCP23017_ADDRESS);

  if(Wire.endTransmission() != 0)
  {
    return false;
  }

  return
      mcp23017WriteRegister(MCP23017_IODIRA, 0xFF) &&
      mcp23017WriteRegister(MCP23017_IODIRB, 0xFF) &&
      mcp23017WriteRegister(MCP23017_GPPUA,  0xFF) &&
      mcp23017WriteRegister(MCP23017_GPPUB,  0xFF);
}

static void updateContacts(void)
{
  uint16_t NewContacts;

  if(!mcp23017ReadContacts(NewContacts))
  {
    Mcp23017Ready = false;
    return;
  }

  Mcp23017Ready = true;

  const uint32_t NowMs = millis();

  if(NewContacts != CandidateContacts)
  {
    CandidateContacts = NewContacts;
    CandidateSinceMs = NowMs;
  }
  else if((Contacts != CandidateContacts) &&
          ((uint32_t)(NowMs - CandidateSinceMs) >=
           SWITCH_DEBOUNCE_MS))
  {
    Contacts = CandidateContacts;
  }
}

// ============================================================================
// Entrées analogiques
// ============================================================================
static uint8_t readProp(uint8_t Pin)
{
  const uint32_t Raw =
      (uint32_t)analogRead(Pin);

  return (uint8_t)(
      (Raw * 255UL + (ADC_MAX / 2UL)) /
      ADC_MAX);
}

static void updateAnalogInputs(void)
{
  Prop1 = readProp(PROP1_PIN);

  const uint32_t RawAngle =
      (uint32_t)analogRead(ANGLE_PIN);

  Angle = (uint16_t)(
      (RawAngle * 4095UL + (ADC_MAX / 2UL)) /
      ADC_MAX);

  Prop2 = readProp(PROP2_PIN);
}

// ============================================================================
// Construction des messages
// ============================================================================

/*
  SW16+PROP :
    8 bits PROP1 + 16 bits contacts
    24 bits = 6 nibbles
*/
static void buildSw16PropMessage(uint8_t *Message)
{
  Message[0] = Prop1;
  Message[1] = highByte(Contacts);
  Message[2] = lowByte(Contacts);
}

/*
  ANGLE+PROP :
    12 bits ANGLE + 8 bits PROP2
    20 bits = 5 nibbles
*/
static void buildAnglePropMessage(uint8_t *Message)
{
  Message[0] =
      (uint8_t)((Angle & 0x0FF0U) >> 4);

  Message[1] =
      (uint8_t)(
          ((Angle & 0x000FU) << 4) |
          ((Prop2 & 0xF0U) >> 4));

  Message[2] =
      (uint8_t)((Prop2 & 0x0FU) << 4);
}

// ============================================================================
// Debug
// ============================================================================
static void displayStatus(void)
{
  static uint32_t LastDisplayMs = 0;

  if((uint32_t)(millis() - LastDisplayMs) < 1000UL)
  {
    return;
  }

  LastDisplayMs = millis();

  Serial.print(F("[RDAC0] W="));
  Serial.print(RculI2cPotTx0.getWiper());

  Serial.print(F(" SW16=0x"));
  if(Contacts < 0x1000) Serial.print('0');
  if(Contacts < 0x0100) Serial.print('0');
  if(Contacts < 0x0010) Serial.print('0');
  Serial.print(Contacts, HEX);

  Serial.print(F(" PROP1="));
  Serial.print(Prop1);

  Serial.print(F(" | [RDAC1] W="));
  Serial.print(RculI2cPotTx1.getWiper());

  Serial.print(F(" ANGLE="));
  Serial.print(Angle);

  Serial.print(F(" PROP2="));
  Serial.println(Prop2);
}

// ============================================================================
// Setup
// ============================================================================
void setup()
{
  Serial.begin(115200);
  delay(300);

  /*
    Le bus I2C est initialisé une seule fois.
  */
  Wire.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY);

  analogReadResolution(12);

  analogSetPinAttenuation(
      PROP1_PIN,
      ADC_11db);

  analogSetPinAttenuation(
      ANGLE_PIN,
      ADC_11db);

  analogSetPinAttenuation(
      PROP2_PIN,
      ADC_11db);

  pinMode(PROP1_PIN, INPUT);
  pinMode(ANGLE_PIN, INPUT);
  pinMode(PROP2_PIN, INPUT);

  Cppm32.beginRx(
      PPM_INPUT_PIN,
      PPM_CHANNEL_NB,   // 8 voies
      true,             // impulsions PPM positives
                        // false = impulsions négatives
      PPM_SYNC_MIN_US,  // 3500 us
      800,              // largeur mini voie
      2200);            // largeur maxi voie

  /*
    Les deux objets utilisent le même bus et la même adresse I2C,
    mais sélectionnent chacun leur RDAC interne.
  */
  Pot0Ready = RculI2cPotTx0.begin(
      &Wire,
      AD5282_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  Pot1Ready = RculI2cPotTx1.begin(
      &Wire,
      AD5282_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  Mcp23017Ready = mcp23017Begin();

  if(Mcp23017Ready)
  {
    uint16_t InitialContacts;

    if(mcp23017ReadContacts(InitialContacts))
    {
      Contacts = InitialContacts;
      CandidateContacts = InitialContacts;
      CandidateSinceMs = millis();
    }
  }

  updateAnalogInputs();

  Serial.println();
  Serial.println(F("AD5282 double sortie RCUL"));
  Serial.println(F("--------------------------"));

  Serial.print(F("RDAC0 SW16+PROP : "));
  Serial.println(Pot0Ready ? F("OK") : F("ERROR"));

  Serial.print(F("RDAC1 ANGLE+PROP: "));
  Serial.println(Pot1Ready ? F("OK") : F("ERROR"));

  Serial.print(F("MCP23017        : "));
  Serial.println(Mcp23017Ready ? F("OK") : F("ERROR"));

  RculI2cPotTx0.printInfo(Serial);
  RculI2cPotTx1.printInfo(Serial);
}

// ============================================================================
// Loop
// ============================================================================
void loop()
{
  if(!Pot0Ready || !Pot1Ready)
  {
    return;
  }

  updateContacts();
  updateAnalogInputs();

  /*
    En mode PPMIN, process() ne fabrique pas le top.
    Chaque objet récupère directement sa synchronisation depuis Cppm32.
  */
  RculI2cPotTx0.process();
  RculI2cPotTx1.process();

  if(RcTxPot0.isReadyForTx())
  {
    uint8_t Message[3];

    buildSw16PropMessage(Message);

    RcTxPot0.sendNibbleMsg(
        Message,
        6,
        1);
  }

  if(RcTxPot1.isReadyForTx())
  {
    uint8_t Message[3];

    buildAnglePropMessage(Message);

    RcTxPot1.sendNibbleMsg(
        Message,
        5,
        1);
  }

  RcTxSerial::process();
  displayStatus();
}
