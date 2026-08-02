#include <Wire.h>
#include <ESP32_PPM.h>
#include <RculPWMRead.h>
#include <RcTxSerial.h>

#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

// -----------------------------------------------------------------------------
// Pins
// -----------------------------------------------------------------------------
#define SDA_PIN          5
#define SCL_PIN          6
#define PPM_INPUT_PIN    4

#define PROP1_PIN        0
#define ANGLE_PIN        1
#define PROP2_PIN        2

#define PWM_CAL_POT0_PIN 7
#define PWM_CAL_POT1_PIN 8

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------
#define AD5282_ADDRESS   0x2C
#define MCP23017_ADDRESS 0x20

// -----------------------------------------------------------------------------
// PPM / RCUL
// -----------------------------------------------------------------------------
#define PPM_CHANNEL_NB   8
#define PPM_SYNC_MIN_US  3500

#define RCUL_CH_POT0     8
#define RCUL_CH_POT1     9
#define RCUL_REPEAT      5
#define RCUL_FIFO_SIZE   8

// -----------------------------------------------------------------------------
// MCP23017 registers
// -----------------------------------------------------------------------------
#define IODIRA 0x00
#define IODIRB 0x01
#define GPPUA  0x0C
#define GPPUB  0x0D
#define GPIOA  0x12

// -----------------------------------------------------------------------------
// AD5282 outputs
// -----------------------------------------------------------------------------
static RculI2cPotTxClass Pot0(
    AD5282,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_0
);

static RculI2cPotTxClass Pot1(
    AD5282,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_1
);

static RcTxSerial Tx0(
    &Pot0,
    RCUL_REPEAT,
    RCUL_FIFO_SIZE,
    RCUL_CH_POT0
);

static RcTxSerial Tx1(
    &Pot1,
    RCUL_REPEAT,
    RCUL_FIFO_SIZE,
    RCUL_CH_POT1
);

// -----------------------------------------------------------------------------
// PWM feedback used only during automatic calibration
// -----------------------------------------------------------------------------
static RculPWMRead PwmCal0;
static RculPWMRead PwmCal1;

enum CalibrationState
{
  CAL_IDLE,
  CAL_POT0,
  CAL_POT1
};

static CalibrationState CalState = CAL_IDLE;

// -----------------------------------------------------------------------------
// Input values
// -----------------------------------------------------------------------------
static uint16_t Contacts = 0;
static uint8_t Prop1 = 0;
static uint16_t Angle = 0;
static uint8_t Prop2 = 0;

// -----------------------------------------------------------------------------
// MCP23017
// -----------------------------------------------------------------------------
static bool writeMcp(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool beginMcp23017()
{
  return writeMcp(IODIRA, 0xFF) &&
         writeMcp(IODIRB, 0xFF) &&
         writeMcp(GPPUA,  0xFF) &&
         writeMcp(GPPUB,  0xFF);
}

static bool readContacts()
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(GPIOA);

  if (Wire.endTransmission(false) != 0)
    return false;

  if (Wire.requestFrom((uint8_t)MCP23017_ADDRESS,
                       (uint8_t)2) != 2)
    return false;

  const uint8_t a = Wire.read();
  const uint8_t b = Wire.read();

  Contacts = (uint16_t)~((uint16_t)a | ((uint16_t)b << 8));
  return true;
}

// -----------------------------------------------------------------------------
// Analog inputs
// -----------------------------------------------------------------------------
static void readAnalogInputs()
{
  Prop1 = (uint8_t)(((uint32_t)analogRead(PROP1_PIN) * 255UL) / 4095UL);
  Angle = (uint16_t)analogRead(ANGLE_PIN);
  Prop2 = (uint8_t)(((uint32_t)analogRead(PROP2_PIN) * 255UL) / 4095UL);
}

// -----------------------------------------------------------------------------
// Payloads
// -----------------------------------------------------------------------------
static void buildSw16Prop(uint8_t *msg)
{
  msg[0] = Prop1;
  msg[1] = highByte(Contacts);
  msg[2] = lowByte(Contacts);
}

static void buildAngleProp(uint8_t *msg)
{
  msg[0] = (Angle >> 4) & 0xFF;
  msg[1] = ((Angle & 0x0F) << 4) | (Prop2 >> 4);
  msg[2] = (Prop2 & 0x0F) << 4;
}

// -----------------------------------------------------------------------------
// Automatic calibration
// -----------------------------------------------------------------------------
static void startAutoCalibration()
{
  if (CalState != CAL_IDLE)
    return;

  Serial.println(F("Calibration RDAC0..."));

  if (Pot0.startRculTableCalibration(
          PwmCal0,
          &Serial,
          5,
          2,
          4))
  {
    CalState = CAL_POT0;
  }
  else
  {
    Serial.println(F("Erreur demarrage calibration RDAC0"));
  }
}

static void processAutoCalibration()
{
  if (CalState == CAL_POT0)
  {
    Pot0.processRculTableCalibration();

    if (!Pot0.isRculTableCalibrationActive())
    {
      if (Pot0.didRculTableCalibrationFail())
      {
        Serial.println(F("Calibration RDAC0 echouee"));
        CalState = CAL_IDLE;
        return;
      }

      Serial.println(F("Calibration RDAC1..."));

      if (Pot1.startRculTableCalibration(
              PwmCal1,
              &Serial,
              6,
              2,
              4))
      {
        CalState = CAL_POT1;
      }
      else
      {
        Serial.println(F("Erreur demarrage calibration RDAC1"));
        CalState = CAL_IDLE;
      }
    }
  }
  else if (CalState == CAL_POT1)
  {
    Pot1.processRculTableCalibration();

    if (!Pot1.isRculTableCalibrationActive())
    {
      if (Pot1.didRculTableCalibrationFail())
        Serial.println(F("Calibration RDAC1 echouee"));
      else
        Serial.println(F("Calibration AD5282 terminee"));

      CalState = CAL_IDLE;
    }
  }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(300);

  Wire.begin(SDA_PIN, SCL_PIN, 100000UL);

  analogReadResolution(12);
  analogSetPinAttenuation(PROP1_PIN, ADC_11db);
  analogSetPinAttenuation(ANGLE_PIN, ADC_11db);
  analogSetPinAttenuation(PROP2_PIN, ADC_11db);

  Cppm32.beginRx(
      PPM_INPUT_PIN,
      PPM_CHANNEL_NB,
      true,
      PPM_SYNC_MIN_US,
      800,
      2200);

  if (PwmCal0.attach(PWM_CAL_POT0_PIN, 700, 2300) < 0 ||
      PwmCal1.attach(PWM_CAL_POT1_PIN, 700, 2300) < 0)
  {
    Serial.println(F("Erreur entrees PWM calibration"));
    while (1);
  }

  if (!Pot0.begin(&Wire, AD5282_ADDRESS, 18, 1500) ||
      !Pot1.begin(&Wire, AD5282_ADDRESS, 18, 1500))
  {
    Serial.println(F("AD5282 non detecte"));
    while (1);
  }

  Pot0.setTableStoragePreferences("RculAD0");
  Pot1.setTableStoragePreferences("RculAD1");

  Pot0.useStoredRculTable();
  Pot1.useStoredRculTable();

  if (!beginMcp23017())
  {
    Serial.println(F("MCP23017 non detecte"));
    while (1);
  }

  Serial.println(F("Commande c : calibration automatique des 2 RDAC"));
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop()
{
  if (Serial.available() && Serial.read() == 'c')
    startAutoCalibration();

  if (CalState != CAL_IDLE)
  {
    processAutoCalibration();
    return;
  }

  readContacts();
  readAnalogInputs();

  Pot0.process();
  Pot1.process();

  if (Tx0.isReadyForTx())
  {
    uint8_t msg[3];
    buildSw16Prop(msg);
    Tx0.sendNibbleMsg(msg, 6, 1);
  }

  if (Tx1.isReadyForTx())
  {
    uint8_t msg[3];
    buildAngleProp(msg);
    Tx1.sendNibbleMsg(msg, 5, 1);
  }

  RcTxSerial::process();
}
