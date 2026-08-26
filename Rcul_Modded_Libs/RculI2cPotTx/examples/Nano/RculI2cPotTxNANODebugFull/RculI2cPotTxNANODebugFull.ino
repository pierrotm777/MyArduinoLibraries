
/*
Use ONLY Arduino Nano(Old bootloader)
*/

#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <elapsedMillis.h>
#include <SoftRcPulseIn.h>

// ============================================================================
// Leds RGB
// ============================================================================
CRGB leds_Synchro[3];
uint32_t SYNCHRO_COLOR = CRGB::Red;
uint8_t  setRGBBrightness = 10;
uint16_t setBrightness = 255;

bool ledRGBSate   = false;
elapsedMillis ledRGBInfo;
// ============================================================================

bool consoleMode = false;
bool DebugMode   = false;
uint8_t DbgVersion = 0;
elapsedMillis printMs;

#define PRINT_BUF_SIZE      100
static char PrintBuf[PRINT_BUF_SIZE + 1];
#define PRINTF(fmt, ...)                                   \
  do {                                                     \
    if (Serial) {                                          \
      snprintf(PrintBuf, PRINT_BUF_SIZE, fmt, ##__VA_ARGS__); \
      Serial.print(PrintBuf);                              \
    }                                                      \
  } while (0)


// ============================================================================
// PWM Input for calibration only
// ============================================================================
#define PWM_RX_PIN  8
static SoftRcPulseIn ReceiverPwm;

// ============================================================================
// I2C bus
// ============================================================================

#define SDA_PIN A4
#define SCL_PIN A5

#define I2C_FREQUENCY RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY

// ============================================================================
// Main Payload configuration
// ============================================================================
#define ANGLE 0
#define PROP  0
#define SW_NB 8

// ============================================================================
// Optional analog inputs
// ============================================================================
#if PROP
  #define PROP_PIN      A0
  #define PROP_ADC_MIN  0UL
  #define PROP_ADC_MAX  1023UL
#endif

#if ANGLE
  #define ANGLE_PIN      A1
  #define ANGLE_ADC_MIN  0UL
  #define ANGLE_ADC_MAX  1023UL
#endif

// ============================================================================
// Input expander selection
// ============================================================================
#define INPUT_EXPANDER_PCF8574A  1
#define INPUT_EXPANDER_MCP23017  2

// Select ONE expander:
#define INPUT_EXPANDER INPUT_EXPANDER_PCF8574A
//#define INPUT_EXPANDER INPUT_EXPANDER_MCP23017

#if INPUT_EXPANDER == INPUT_EXPANDER_PCF8574A
  #define INPUT_EXPANDER_ADDRESS 0x38
  #define INPUT_EXPANDER_NAME    "PCF8574A"
#elif INPUT_EXPANDER == INPUT_EXPANDER_MCP23017
  #define INPUT_EXPANDER_ADDRESS 0x24
  #define INPUT_EXPANDER_NAME    "MCP23017"
#else
  #error "Invalid INPUT_EXPANDER selection"
#endif

static bool inputExpanderReady = false;
#define SWITCH_DEBOUNCE_MS 20UL

// ============================================================================
// RCUL payload
// ============================================================================
#define ANGLE_BIT_NB_MAX    12
#define PROP_BIT_NB_MAX      8
#define CONTACT_BIT_NB_MAX  16
#define CHKS_BIT_NB_MAX      8
#define TX_MSG_BIT_NB_MAX   (ANGLE_BIT_NB_MAX + PROP_BIT_NB_MAX + CONTACT_BIT_NB_MAX + CHKS_BIT_NB_MAX)
#define TX_MSG_BYTE_NB_MAX  ((TX_MSG_BIT_NB_MAX + 7) / 8)

typedef struct {
  uint8_t Byte[TX_MSG_BYTE_NB_MAX];
} TxMsgSt_t;

typedef struct {
  uint8_t Angle     : 1;
  uint8_t Prop      : 1;
  uint8_t ContactNb : 6;
} PayloadDescSt_t;

static TxMsgSt_t MyTxMsg;
static PayloadDescSt_t PayloadDesc;

static uint16_t Angle = 0;
static uint8_t  Prop = 0;
static uint32_t RawAngle = 0;
static uint32_t RawProp = 0;
static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint32_t LastDebugMs = 0;

// ============================================================================
// DIGITAL POTENTIOMETER
// ============================================================================
/* RCUL_I2C_POT_TX_AUTO_ADDRESS selects the correct default automatically:
     POT : 0x2C
     DS3502  : 0x28 when A1=A0=0 (0x28..0x2B possible).
   A different valid address can be forced if required. */

// Select the I2C digital potentiometer used by this example.
// Choose MCP4561 or DS3502.
#define RCUL_I2C_POT_DEVICE DS3502
// Prevents the header from declaring the default global instance.
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE // doit impérativement être définie avant #include <RculI2cPotTx.h>
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>


static RculI2cPotTxClass RculI2cPotTx(RCUL_I2C_POT_DEVICE);

static uint8_t POT_I2C_ADDRESS = 0;
static bool potIsReady = false;

uint8_t RCUL_REPEAT = 4;
#define RCUL_CHANNEL 3
static RcTxSerial MyRcTxSerial(
  &RculI2cPotTx, 
  RCUL_REPEAT, 8
  /*, RCUL_CHANNEL*/
);

// ============================================================================
// Prototypes
// ============================================================================
static void BuildValueMsg(void);
static void LoadAngle(uint16_t angle);
static void LoadProp(uint8_t prop);
static void LoadContacts(uint16_t contacts);
static uint8_t NibbleNbToSend(void);
static void updateContacts(void);
static void updateAngle(void);
static void updateProp(void);
static void printContactsBits(uint16_t value, uint8_t bitCount);
static void displayStatus(void);

// ============================================================================
// Generic I2C helpers
// ============================================================================
static bool i2cProbe(uint8_t address)
{
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

static uint8_t findPOTAddress(void)
{
  // Dedicated candidate list: never return the OLED or the GPIO expander.
  static const uint8_t addresses[] = { 0x2E, 0x2F, //MCP4561
									          0x28, 0x29, 0x2A, 0x2B //DS3502
									        };
  for (uint8_t i = 0; i < sizeof(addresses); ++i) {
    if (i2cProbe(addresses[i])) return addresses[i];
  }
  return 0;
}

static uint8_t findOledAddress(void)
{
  static const uint8_t addresses[] = {0x3C, 0x3D};
  for (uint8_t i = 0; i < sizeof(addresses); ++i) {
    if (i2cProbe(addresses[i])) return addresses[i];
  }
  return 0;
}

// ============================================================================
// PCF8574A
// ============================================================================
#if INPUT_EXPANDER == INPUT_EXPANDER_PCF8574A
static bool pcf8574Write(uint8_t value)
{
  Wire.beginTransmission(INPUT_EXPANDER_ADDRESS);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool pcf8574Read(uint16_t &value)
{
  const uint8_t count = Wire.requestFrom((uint8_t)INPUT_EXPANDER_ADDRESS, (uint8_t)1);
  if (count != 1 || !Wire.available()) return false;
  value = (uint8_t)Wire.read();
  return true;
}
#endif

// ============================================================================
// MCP23017 (BANK=0 register map)
// ============================================================================
#if INPUT_EXPANDER == INPUT_EXPANDER_MCP23017
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPPUA  0x0C
#define MCP23017_GPPUB  0x0D
#define MCP23017_GPIOA  0x12

static bool mcp23017WriteRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(INPUT_EXPANDER_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool mcp23017Read(uint16_t &value)
{
  Wire.beginTransmission(INPUT_EXPANDER_ADDRESS);
  Wire.write(MCP23017_GPIOA);
  if (Wire.endTransmission(false) != 0) return false;

  const uint8_t count = Wire.requestFrom((uint8_t)INPUT_EXPANDER_ADDRESS, (uint8_t)2);
  if (count != 2 || Wire.available() < 2) return false;

  const uint8_t gpioA = Wire.read();
  const uint8_t gpioB = Wire.read();
  value = (uint16_t)gpioA | ((uint16_t)gpioB << 8);
  return true;
}
#endif

// ============================================================================
// Input expander abstraction
// ============================================================================
static bool inputExpanderRead(uint16_t &raw)
{
#if INPUT_EXPANDER == INPUT_EXPANDER_PCF8574A
  return pcf8574Read(raw);
#else
  return mcp23017Read(raw);
#endif
}

static bool inputExpanderBegin(void)
{
  if (!i2cProbe(INPUT_EXPANDER_ADDRESS)) return false;

#if INPUT_EXPANDER == INPUT_EXPANDER_PCF8574A
  // A '1' releases each quasi-bidirectional pin for input use.
  if (!pcf8574Write(0xFF)) return false;
#else
  // 16 inputs, with internal pull-ups enabled.
  if (!mcp23017WriteRegister(MCP23017_IODIRA, 0xFF)) return false;
  if (!mcp23017WriteRegister(MCP23017_IODIRB, 0xFF)) return false;
  if (!mcp23017WriteRegister(MCP23017_GPPUA,  0xFF)) return false;
  if (!mcp23017WriteRegister(MCP23017_GPPUB,  0xFF)) return false;
#endif

  uint16_t raw = 0;
  if (!inputExpanderRead(raw)) return false;

  const uint16_t mask = (SW_NB == 8) ? 0x00FFU : 0xFFFFU;
  Contacts = (uint16_t)(~raw) & mask;
  CandidateContacts = Contacts;
  CandidateSinceMs = millis();
  return true;
}

static void updateContacts(void)
{
  uint16_t raw = 0;
  if (!inputExpanderRead(raw)) {
    inputExpanderReady = false;
    return;
  }

  inputExpanderReady = true;
  const uint16_t mask = (SW_NB == 8) ? 0x00FFU : 0xFFFFU;
  const uint16_t newContacts = (uint16_t)(~raw) & mask;

  if (newContacts != CandidateContacts) {
    CandidateContacts = newContacts;
    CandidateSinceMs = millis();
    return;
  }

  if (Contacts != CandidateContacts &&
      (uint32_t)(millis() - CandidateSinceMs) >= SWITCH_DEBOUNCE_MS) {
    Contacts = CandidateContacts;
  }
}

/*
  Convert an ADC range to an unsigned output range.
*/
static uint32_t scaleAdcValue(uint32_t raw,
                              uint32_t adcMin,
                              uint32_t adcMax,
                              uint32_t outputMax)
{
  if (raw < adcMin) raw = adcMin;
  if (raw > adcMax) raw = adcMax;

  const uint32_t span = adcMax - adcMin;
  if (span == 0) return 0;

  return ((raw - adcMin) * outputMax + (span / 2UL)) / span;
}

/*
  ANGLE enabled:
    ANGLE_ADC_MIN..ANGLE_ADC_MAX -> 0..4095.

  ANGLE disabled:
    Angle and RawAngle are forced to 0.
*/
static void updateAngle(void)
{
#if ANGLE
  RawAngle = (uint32_t)analogRead(ANGLE_PIN);
  Angle = (uint16_t)scaleAdcValue(
      RawAngle,
      ANGLE_ADC_MIN,
      ANGLE_ADC_MAX,
      4095UL
  );
#else
  RawAngle = 0;
  Angle = 0;
#endif
}

/*
  PROP enabled:
    PROP_ADC_MIN..PROP_ADC_MAX -> 0..255.

  PROP disabled:
    Prop and RawProp are forced to 0.
*/
static void updateProp(void)
{
#if PROP
  RawProp = (uint32_t)analogRead(PROP_PIN);
  Prop = (uint8_t)scaleAdcValue(
      RawProp,
      PROP_ADC_MIN,
      PROP_ADC_MAX,
      255UL
  );
#else
  RawProp = 0;
  Prop = 0;
#endif
}

// ============================================================================
// Setup / loop
// ============================================================================
void setup()
{
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500UL) delay(1);

  Serial.println();
  Serial.println(F("***********************************"));
  Serial.println(F("RculI2cPotTx Nano V3 expander debug"));
  Serial.println(F("***********************************"));

  Wire.begin();

#if ANGLE || PROP
  #if defined(ARDUINO_ARCH_ESP32)
    analogReadResolution(12);
  #endif
#endif

#if ANGLE
  pinMode(ANGLE_PIN, INPUT);
  updateAngle();
#endif

#if PROP
  pinMode(PROP_PIN, INPUT);
  updateProp();
#endif

  PayloadDesc.Angle = ANGLE;
  PayloadDesc.Prop  = PROP;

  /*
    ANGLE mode uses the ANGLE+PROP payload.
    Switch contacts are mutually exclusive with ANGLE.
  */
#if ANGLE
  PayloadDesc.ContactNb = 0;
#else
  PayloadDesc.ContactNb = SW_NB;
#endif

  ReceiverPwm.attach(PWM_RX_PIN, 700, 2300);
  
  POT_I2C_ADDRESS = findPOTAddress();
  if (POT_I2C_ADDRESS != 0 &&
      RculI2cPotTx.begin(SDA_PIN, SCL_PIN, I2C_FREQUENCY,
                         POT_I2C_ADDRESS,
                         RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
                         1500)) {
    potIsReady = true;
  } else {
    Serial.println(F("POT not detected or initial write failed"));
  }

  RculI2cPotTx.setTableStorageEEPROM(0);

  if(!RculI2cPotTx.useStoredRculTable())
  {
    Serial.println(F("No stored table: embedded default selected"));
  }

#if ANGLE
  inputExpanderReady = false;
  Contacts = 0;
  CandidateContacts = 0;
#else
  inputExpanderReady = inputExpanderBegin();

  if (!inputExpanderReady) {
    Serial.print(F(INPUT_EXPANDER_NAME));
    Serial.println(F(" not detected; command C remains available"));
  }
#endif

  BuildValueMsg();

  PRINTF("POT address  : 0x%02X\r\n", potIsReady ? POT_I2C_ADDRESS : 0);
#if ANGLE
  PRINTF("Payload      : ANGLE+PROP, %u nibbles\r\n",
                NibbleNbToSend());
#else
  PRINTF("%s address   : 0x%02X\r\n", INPUT_EXPANDER_NAME,
                inputExpanderReady ? INPUT_EXPANDER_ADDRESS : 0);
  PRINTF("Payload      : SW%u%s, %u nibbles\r\n",
                PayloadDesc.ContactNb,
                PayloadDesc.Prop ? "+PROP" : "",
                NibbleNbToSend());
#endif

  RculI2cPotTx.printInfo(Serial);
  FastLED.addLeds<WS2812B, 2, GRB>(leds_Synchro, 3);
  FastLED.clear();
  FastLED.setBrightness(setRGBBrightness);//0 to 255
  if (potIsReady) {
    if (PayloadDesc.Angle>0) leds_Synchro[0] = CRGB::Green;
    if (PayloadDesc.Prop>0)  leds_Synchro[1] = CRGB::Green;
    if (PayloadDesc.ContactNb>0) leds_Synchro[2] = CRGB::Green;    
  }
  else {
    leds_Synchro[0] = CRGB::Red;
    leds_Synchro[1] = CRGB::Black;
    leds_Synchro[2] = CRGB::Black;
  }
  FastLED.show();
}

void loop()
{
  processUsbConsole();

  if(RculI2cPotTx.isRculTableCalibrationActive())
  {
    /*
      Do not run RcTxSerial while learning: calibration owns the wiper.
    */
    RculI2cPotTx.processRculTableCalibration();
    return;
  }

#if !ANGLE
  updateContacts();
#endif

  updateAngle();
  updateProp();

  RculI2cPotTx.process();

  // if (PayloadDesc.ContactNb > 0 && inputExpanderReady)
  //   leds_Synchro[0] = CRGB::Green;
  // else
  //  blinkRGBLED();

  if (MyRcTxSerial.isReadyForTx()) {
    BuildValueMsg();
    // Send the REAL payload. The previous fixed 0x80 test message has been removed.
    MyRcTxSerial.sendNibbleMsg(MyTxMsg.Byte, NibbleNbToSend(), 1);
  }

  RcTxSerial::process();

  if (DebugMode && (uint32_t)(millis() - LastDebugMs) >= 1000UL) {
    LastDebugMs = millis();

    if (DbgVersion == 1) {
      Serial.print(F("Wiper="));
      Serial.print(RculI2cPotTx.getWiper());
      Serial.print('/');
      Serial.println(RculI2cPotTx.getMaxWiper());
    } else if (DbgVersion == 2) {
      Serial.print(F("[D2] "));

#if ANGLE
      Serial.print(F("ANGLE_RAW="));
      Serial.print(RawAngle);
      Serial.print(F(" ANGLE="));
      Serial.print(Angle);
      Serial.print(F("/4095 "));
#endif

#if PROP
      Serial.print(F("PROP_RAW="));
      Serial.print(RawProp);
      Serial.print(F(" PROP="));
      Serial.print(Prop);
      Serial.print(F("/255 "));
#endif

#if ANGLE
      Serial.println();
#else
      Serial.print(F(INPUT_EXPANDER_NAME));
      Serial.print(F("[0x"));
      if (INPUT_EXPANDER_ADDRESS < 0x10) Serial.print('0');
      Serial.print(INPUT_EXPANDER_ADDRESS, HEX);
      Serial.print(F("]="));

      if (!inputExpanderReady) {
        Serial.println(F("OFF"));
      } else {
        Serial.print(F("ON SW"));
        Serial.print(SW_NB);
        Serial.print(F("=0b"));
        printContactsBits(Contacts, SW_NB);
        Serial.print(F(" 0x"));

        if (SW_NB == 8 && Contacts < 0x10) Serial.print('0');

        if (SW_NB == 16) {
          if (Contacts < 0x1000) Serial.print('0');
          if (Contacts < 0x0100) Serial.print('0');
          if (Contacts < 0x0010) Serial.print('0');
        }

        Serial.println(Contacts, HEX);
      }
#endif
    }
  }
}

static void printContactsBits(uint16_t value, uint8_t bitCount)
{
  for (int8_t bit = (int8_t)bitCount - 1; bit >= 0; --bit)
    Serial.print((value >> bit) & 0x01);
}

static void BuildValueMsg(void)
{
  memset(MyTxMsg.Byte, 0, sizeof(MyTxMsg.Byte));
  LoadAngle(Angle);
  LoadProp(Prop);
  LoadContacts(Contacts);
}

static void LoadAngle(uint16_t angle)
{
  if (PayloadDesc.Angle) {
    MyTxMsg.Byte[0] = (angle & 0x0FF0) >> 4;
    MyTxMsg.Byte[1] = (angle & 0x000F) << 4;
  }
}

static void LoadProp(uint8_t prop)
{
  if (!PayloadDesc.Prop) return;

  if (!PayloadDesc.Angle) {
    MyTxMsg.Byte[0] = prop;
  } else {
    MyTxMsg.Byte[1] |= (prop & 0xF0) >> 4;
    MyTxMsg.Byte[2] |= (prop & 0x0F) << 4;
  }
}

static void LoadContacts(uint16_t contacts)
{
  if (!PayloadDesc.ContactNb) return;

  uint8_t nibbleNb = PayloadDesc.ContactNb / 4;
  uint8_t nibbleIndex = (PayloadDesc.Angle * 3) + (PayloadDesc.Prop * 2) + nibbleNb - 1;
  uint8_t byteIndex = nibbleIndex / 2;

  while (nibbleNb) {
    const uint8_t nibble = contacts & 0x0F;
    contacts >>= 4;

    if (nibbleIndex & 1)
      MyTxMsg.Byte[byteIndex] |= nibble;
    else {
      MyTxMsg.Byte[byteIndex] |= nibble << 4;
      if (byteIndex > 0) --byteIndex;
    }

    --nibbleIndex;
    --nibbleNb;
  }
}

static uint8_t NibbleNbToSend(void)
{
  return ((PayloadDesc.Angle * 12) +
          (PayloadDesc.Prop * 8) +
          PayloadDesc.ContactNb) / 4;
}

void blinkRGBLED()
{
  // Check if it's time to change the state of the LED
  if (ledRGBInfo > 1000)
  {
    ledRGBInfo = 0;  // restart the chrono so that it triggers again later
    // toggle the stored state
    ledRGBSate = !ledRGBSate;
    ledRGBSate ? leds_Synchro[0] = CRGB::Black : leds_Synchro[0] = CRGB::Red;
    FastLED.show();
    /* En abscence de signal RCUL, la led RGB clignote en rouge */
  }
}
