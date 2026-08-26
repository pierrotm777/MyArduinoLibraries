#include <Arduino.h>
#include <Wire.h>
#include <RcTxSerial.h>

bool DebugIsOn = false;

#define PRINT_BUF_SIZE      100
static char PrintBuf[PRINT_BUF_SIZE + 1];
#define PRINTF(fmt, ...)                                   \
  do {                                                     \
    if (Serial) {                                          \
      snprintf(PrintBuf, PRINT_BUF_SIZE, fmt, ##__VA_ARGS__); \
      Serial.print(PrintBuf);                              \
    }                                                      \
  } while (0)

// Select the I2C digital potentiometer used by this example.
// Choose MCP4561 or DS3502.
#define RCUL_I2C_POT_DEVICE DS3502

// ============================================================================
// I2C pins
// ============================================================================
#define SDA_PIN     A4
#define SCL_PIN     A5

// ============================================================================
// Shared CALLBACK / PWM Learn input pin
// ============================================================================
// One physical GPIO is reused for both functions:
//
//   NORMAL mode : SYNC_LEARN_PIN <- GDO0 PTR-6A
//   LEARN mode  : SYNC_LEARN_PIN <- receiver PWM output
//
// IMPORTANT: GDO0 and the receiver PWM output are two active outputs.
// They must NOT be electrically connected together at the same time.
// Select only one source at a time with a jumper, switch, connector or mux.
#define SYNC_LEARN_PIN  5
#define GDO0_PIN        SYNC_LEARN_PIN
#define PWM_RX_PIN      SYNC_LEARN_PIN

#include <RculPWMRead.h>
static RculPWMRead ReceiverPwm;
static bool ReceiverPwmAttached = false;
static bool useStoredRculTable = false;

// ============================================================================
// Analog proportional input
// ============================================================================
#define PROP_PIN     A0
#define PROP_ADC_MAX 1023UL

/* Potentiometer wiring:
     one end -> board VCC
     wiper   -> PROP_PIN
     other end -> GND

   PROP is encoded on 8 bits: ADC minimum -> 0, ADC maximum -> 255.
*/

#define I2C_FREQUENCY RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY
// #define I2C_FREQUENCY RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY

#define DIGIPOT_ADDRESS RCUL_I2C_POT_TX_AUTO_ADDRESS
// #define DIGIPOT_ADDRESS 0x2D

// Keep the original global RculI2cPotTx object name while selecting its driver.
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A,
    GDO0_PIN
);

// ============================================================================
// MCP23017 - 16 switches on ports A and B
// ============================================================================
/* MCP23017 7-bit I2C address:
     A2 A1 A0 = 000 -> 0x20 ... 111 -> 0x27

   Register-based device, unlike the PCF8574.

   GPA0 ... GPA7 -> SW1 ... SW8
   GPB0 ... GPB7 -> SW9 ... SW16
   Each switch is connected between its GPIO and GND.
*/
static bool Mcp23017Connected = false;
static bool ProportionnalConnected = false;
#define MCP23017_ADDRESS 0x24
#define SWITCH_DEBOUNCE_MS 20UL

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPPUA  0x0C
#define MCP23017_GPPUB  0x0D
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

// RCUL payload sizes and resulting MLEN values:
//   SW8          : 2 payload nibbles -> MLEN=4
//   SW8+PROP     : 4 payload nibbles -> MLEN=6
//   SW16         : 4 payload nibbles -> MLEN=6
//   ANGLE+PROP   : 5 payload nibbles -> MLEN=7
//   SW16+PROP    : 6 payload nibbles -> MLEN=8
//
// For SW16+PROP the payload byte order is:
//   Byte[0] = PROP
//   Byte[1] = SW16 high byte (SW9..SW16)
//   Byte[2] = SW16 low byte  (SW1..SW8)
#define RCUL_REPEAT  	  2
#define RCUL_FIFO_SIZE	8
#define RCUL_CHANNEL 	  8

static RcTxSerial MyRcTxSerial(
    &RculI2cPotTx,
    RCUL_REPEAT,
    RCUL_FIFO_SIZE,
    RCUL_CHANNEL
);

static uint16_t Contacts = 0;
static uint16_t CandidateContacts = 0;
static uint32_t CandidateSinceMs = 0;
static uint8_t Prop = 0;
static uint32_t LastPrintMs = 0;

static bool mcp23017WriteRegister(uint8_t Register, uint8_t Value)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(Register);
  Wire.write(Value);
  return (Wire.endTransmission() == 0);
}

static bool mcp23017ReadRegisters(uint8_t StartRegister,
                                  uint8_t *Data,
                                  uint8_t Length)
{
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(StartRegister);
  if(Wire.endTransmission(false) != 0)
  {
    return false;
  }

  const uint8_t Count = Wire.requestFrom((uint8_t)MCP23017_ADDRESS, Length);
  if(Count != Length)
  {
    while(Wire.available()) Wire.read();
    return false;
  }

  for(uint8_t Index = 0; Index < Length; Index++)
  {
    if(!Wire.available()) return false;
    Data[Index] = Wire.read();
  }

  return true;
}

static bool mcp23017ReadContacts(uint16_t &Value)
{
  uint8_t Ports[2];
  if(!mcp23017ReadRegisters(MCP23017_GPIOA, Ports, 2))
  {
    return false;
  }

  const uint16_t Raw = (uint16_t)Ports[0] |
                       ((uint16_t)Ports[1] << 8);
  Value = (uint16_t)~Raw;
  return true;
}

static bool mcp23017Begin(void)
{
  if(!mcp23017WriteRegister(MCP23017_IODIRA, 0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_IODIRB, 0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_GPPUA,  0xFF)) return false;
  if(!mcp23017WriteRegister(MCP23017_GPPUB,  0xFF)) return false;

  if(!mcp23017ReadContacts(Contacts)) return false;

  CandidateContacts = Contacts;
  CandidateSinceMs = millis();
  return true;
}

static void updateContacts(void)
{
  uint16_t NewContacts;
  if(!mcp23017ReadContacts(NewContacts))
  {
    return;
  }

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
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500UL) delay(1);

  Serial.println();
  Serial.println(F("***********************************"));
  Serial.println(F("RculI2cPotTx + MCP23017 - SW16+PROP"));
  Serial.println(F("***********************************"));

  Serial.println("Help: c=learn t=display r=default e=erase");
  Serial.println();  
  Serial.println();  

  pinMode(PROP_PIN, INPUT);

  if(!RculI2cPotTx.begin(SDA_PIN,
                         SCL_PIN,
                         I2C_FREQUENCY,
                         DIGIPOT_ADDRESS,
                         RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
                         1500))
  {
    Serial.println(F("Digital potentiometer not detected or initial write failed"));
    //return;
  }

  Mcp23017Connected = mcp23017Begin();
  if(Prop !=0) 
    ProportionnalConnected = true;

  RculI2cPotTx.setTableStorageEEPROM(0);

  useStoredRculTable = RculI2cPotTx.useStoredRculTable();

  //updateProp();

  RculI2cPotTx.printInfo(Serial);

  PRINTF("MCP23017     : %s (Add: 0x%02X)\r\n", Mcp23017Connected ? F("YES") : F("NO"), Mcp23017Connected ? MCP23017_ADDRESS : 0);
  PRINTF("Proportionnal: %s (Pin: %d)\r\n", ProportionnalConnected ? F("YES") : F("NO "), SYNC_LEARN_PIN);
  Serial.println(F("----------------------------------------"));
  Serial.println(F("Payload: PROP + SW16 high byte + SW16 low byte"));
  Serial.println(F("6 payload nibbles; MLEN=8"));
  Serial.println(F("----------------------------------------"));

  // Serial.print(F("Use Stored table: "));
  // Serial.println(useStoredRculTable ? F("YES") : F("NO"));
  // Serial.println(F("----------------------------------------"));

}

void loop()
{

  if(Serial.available())
  {
    const char Cmd = Serial.read();

    if(Cmd == 'h')
    {
      Serial.println("Help: c=learn t=display r=default e=erase");
    }

    if(Cmd == 'd')
    {
      PRINTF("Debug is %s\r\n",DebugIsOn ? "ON":"OFF");
    }

    if(Cmd == 'c')
    {
      if(!RculI2cPotTx.isRculTableCalibrationActive())
      {
        Serial.println(F("LEARN: connect receiver PWM to SYNC_LEARN_PIN"));

        if(ReceiverPwm.attach(PWM_RX_PIN, 700, 2300) < 0)
        {
          Serial.println(F("PWM input ERROR"));
        }
        else
        {
          ReceiverPwmAttached = true;

          RculI2cPotTx.startRculTableCalibration(
              ReceiverPwm,
              &Serial,
              6,
              2,
              4);

          if(!RculI2cPotTx.isRculTableCalibrationActive())
          {
            ReceiverPwm.detach();
            ReceiverPwmAttached = false;
            pinMode(SYNC_LEARN_PIN, INPUT);
            Serial.println(F("LEARN start ERROR"));
          }
        }
      }
    }

    else if(Cmd == 't')
    {
      RculI2cPotTx.displayRculTable(Serial);
    }

    else if(Cmd == 'r')
    {
      RculI2cPotTx.useDefaultRculTable();
      Serial.println("Embedded default table selected");
    }

    else if(Cmd == 'e')
    {
      RculI2cPotTx.eraseStoredRculTable();
      Serial.println("Stored table erased");
    }
  }

  if(RculI2cPotTx.isRculTableCalibrationActive())
  {
    RculI2cPotTx.processRculTableCalibration();

    if(!RculI2cPotTx.isRculTableCalibrationActive() && ReceiverPwmAttached)
    {
      ReceiverPwm.detach();
      ReceiverPwmAttached = false;

      // Return the shared GPIO to normal CALLBACK / GDO0 use.
      pinMode(SYNC_LEARN_PIN, INPUT);

      Serial.println(F("LEARN finished: reconnect GDO0 to SYNC_LEARN_PIN"));
    }

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
    Message[2] = (uint8_t)(Contacts);       // SW1..SW8

    // SW16+PROP payload = 24 bits = 6 nibbles -> MLEN=8.
    MyRcTxSerial.sendNibbleMsg(Message, 6, 1);
  }

  RcTxSerial::process();

  if (((uint32_t)(millis() - LastPrintMs) >= 1000UL) && DebugIsOn)
  {
    LastPrintMs = millis();
    Serial.print(F("PROP="));
    Serial.print(Prop);
    Serial.print(F(" Contacts=0x"));
    if(Contacts < 0x1000) Serial.print('0');
    if(Contacts < 0x0100) Serial.print('0');
    if(Contacts < 0x0010) Serial.print('0');
    Serial.print(Contacts, HEX);
    Serial.print(F(" Wiper="));
    Serial.print(RculI2cPotTx.getWiper());
    Serial.print('/');
    Serial.println(RculI2cPotTx.getMaxWiper());
  }
}
