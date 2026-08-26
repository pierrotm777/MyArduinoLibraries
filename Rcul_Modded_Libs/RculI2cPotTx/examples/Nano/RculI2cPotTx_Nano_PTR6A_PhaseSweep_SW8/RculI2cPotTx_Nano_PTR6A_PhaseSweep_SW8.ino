/*
  Arduino Nano + DS3502 + PCF8574A SW8 + PTR-6A
  Balayage de phase du profil CALLBACK PTR-6A.

  Le profil PTR-6A normal de la V1.10.6 utilise 2750 us par defaut.
  CET EXEMPLE est volontairement different : il ecrase cette valeur pour
  balayer la phase et permettre la caracterisation d'autres emetteurs.

  DS3502   : 0x28
  PCF8574A : 0x38
  I2C      : 100 kHz
  GDO0     : D8
  SDA/SCL  : A4/A5

  RCUL_REPEAT = 3
  Filtrage RX = 1

  Auto: 0,500,1000,...3500 us, 60 s par phase.
  Commandes:
    a    auto ON/OFF
    n/p  phase suivante/precedente (+/-500 us)
    0..7 phase directe
    +/-  reglage fin +/-100 us
    s    status
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

#define RCUL_REPEAT             3
#define RCUL_FIFO_SIZE          8
#define RCUL_CHANNEL            8

#define SWITCH_DEBOUNCE_MS      20UL
#define PCF_READ_PERIOD_US      7000UL

#define PHASE_MIN_US            0U
#define PHASE_MAX_US            3500U
#define PHASE_COARSE_STEP_US    500U
#define PHASE_FINE_STEP_US      100U

#define AUTO_SWEEP_DEFAULT      1
#define AUTO_PHASE_TIME_MS      60000UL

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

static uint16_t PhaseUs = 0;
static bool AutoSweep = AUTO_SWEEP_DEFAULT;
static uint32_t PhaseStartMs = 0;

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

static void printPhase(void)
{
  Serial.print(F("PHASE="));
  Serial.print(PhaseUs);
  Serial.print(F(" us  AUTO="));
  Serial.println(AutoSweep ? F("ON") : F("OFF"));
}

static void applyPhase(uint16_t NewPhaseUs)
{
  if(NewPhaseUs > PHASE_MAX_US)
  {
    NewPhaseUs = PHASE_MAX_US;
  }

  PhaseUs = NewPhaseUs;
  RculI2cPotTx.setCallbackSyncOffsetUs(PhaseUs);
  PhaseStartMs = millis();
  printPhase();
}

static void nextPhase(void)
{
  if(PhaseUs + PHASE_COARSE_STEP_US > PHASE_MAX_US)
    applyPhase(PHASE_MIN_US);
  else
    applyPhase(PhaseUs + PHASE_COARSE_STEP_US);
}

static void previousPhase(void)
{
  if(PhaseUs < PHASE_COARSE_STEP_US)
    applyPhase(PHASE_MAX_US);
  else
    applyPhase(PhaseUs - PHASE_COARSE_STEP_US);
}

static void processSerial(void)
{
  if(!Serial.available()) return;

  const char Cmd = (char)Serial.read();

  if((Cmd >= '0') && (Cmd <= '7'))
  {
    AutoSweep = false;
    applyPhase((uint16_t)(Cmd - '0') * PHASE_COARSE_STEP_US);
  }
  else if(Cmd == 'a' || Cmd == 'A')
  {
    AutoSweep = !AutoSweep;
    PhaseStartMs = millis();
    printPhase();
  }
  else if(Cmd == 'n' || Cmd == 'N')
  {
    AutoSweep = false;
    nextPhase();
  }
  else if(Cmd == 'p' || Cmd == 'P')
  {
    AutoSweep = false;
    previousPhase();
  }
  else if(Cmd == '+')
  {
    AutoSweep = false;
    uint16_t v = PhaseUs + PHASE_FINE_STEP_US;
    if(v > PHASE_MAX_US) v = PHASE_MAX_US;
    applyPhase(v);
  }
  else if(Cmd == '-')
  {
    AutoSweep = false;
    uint16_t v = (PhaseUs >= PHASE_FINE_STEP_US)
               ? (uint16_t)(PhaseUs - PHASE_FINE_STEP_US)
               : PHASE_MIN_US;
    applyPhase(v);
  }
  else if(Cmd == 's' || Cmd == 'S')
  {
    printPhase();
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
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
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

  applyPhase(PHASE_MIN_US);

  RculI2cPotTx.printInfo(Serial);

  Serial.println(F("----------------------------------------"));
  Serial.println(F("PTR-6A PHASE SWEEP / SW8"));
  Serial.print(F("PTR-6A library reference phase = "));
  Serial.print(RCUL_I2C_POT_CALLBACK_PTR_6A_SYNC_OFFSET_US);
  Serial.println(F(" us"));
  Serial.println(F("I2C=100kHz PCF8574A=0x38 R=3"));
  Serial.println(F("Receiver filtering = 1"));
  Serial.println(F("AUTO: 60 s per phase"));
  Serial.println(F("Commands: a n p 0..7 + - s"));
  Serial.println(F("----------------------------------------"));

  PhaseStartMs = millis();
}

void loop()
{
  RculI2cPotTx.process();

  processSerial();

  if(AutoSweep &&
     ((uint32_t)(millis() - PhaseStartMs) >= AUTO_PHASE_TIME_MS))
  {
    nextPhase();
  }

  updateContacts();

  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[1];
    Message[0] = Contacts;
    MyRcTxSerial.sendNibbleMsg(Message, 2, 1);
  }

  RcTxSerial::process();

  RculI2cPotTx.process();
}
