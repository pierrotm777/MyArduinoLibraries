#include <Arduino.h>
#include <TinyWireM.h>
// Select the I2C digital potentiometer used by this example.
// Choose MCP4561 or DS3502.
#define RCUL_I2C_POT_DEVICE DS3502

// Keep the original global RculI2cPotTx object name while selecting its driver.
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
static RculI2cPotTxClass RculI2cPotTx(RCUL_I2C_POT_DEVICE);
#include <RcTxSerial.h>

/*
  RculI2cPotTx - ATtiny85 + PCF8574 + SW8 + PROP

  I2C master driver:
    TinyWireM (ATtiny85 USI peripheral)

  Hardware:
    - ATtiny85
    - MCP4561 or DS3502 10 kOhm on the I2C bus
    - PCF8574 on the same I2C bus for SW1..SW8
    - Analog potentiometer for PROP
    - Status LED on PB1

  RCUL payload order:
    Byte[0] = PROP
    Byte[1] = SW8

  Payload = 4 nibbles, therefore MLEN=6.

  ATtiny85 pin use:
    PB0 / Arduino pin 0 / physical pin 5 : SDA
    PB1 / Arduino pin 1 / physical pin 6 : status LED
    PB2 / Arduino pin 2 / physical pin 7 : SCL
    PB3 / A3            / physical pin 2 : PROP analog input

  LED behavior:
    blinking : digital potentiometer or PCF8574 is not ready
    off      : both I2C devices are ready

  The RESET pin is preserved.
*/

// ============================================================================
// ATtiny85 pins
// ============================================================================
#define SDA_PIN  0      // PB0, physical pin 5 - fixed TinyWireM SDA
#define LED_PIN  1      // PB1, physical pin 6
#define SCL_PIN  2      // PB2, physical pin 7 - fixed TinyWireM SCL
#define PROP_PIN A3     // PB3, physical pin 2

/* TinyWireM normally runs the ATtiny85 USI bus at 100 kHz.
   The argument is retained for compatibility with the common begin() API. */
#define I2C_FREQUENCY RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY

// ============================================================================
// DIGITAL POTENTIOMETER
// ============================================================================
#define DIGIPOT_ADDRESS RCUL_I2C_POT_TX_AUTO_ADDRESS  // automatic: MCP4561=0x2C, DS3502=0x28

// ============================================================================
// PCF8574
// ============================================================================
/*
  PCF8574 address selected by A2/A1/A0:
    000 -> 0x20
    001 -> 0x21
    010 -> 0x22
    011 -> 0x23
    100 -> 0x24
    101 -> 0x25
    110 -> 0x26
    111 -> 0x27

  PCF8574A normally uses 0x38..0x3F.

  Wiring:
    P0 ---- SW1 ---- GND
    ...
    P7 ---- SW8 ---- GND

  Closed switch = logical 1 in the RCUL message.
*/
#define PCF8574_ADDRESS    0x20
#define SWITCH_DEBOUNCE_MS 20UL
#define LED_BLINK_MS       500UL

// ============================================================================
// PROP analog input
// ============================================================================
/*
  Potentiometer wiring:
    one end -> VCC
    wiper   -> PB3 / A3
    other end -> GND
*/
#define PROP_ADC_MAX 1023UL

// ============================================================================
// RCUL
// ============================================================================
#define RCUL_REPEAT  5
#define RCUL_CHANNEL 8

static RcTxSerial MyRcTxSerial(&RculI2cPotTx,
                               RCUL_REPEAT,
                               8,
                               RCUL_CHANNEL);

static uint8_t Contacts = 0;
static uint8_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint8_t Prop = 0;
static bool HardwareReady = false;

// ============================================================================
// PCF8574 low-level access through TinyWireM
// ============================================================================
static bool pcf8574Write(uint8_t Value)
{
  TinyWireM.beginTransmission(PCF8574_ADDRESS);
  TinyWireM.send(Value);
  return (TinyWireM.endTransmission() == 0);
}

static bool pcf8574Read(uint8_t &Value)
{
  const uint8_t Count = TinyWireM.requestFrom((uint8_t)PCF8574_ADDRESS,
                                               (uint8_t)1);

  if((Count != 1) || !TinyWireM.available())
  {
    return false;
  }

  Value = TinyWireM.receive();
  return true;
}

static bool pcf8574Begin(void)
{
  // A written 1 releases each quasi-bidirectional pin for input operation.
  if(!pcf8574Write(0xFF))
  {
    return false;
  }

  uint8_t Raw;
  if(!pcf8574Read(Raw))
  {
    return false;
  }

  Contacts = (uint8_t)~Raw;
  CandidateContacts = Contacts;
  CandidateSinceMs = millis();
  return true;
}

// ============================================================================
// Status LED
// ============================================================================
static void updateStatusLed(void)
{
  static uint32_t PreviousBlinkMs = 0;
  static bool LedState = false;

  if(HardwareReady)
  {
    LedState = false;
    digitalWrite(LED_PIN, LOW);   // all OK: LED off
    return;
  }

  if((uint32_t)(millis() - PreviousBlinkMs) >= LED_BLINK_MS)
  {
    PreviousBlinkMs = millis();
    LedState = !LedState;
    digitalWrite(LED_PIN, LedState ? HIGH : LOW);
  }
}

// ============================================================================
// Inputs
// ============================================================================
static void updateContacts(void)
{
  uint8_t Raw;
  if(!pcf8574Read(Raw))
  {
    return;
  }

  const uint8_t NewContacts = (uint8_t)~Raw;

  if(NewContacts != CandidateContacts)
  {
    CandidateContacts = NewContacts;
    CandidateSinceMs = millis();
    return;
  }

  if((Contacts != CandidateContacts) &&
     ((uint32_t)(millis() - CandidateSinceMs) >= SWITCH_DEBOUNCE_MS))
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
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);    // not ready during initialization
  pinMode(PROP_PIN, INPUT);

  // Optional digital-potentiometer range. Defaults: MCP4561=4..252, DS3502=2..125.
  // RculI2cPotTx.setWiperRange(0, RculI2cPotTx.getMaxWiper()); // full range
  // RculI2cPotTx.setWiperRange(10, 245);  // reduced range

  /* On ATtiny85, RculI2cPotTx.begin() initializes TinyWireM.
     SDA and SCL are fixed by the USI hardware: PB0 and PB2. */
  const bool PotReady = RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
      1500);

  const bool SwitchesReady = pcf8574Begin();

  updateProp();
  HardwareReady = PotReady && SwitchesReady;
  updateStatusLed();
}

void loop()
{
  updateStatusLed();

  if(!HardwareReady)
  {
    return;
  }

  updateContacts();
  updateProp();

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
