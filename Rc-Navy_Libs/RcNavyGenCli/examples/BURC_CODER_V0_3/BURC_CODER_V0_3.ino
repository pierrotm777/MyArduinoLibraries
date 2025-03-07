#include <Cppm.h>
#include <SBusTx.h>
#include <GenCli.h>
#include <RcTxSerial.h>
#include <SoftRcPulseIn.h>
#include <TinyCppmReader.h>
#include <RcRxSerial.h>
#include <EEPROM.h>
#ifdef ARDUINO_AVR_NANO
#include <Wire.h>
#endif

#define SW_VERSION          0
#define SW_REVISION         3 /* TO DO: read contacts from I2C Expenders for RCUL intances 2, 3 and 4 */

#define HTONS(x)            __builtin_bswap16((uint16_t) (x))

#define PRINT_BUF_SIZE      100
static char PrintBuf[PRINT_BUF_SIZE + 1];

#define PRINTF(fmt, ...)    snprintf_P(PrintBuf, PRINT_BUF_SIZE, PSTR(fmt) ,##__VA_ARGS__);Serial.print(PrintBuf)
#define PRINT_P(FlashStr)   Serial.print(F(FlashStr))

#if (GEN_CLI_CMD_FIELD_MAX_NB != 3)
#error Please, set GEN_CLI_CMD_FIELD_MAX_NB to the correct value in GenCli.h
#endif

#define CMD_FLD_SEPARATOR   (char*)"."

/*
                                         O
                                         |
                                         |
        .--------------------------------+-------------------------------.
        |                                                                |
        |                                                                |
        |                                                                |
        |                                                                |
        |                                                                |
        |                                                                |
        |                        RC TRANSMITTER                          |
        |                                                                |
        |                                                                |
        |                                                                |
        |                                                                |
        |                                                                |
        |                                                                |
        |                       .---------------.                        |
        |                       | Trainer Port  |                        |
        |                       | 0V Out  In 5V |                        |
        '-----------------------+-+-----------+-+------------------------'
                                  |   |   ^   |
                                  |   V   |   |
      .---------------------------+-----------+-------------------------------.           .-----------------------------------------------------------------------.
      |                         0V          5V                             +5V+-----------+                                                                       + +5V
      |                           BURC CODER                               SDA+-----------+                               PCF8575                                 + SDA
      |                                                                    SCL+-----------+                                                                       + SCL
      |    C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 C13 C14 C15 C16 GND+-----------+    C1  C2  C3  C4  C5  C6  C7  C8  C9  C10 C11 C12 C13 C14 C15 C16    + GND
      '-----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-----'           '-----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-----'
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |                       |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        C1 [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [| C16               C1 [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [|  [| C16 
            |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |                       |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
            '---+---+---+---+---+---+---+-+-+---+---+---+---+---+---+---'                       '---+---+---+---+---+---+---+-+-+---+---+---+---+---+---+---'
                                          |                                                                                   |
                                        --+--                                                                               --+--
                                         0V                                                                                  0V
*/

/* The  PPM_OUTPUT_PIN  is hard-wired on pin 9 -> Trainer port PPM in  */
#define PPM_INPUT_PIN   8                      /* Trainer port PPM out */

#define C01_INPUT_PIN   2
#define C02_INPUT_PIN   3
#define C03_INPUT_PIN   4
#define C04_INPUT_PIN   5
#define C05_INPUT_PIN   6
#define C06_INPUT_PIN   7
#define C07_INPUT_PIN   10
#define C08_INPUT_PIN   11
#define C09_INPUT_PIN   12
#define C10_INPUT_PIN   13
#define C11_INPUT_PIN   A0
#define C12_INPUT_PIN   A1
#define C13_INPUT_PIN   A2
#define C14_INPUT_PIN   A3
#ifdef ARDUINO_AVR_NANO
#warning Compilation of BURC coder for NANO
#define C15_INPUT_PIN   A6
#define C16_INPUT_PIN   A7
#else
#ifdef ARDUINO_AVR_UNO
#warning Compilation of BURC coder for UNO
#define C15_INPUT_PIN   A4
#define C16_INPUT_PIN   A4 // A5 is reserved for RX debug on UNO
#else
#error Board not supported: only UNO and Nano are supported!
#endif
#endif

static SoftRcPulseIn  PwmRcSignal;
static TinyCppmReader TinyCppmReader;

RcRxSerial MyRcRxSerial(&PwmRcSignal, RC_RX_SERIAL_NO_FILTER); /* Create a Rx serial port on the channel of the PWM */

#define EXPECTED_NIBBLE_LEN  4
#define NEUTRAL_WIDTH_US     1500
#define NIBBLE_WIDTH_US                   56
#define FULL_EXCURSION_US                 (NIBBLE_WIDTH_US * NIBBLE_NB)
#define PULSE_MIN_US                      (NEUTRAL_WIDTH_US - (FULL_EXCURSION_US / 2))
#define PULSE_WIDTH_US(NibbleIdx)         (PULSE_MIN_US + (NIBBLE_WIDTH_US / 2)+ ((NibbleIdx) * NIBBLE_WIDTH_US))
#define EXCURSION_HALF_US(NibbleIdx)      ((PULSE_WIDTH_US(NibbleIdx) - NEUTRAL_WIDTH_US) * 2)
#define RX_MSG_LEN_MAX       6
#define TABLE_ITEM_NBR(Tbl)               (sizeof(Tbl) / sizeof(Tbl[0]))

enum {NIBBLE_0 = 0, NIBBLE_1, NIBBLE_2, NIBBLE_3, NIBBLE_4, NIBBLE_5, NIBBLE_6, NIBBLE_7, NIBBLE_8, NIBBLE_9, NIBBLE_A, NIBBLE_B, NIBBLE_C, NIBBLE_D, NIBBLE_E, NIBBLE_F, NIBBLE_R, NIBBLE_I, NIBBLE_NB};

const int16_t ExcursionHalf_us[] PROGMEM = {EXCURSION_HALF_US(NIBBLE_0), EXCURSION_HALF_US(NIBBLE_1), EXCURSION_HALF_US(NIBBLE_2), EXCURSION_HALF_US(NIBBLE_3),
                                            EXCURSION_HALF_US(NIBBLE_4), EXCURSION_HALF_US(NIBBLE_5), EXCURSION_HALF_US(NIBBLE_6), EXCURSION_HALF_US(NIBBLE_7),
                                            EXCURSION_HALF_US(NIBBLE_8), EXCURSION_HALF_US(NIBBLE_9), EXCURSION_HALF_US(NIBBLE_A), EXCURSION_HALF_US(NIBBLE_B),
                                            EXCURSION_HALF_US(NIBBLE_C), EXCURSION_HALF_US(NIBBLE_D), EXCURSION_HALF_US(NIBBLE_E), EXCURSION_HALF_US(NIBBLE_F),
                                            EXCURSION_HALF_US(NIBBLE_R), EXCURSION_HALF_US(NIBBLE_I)};

#define GET_EXC_HALF_US(NibbleIdx)        (int16_t)pgm_read_word(&ExcursionHalf_us[(NibbleIdx)])

static uint8_t RxMsg[RX_MSG_LEN_MAX];

typedef struct{
  uint8_t RxFrameCnt;
  uint8_t ErrorCnt;
}RculStatSt_t;

static RculStatSt_t   RculStat = {0, 0};
void PrintBin(uint8_t *Buf, uint8_t BufSize, uint8_t RemainingNibble = 0);
void PrintByteBin(uint8_t Byte, uint8_t RemainingNibble = 0);

#define CONTACT_NB           16

#define CPPM_PERIOD_US_FOR_CH_NB(ChNb)    (((ChNb) * 2000) + 3000)

const uint8_t InputPin[] = { 0, /* Dummy value for index 0 */
        C01_INPUT_PIN, C02_INPUT_PIN, C03_INPUT_PIN, C04_INPUT_PIN,
        C05_INPUT_PIN, C06_INPUT_PIN, C07_INPUT_PIN, C08_INPUT_PIN,
        C09_INPUT_PIN, C10_INPUT_PIN, C11_INPUT_PIN, C12_INPUT_PIN,
        C13_INPUT_PIN, C14_INPUT_PIN, C15_INPUT_PIN, C16_INPUT_PIN
        };

enum {INITIAL_CPPM_FRAME = 0, FINAL_CPPM_FRAME};

/*
TRAINER.MODE=Mode (avec Mode: PPM_OUT+PPM_IN, PPM_IN, SBUS_IN)
RCULx.CHANNEL=y
RCULx.REPEAT=n
CPPM.MODU=Modu (avec Mode: POS ou NEG)
CPPM.HEADER=300
CPPM.CHANNEL.NB=4
CPPM.PERIOD=22500
*/
DECL_FLASH_STR(CONF);
DECL_FLASH_STR(HELP);DECL_FLASH_STR(AIDE);
DECL_FLASH_STR(VERSION);
DECL_FLASH_STR(LANG);
DECL_FLASH_STR(TRAINER);DECL_FLASH_STR(ECOLAGE);
DECL_FLASH_STR(RCUL);
DECL_FLASH_STR(CPPM);
DECL_FLASH_STR(SBUS);
DECL_FLASH_STR(RX);
DECL_FLASH_STR_TBL(CmdFld0Tbl)        = {_CONF_, _HELP_, _AIDE_, _VERSION_, _LANG_, _TRAINER_, _ECOLAGE_, _RCUL_, _CPPM_, _SBUS_, _RX_};
enum {F_CONF = 0, F_HELP, F_AIDE, F_VERSION, F_LANG, F_TRAINER, F_ECOLAGE, F_RCUL, F_CPPM, F_SBUS, F_RX}; // Field

DECL_FLASH_STR(MODE);
DECL_FLASH_STR(REPEAT);DECL_FLASH_STR(REPET);
DECL_FLASH_STR(TEST);
DECL_FLASH_STR(CHANNEL);DECL_FLASH_STR(VOIE);
DECL_FLASH_STR(MODU);
DECL_FLASH_STR(HEADER);DECL_FLASH_STR(ENTETE);
DECL_FLASH_STR(PERIOD);DECL_FLASH_STR(PERIODE);
DECL_FLASH_STR(INITIAL);
DECL_FLASH_STR(FINAL);
DECL_FLASH_STR(SPEED);
DECL_FLASH_STR(VITESSE);
DECL_FLASH_STR(DBG);
DECL_FLASH_STR_TBL(CmdFld1Tbl)        = {_MODE_, _REPEAT_, _REPET_, _TEST_, _CHANNEL_, _VOIE_, _MODU_, _HEADER_, _ENTETE_, _PERIOD_, _PERIODE_, _INITIAL_, _FINAL_, _SPEED_, _VITESSE_, _DBG_};
enum {SF_MODE = 0, SF_REPEAT, SF_REPET, SF_TEST, SF_CHANNEL, SF_VOIE, SF_MODU, SF_HEADER, SF_ENTETE, SF_PERIOD, SF_PERIODE, SF_INITIAL, SF_FINAL, SF_SPEED, SF_VITESSE, SF_DBG}; // Sub-Field

DECL_FLASH_STR(NB);
DECL_FLASH_STR_TBL(CmdFld2Tbl)        = {_NB_};
enum {SSF_NB = 0}; // Sub-Sub-Field

GEN_CLI_FLD_TBL_LIST(CmdFldListTbl) = {
                                        GEN_CLI_FLD_TBL_ATTR(CmdFld0Tbl),
                                        GEN_CLI_FLD_TBL_ATTR(CmdFld1Tbl),
                                        GEN_CLI_FLD_TBL_ATTR(CmdFld2Tbl),
                                      };

enum {_CONF = 0, _HELP, _AIDE, _VERSION, _LANG, _TRAINER_MODE, _ECOLAGE_MODE, _RCUL, _RCUL_CHANNEL, _RCUL_VOIE, _RCUL_REPEAT, _RCUL_REPET, _RCUL_TEST,
      _CPPM, _CPPM_MODU, _CPPM_HEADER, _CPPM_ENTETE, _CPPM_CHANNEL_NB, _CPPM_VOIE_NB, _CPPM_PERIOD, _CPPM_PERIODE, _CPPM_INITIAL, _CPPM_FINAL,
      _SBUS_SPEED, _SBUS_VITESSE, _RX_MODE, _RX_DBG};

GEN_CLI_CMD_TBL(CmdTbl)             = {
                                        {_CONF,            {F_CONF,    -1,         -1     }},
                                        {_HELP,            {F_HELP,    -1,         -1     }},
                                        {_AIDE,            {F_AIDE,    -1,         -1     }},
                                        {_VERSION,         {F_VERSION, -1,         -1     }},
                                        {_LANG,            {F_LANG,    -1,         -1     }},
                                        {_TRAINER_MODE,    {F_TRAINER, SF_MODE,    -1     }},
                                        {_ECOLAGE_MODE,    {F_ECOLAGE, SF_MODE,    -1     }},
                                        {_RCUL,            {F_RCUL,    -1,         -1     }},
                                        {_RCUL_CHANNEL,    {F_RCUL,    SF_CHANNEL, -1     }},
                                        {_RCUL_VOIE,       {F_RCUL,    SF_VOIE,    -1     }},
                                        {_RCUL_REPEAT,     {F_RCUL,    SF_REPEAT,  -1     }},
                                        {_RCUL_REPET,      {F_RCUL,    SF_REPET,   -1     }},
                                        {_RCUL_TEST,       {F_RCUL,    SF_TEST,    -1     }},
                                        {_CPPM,            {F_CPPM,    -1,         -1     }},
                                        {_CPPM_MODU,       {F_CPPM,    SF_MODU,    -1     }},
                                        {_CPPM_HEADER,     {F_CPPM,    SF_HEADER,  -1     }},
                                        {_CPPM_ENTETE,     {F_CPPM,    SF_ENTETE,  -1     }},
                                        {_CPPM_CHANNEL_NB, {F_CPPM,    SF_CHANNEL, SSF_NB }},
                                        {_CPPM_VOIE_NB,    {F_CPPM,    SF_VOIE,    SSF_NB }},
                                        {_CPPM_PERIOD,     {F_CPPM,    SF_PERIOD,  -1     }},
                                        {_CPPM_PERIODE,    {F_CPPM,    SF_PERIODE, -1     }},
                                        {_CPPM_INITIAL,    {F_CPPM,    SF_INITIAL, -1     }},
                                        {_CPPM_FINAL,      {F_CPPM,    SF_FINAL,   -1     }},
                                        {_SBUS_SPEED,      {F_SBUS,    SF_SPEED,   -1     }},
                                        {_SBUS_VITESSE,    {F_SBUS,    SF_VITESSE, -1     }},
                                        {_RX_MODE,         {F_RX,      SF_MODE,    -1     }},
                                        {_RX_DBG,          {F_RX,      SF_DBG,     -1     }},
                                      };
/* Possible argument values for commands */
DECL_FLASH_STR(UK);
DECL_FLASH_STR(FR);
DECL_FLASH_STR_TBL(LangTbl)     = {_UK_, _FR_};
enum {LANG_UK = 0, LANG_FR, LANG_MAX_NB};


DECL_FLASH_STR2(PPM_OUT_PLUS_PPM_IN, PPM_OUT+PPM_IN);
DECL_FLASH_STR(PPM_IN);
DECL_FLASH_STR(SBUS_IN);
DECL_FLASH_STR_TBL(TrainerModeTbl)     = {_PPM_OUT_PLUS_PPM_IN_, _PPM_IN_, _SBUS_IN_};
enum {TRAINER_MODE_PPM_OUT_PLUS_PPM_IN = 0, TRAINER_MODE_PPM_IN, TRAINER_MODE_SBUS_IN, TRAINER_MODE_MAX_NB};

DECL_FLASH_STR(NEG);
DECL_FLASH_STR(POS);
DECL_FLASH_STR_TBL(CppmModuTbl)        = {_NEG_, _POS_};
enum {CPPM_MODU_NEG = 0, CPPM_MODU_POS, CPPM_MODU_MAX_NB};

DECL_FLASH_STR(NORMAL);
DECL_FLASH_STR(FAST);
DECL_FLASH_STR_TBL(SbusSpeedUkTbl)     = {_NORMAL_, _FAST_};
DECL_FLASH_STR(NORMALE);
DECL_FLASH_STR(RAPIDE);
DECL_FLASH_STR_TBL(SbusSpeedFrTbl)     = {_NORMALE_, _RAPIDE_};
enum {SBUS_NORMAL = 0, SBUS_FAST, SBUS_SPEED_NB};

DECL_FLASH_STR(OFF);
DECL_FLASH_STR(ON);
DECL_FLASH_STR_TBL(RculTestTbl)        = {_OFF_, _ON_};
enum {RCUL_TEST_OFF = 0, RCUL_TEST_ON, RCUL_TEST_MAX_NB};

DECL_FLASH_STR(PWM);
DECL_FLASH_STR_TBL(RxModeTbl)          = {_OFF_, _PWM_, _CPPM_};
enum {RX_MODE_OFF = 0, RX_MODE_PWM, RX_MODE_CPPM, RX_MODE_MAX_NB};

#define RCUL_INSTANCE_MAX_NB    4
#define RCUL_REPEAT_MAX_NB      9

static Rcul *RculDst = NULL;

static RcTxSerial MyRcTxSerial[RCUL_INSTANCE_MAX_NB] = {
                                      RcTxSerial(RculDst, RC_TX_SERIAL_NO_REPEAT, 8), 
                                      RcTxSerial(RculDst, RC_TX_SERIAL_NO_REPEAT, 8), 
                                      RcTxSerial(RculDst, RC_TX_SERIAL_NO_REPEAT, 8), 
                                      RcTxSerial(RculDst, RC_TX_SERIAL_NO_REPEAT, 8)
                                    };
typedef struct {
  uint16_t Contacts;
}RcTxMsgSt_t;

static RcTxMsgSt_t PrevRcTxMsg[RCUL_INSTANCE_MAX_NB];
static RcTxMsgSt_t RcTxMsg[RCUL_INSTANCE_MAX_NB];

typedef struct {
  uint8_t  Modulation;
  uint8_t  ChannelNb;
  uint16_t Header_us;
  uint16_t InitialPeriod_us;
  uint16_t FinalPeriod_us;
}CppmSt_t;

typedef struct{
  uint8_t Mode;
}TrainerSt_t;

typedef struct{
  uint8_t Channel;
}RculSt_t;

typedef struct{
  uint8_t Mode; /* 0: PWM, 1: CPPMx */
  uint8_t Ch;
}RxSt_t;

#define RCUL_MAX_NB            4
#define CH_MAX_NB              12

typedef struct{
  uint8_t     Lang;
  TrainerSt_t Trainer;
  CppmSt_t    Cppm;
  uint8_t     SbusSpeedMs;
  uint8_t     RculRepeatNb;
  RculSt_t    Rcul[RCUL_MAX_NB];
  RxSt_t      Rx;
}EepSt_t;

#define EEP_ST_LOCATION        0 /* At the beginning of the EEPROM */
static EepSt_t   EepSt; /* Structure in RAM (same as in EEPROM) */

static CppmSt_t  Cppm;

#define CPPM_TRY_CNT_MAX       4

#define TM_MSG_MAX_LENGTH      40 /* Message le plus long */

static char TxRxBuf[TM_MSG_MAX_LENGTH + 1];/* + 1 pour fin de chaine */

static uint8_t RxDbg = 0;

#define SBUS_START_DELAY_MS    5000
static uint16_t SBusStartMs16 = 1;

static void RculDstInit(uint8_t EnableSBus = 0);
static void RculRxInit(uint8_t PrevRxMode, uint8_t NewRxMode);

void setup()
{
  uint8_t CppmCaught = 0, TryCnt = 0;
  
  Serial.begin(115200);
    
  for(uint8_t Id = 1; Id <= CONTACT_NB; Id++)
  {
    pinMode(InputPin[Id], INPUT_PULLUP); /* Activate pull-up for each input */
  }
  EEPROM.get(EEP_ST_LOCATION, EepSt);
  if((EepSt.Lang == 0xFF) || (EepSt.SbusSpeedMs == 0xFF))
  {
    SetDefaultParameters();
  }
  if(EepSt.Trainer.Mode == TRAINER_MODE_PPM_OUT_PLUS_PPM_IN)
  {
    CppmReader.begin(); /* Start the PPM frame reader */
    do
    {
      delay(500);         /* Wait to catch the PPM Modu, PPM Header, PPM Period and the number of channels */
      /* Store PPM characteristics in a structure to avoid small differences in measurements */
      EepSt.Cppm.Modulation        = CppmReader.modulation();
      EepSt.Cppm.ChannelNb         = CppmReader.detectedChannelNb();
      EepSt.Cppm.Header_us         = CppmReader.ppmHeader_us();
      EepSt.Cppm.InitialPeriod_us  = CppmReader.ppmPeriod_us();
      if(Cppm.Header_us)
      {
        /* Start the PPM frame generator with the same characteristics */
        Cppm.FinalPeriod_us = CppmGen.begin(EepSt.Cppm.Modulation, max(CppmReader.detectedChannelNb(), EepSt.Cppm.ChannelNb), EepSt.Cppm.InitialPeriod_us, EepSt.Cppm.Header_us);
        DisplayCppmCharacteristics(INITIAL_CPPM_FRAME, CppmReader.detectedChannelNb());
        DisplayCppmCharacteristics(  FINAL_CPPM_FRAME, max(CppmReader.detectedChannelNb(), EepSt.Cppm.ChannelNb));
        CppmCaught = 1;
      }
      else
      {
        PRINT_P("No CPPM signal!\n");
        TryCnt++;
      }
    }
    while((TryCnt < CPPM_TRY_CNT_MAX) && !CppmCaught);
    if(TryCnt < CPPM_TRY_CNT_MAX)
    {
      DisplayVersionRevision();
      PRINT_P("CPPM signal OK!\n");
    }
  }
  RculDstInit();
  RculRxInit(RX_MODE_OFF, EepSt.Rx.Mode);
}

static void SetDefaultParameters(void)
{
  /* EEPROM not formatted */
  EepSt.Lang                  = LANG_UK;
  
  EepSt.Trainer.Mode          = TRAINER_MODE_PPM_OUT_PLUS_PPM_IN;
  
  EepSt.Cppm.Modulation       = CPPM_MODU_NEG;
  EepSt.Cppm.ChannelNb        = 4;
  EepSt.Cppm.Header_us        = 300;
  EepSt.Cppm.InitialPeriod_us = 22500;
  EepSt.Cppm.FinalPeriod_us   = 22500;
  EepSt.SbusSpeedMs           = SBUS_TX_NORMAL_FRAME_RATE_MS;
  EepSt.RculRepeatNb          = 0;
  for(uint8_t RculIdx = 0; RculIdx < RCUL_MAX_NB; RculIdx++)
  {
    EepSt.Rcul[RculIdx].Channel = 0;
  }
  EepSt.Rx.Mode = RX_MODE_PWM;
  EepSt.Rx.Ch   = 5;
  EEPROM.put(EEP_ST_LOCATION, EepSt);
}

static void RculRxInit(uint8_t PrevRxMode, uint8_t NewRxMode)
{
  switch(PrevRxMode)
  {
    case RX_MODE_OFF:
    switch(NewRxMode)
    {
      case RX_MODE_OFF:
#ifdef ARDUINO_AVR_NANO
      Wire.begin();
#endif
      break;

      case RX_MODE_PWM:
#ifdef ARDUINO_AVR_NANO
      Wire.end();
#endif
      PwmRcSignal.attach(A5);
      MyRcRxSerial.reassignRculSrc(&PwmRcSignal, RCUL_NO_CH);
      break;

      case RX_MODE_CPPM:
#ifdef ARDUINO_AVR_NANO
      Wire.end();
#endif
      TinyCppmReader.attach(A5);
      MyRcRxSerial.reassignRculSrc(&TinyCppmReader, EepSt.Rx.Ch);
      break;
    }
    break;

    case RX_MODE_PWM:
    switch(NewRxMode)
    {
      case RX_MODE_OFF:
      PwmRcSignal.detach();
      TinyPinChange_UnregisterIsr(A5, SoftRcPulseIn::SoftRcPulseInInterrupt1ISR);
#ifdef ARDUINO_AVR_NANO
      Wire.begin();
#endif
      break;

      case RX_MODE_CPPM:
      PwmRcSignal.detach();
      TinyPinChange_UnregisterIsr(A5, SoftRcPulseIn::SoftRcPulseInInterrupt1ISR);
      TinyCppmReader.attach(A5);
      MyRcRxSerial.reassignRculSrc(&TinyCppmReader, EepSt.Rx.Ch);
      break;

      default:
      break;
    }
    break;

    case RX_MODE_CPPM:
    switch(NewRxMode)
    {
      case RX_MODE_OFF:
      TinyCppmReader.detach(); /* TinyPinChange_UnregisterIsr() is done inside detach() for TinyCppmReader */
#ifdef ARDUINO_AVR_NANO
      Wire.begin();
#endif
      break;

      case RX_MODE_PWM:
      TinyCppmReader.detach(); /* TinyPinChange_UnregisterIsr() is done inside detach() for TinyCppmReader */
      PwmRcSignal.attach(A5);
      MyRcRxSerial.reassignRculSrc(&PwmRcSignal, RCUL_NO_CH);
      break;

      case RX_MODE_CPPM:
      MyRcRxSerial.reassignRculSrc(&TinyCppmReader, EepSt.Rx.Ch); /* In case of Channel change */
      break;

      default:
      break;
    }
    break;
  }
}

static void RculDstInit(uint8_t EnableSBus /*= 0*/)
{
  switch(EepSt.Trainer.Mode)
  {
    case TRAINER_MODE_PPM_OUT_PLUS_PPM_IN: /* No break: Normal */
    case TRAINER_MODE_PPM_IN:
    for(uint8_t Ch = 1; Ch <= CPPM_READER_CH_MAX; Ch++)
    {
      CppmGen.width_us(Ch, 1500);
    }
    RculDst = &CppmGen;
    /* Start the PPM frame generator with the same characteristics */
//    EepSt.Cppm.InitialPeriod_us = 25000; /* SHALL be greater than the CPPM frame generated by the transmitter */
    EepSt.Cppm.FinalPeriod_us   = CppmGen.begin(EepSt.Cppm.Modulation, max(CppmReader.detectedChannelNb(), EepSt.Cppm.ChannelNb), EepSt.Cppm.InitialPeriod_us, EepSt.Cppm.Header_us);    
    break;

    case TRAINER_MODE_SBUS_IN:
    for(uint8_t Ch = 1; Ch <= SBUS_TX_CH_NB; Ch++)
    {
      SBusTx.width_us(Ch, 1500);
    }
    RculDst = EnableSBus? &SBusTx: NULL;
    break;
  }
  for(uint8_t Idx = 0; Idx < RCUL_INSTANCE_MAX_NB; Idx++)
  {
    MyRcTxSerial[Idx].reassignRculDst(RculDst);
    MyRcTxSerial[Idx].setCh(!EepSt.Rcul[Idx].Channel? 255: EepSt.Rcul[Idx].Channel);
    MyRcTxSerial[Idx].setRepeatNb(EepSt.RculRepeatNb);
  }
}

static void DisplayHelp(void)
{
  PRINT_P("LANG=UK/FR                                 Set the current language of the Human Computer Interface\n");
  PRINT_P("TRAINER.MODE=PPM_OUT+PPM_IN/PPM_IN/SBUS_IN Set the Trainer mode of the transmitter\n");
  PRINT_P("CPPM.MODU=NEG/POS                          Set the PPM modulation\n");
  PRINT_P("CPPM.HEADER=xxx                            Set the PPM header (xxx in us)\n");
  PRINT_P("CPPM.CHANNEL.NB=xx                         Set the PPM channel number (xx from 1 to 12)\n");
  PRINT_P("CPPM.PERIOD=xxxxx                          Set the PPM period (xxxxx in us)\n");
  PRINT_P("SBUS.SPEED=NORMAL/FAST                     Set the SBUS speed (NORMAL=14ms, FAST=7ms)\n");
  PRINT_P("RCUL.REPEAT=x                              Set the number of repeatition (x from 0 to 3)\n");
  PRINT_P("RCULx.CHANNEL=y                            Set the channel y (y from 1 to 12) for the RCUL x instance (x from 1 to 4)\n");
  PRINT_P("RCULx.TEST=OFF/ON                          Set the sweep test mode for the RCUL x instance (x from 1 to 4)\n");
  PRINT_P("RX.MODE=OFF/PWM/CPPMx                      Set the RX mode to OFF, PWM or CPPMx (A5 input) (x is the channel number)\n");
  PRINT_P("RX.DBG=DebugLevel                          Set the RX debug level (A5 input) (Msg Level: DebugLevel=1, Nibble Level: DebugLevel=2)\n");
  PRINT_P("COMMAND?                                   Return the current value of the command (Eg: TRAINER.MODE?)\n");
  PRINT_P("\n");
}

static void DisplayAide(void)
{
  PRINT_P("LANG=FR/UK                                 Definit le langage courant de l'Interface Homme Machine\n");
  PRINT_P("ECOLAGE.MODE=PPM_OUT+PPM_IN/PPM_IN/SBUS_IN Definit the mode d'ecolage de l'emetteur\n");
  PRINT_P("CPPM.MODU=NEG/POS                          Definit la modulation PPM\n");
  PRINT_P("CPPM.ENTETE=xxx                            Definit l'entete PPM (xxx en us)\n");
  PRINT_P("CPPM.VOIE.NB=xx                            Definit le nombre de voie du train PPM (xx de 1 a 12)\n");
  PRINT_P("CPPM.PERIODE=xxxxx                         Definit la periode PPM (xxxxx en us)\n");
  PRINT_P("SBUS.VITESSE=NORMALE/RAPIDE                Definit la vitesse SBUS (NORMALE=14ms, RAPIDE=7ms)\n");
  PRINT_P("RCUL.REPET=x                               Definit le nombre de repetition (x de 0 a 3)\n");
  PRINT_P("RCULx.VOIE=y                               Definit la voie y (y de 1 a 12) de l'instance RCUL x (x de 1 a 4)\n");
  PRINT_P("RCULx.TEST=OFF/ON                          Definit le mode de test de balayage de l'instance RCUL x (x de 1 a 4)\n");
  PRINT_P("RX.MODE=OFF/PWM/CPPMx                      Definit le mode RX a OFF, PWM ou CPPMx (Entree A5) (x est le numero de voie)\n");
  PRINT_P("RX.DBG=NiveauDebug                         Definit le niveau de debug en RX (Entree A5) (Niveau Msg: NiveauDebug=1, Niveau quartet: NiveauDebug=2)\n");
  PRINT_P("COMMANDE?                                  Renvoie la valeur courante de la commande (Ex: ECOLAGE.MODE?)\n");
  PRINT_P("\n");
}

static void DisplayVersionRevision(void)
{
  PRINTF("%sBURC %sV%d.%d\n", (EepSt.Lang == LANG_UK)?"": "Codeur ", (EepSt.Lang == LANG_UK)?"Encoder ": "",SW_VERSION, SW_REVISION);
}

static void DisplayCppmCharacteristics(uint8_t InitialFinalCppm, uint8_t ChNb)
{
  char ChNbStr[3];
  uint8_t ActiveRcul = 0;
  Serial.println();
  if(InitialFinalCppm == INITIAL_CPPM_FRAME) PRINT_P("Initial"); else PRINT_P("Final");PRINT_P(" PPM characteristics:\n");
  if(InitialFinalCppm == INITIAL_CPPM_FRAME) PRINT_P("==");PRINT_P("=========================\n");
  PRINTF("-Modulation    = %s\n",(EepSt.Cppm.Modulation == CPPM_GEN_POS_MOD)? "POS": "NEG");
  PRINTF("-Channel Nb    = %d\n", ChNb);
  PRINTF("-Header (us)   = %d\n", EepSt.Cppm.Header_us);
  PRINTF("-Period (us)   = %d\n", (InitialFinalCppm == INITIAL_CPPM_FRAME)? EepSt.Cppm.InitialPeriod_us: EepSt.Cppm.FinalPeriod_us);
  if(InitialFinalCppm == FINAL_CPPM_FRAME)
  {
    PRINTF("-Tx Period (ms)= %d", RC_TX_BYTE_OPT_TIME_MS(EepSt.Cppm.FinalPeriod_us, EepSt.RculRepeatNb, sizeof(RcTxMsgSt_t)));
    for(uint8_t Idx = 0; Idx < RCUL_INSTANCE_MAX_NB; Idx++)
    {
      if(EepSt.Rcul[Idx].Channel && (EepSt.Rcul[Idx].Channel != 255))
      {
        PRINTF(" (RCUL%d message -> CH%d)", Idx + 1, EepSt.Rcul[Idx].Channel);
        ActiveRcul++;
      }
    }
    if(ActiveRcul) PRINT_P("\n");
  }
  itoa(ChNb, ChNbStr, 10);
  if(ChNb < 10) {ChNbStr[1] = ' ';ChNbStr[2] = 0;}
  Serial.println();
  if(InitialFinalCppm == INITIAL_CPPM_FRAME) PRINT_P("Initial");else PRINT_P("Final");PRINT_P(" PPM frame:\n");
  if(InitialFinalCppm == INITIAL_CPPM_FRAME) PRINT_P("==");PRINT_P("===============\n");
  if(Cppm.Modulation == CPPM_GEN_POS_MOD) PrintHeaders();else      PrintPulses();
  PRINTF("  |   |  CH1     |   |          |   |  CH%s    |   |      Synchro      |   |\n", ChNbStr);
  if(Cppm.Modulation == CPPM_GEN_POS_MOD) PrintPulses();else       PrintHeaders();
  PRINT_P("  <--->\n");
  PRINT_P("  Header\n");
  PRINT_P("  <------------------------------------------------------------------>\n");
  PRINT_P("                                Period\n");  
}

static void PrintHeaders(void)
{
  PRINT_P("   ---            ---            ---            ---                     ---\n");  
}

static void PrintPulses(void)
{
  PRINT_P("--     ----------     ----//----     ----------     -------------------\n");
}

void loop()
{
  static uint8_t  WrIdx = 0;
  int8_t          RxLen;

  /*************************************************/
  /* Terminal Message interpretation and execution */
  /*************************************************/  
  if((RxLen = GenCli_getCmd(&Serial, WrIdx, BUF_AND_BUF_SIZE(TxRxBuf))) >= 0)
  {
    InterpretAndExecute(TxRxBuf);
  }
  
  /**********************************/
  /* Maintain RC Frame transmission */
  /**********************************/
  MaintainRcFrame();
  
  /*****************************************/
  /* Additional Part: RCUL message loading */
  /*****************************************/
  for(uint8_t Idx = 0; Idx < RCUL_INSTANCE_MAX_NB; Idx++)
  {
    if(MyRcTxSerial[Idx].getCh() != 255)
    {
      if(MyRcTxSerial[Idx].isReadyForTx()) /* Maximum speed */
      {
        BuildContactMsg(Idx);
        MyRcTxSerial[Idx].write((char*)&RcTxMsg[Idx], sizeof(RcTxMsgSt_t));
        MyRcTxSerial[Idx].addChecksumToByteMsg();
        if(RcTxMsg[Idx].Contacts ^ PrevRcTxMsg[Idx].Contacts)
        {
          memcpy(&PrevRcTxMsg[Idx], &RcTxMsg[Idx], sizeof(RcTxMsgSt_t));
          Serial.println(F("          1111111"));
          Serial.print  (F("          6543210987654321\n"));
          Serial.print  (F("RCUL"));Serial.print(Idx + 1);Serial.print(F(".SW: "));
          for(uint8_t BitIdx = 0; BitIdx < CONTACT_NB; BitIdx++)
          {
            Serial.print(bitRead(HTONS(RcTxMsg[Idx].Contacts), 15 - BitIdx));
          }
          Serial.println();
        }
      }
    }
  }

  /******************************/
  /* Unstack characters to send */
  /******************************/
  RcTxSerial::process();

  /******************************/
  /* RX Part Debug (on UNO)     */
  /******************************/
  RxTest();

}

void RxTest(void)
{
  static uint32_t StartMs, LastMsgStartMs = millis();
  uint16_t        lastWidth_us;
  uint8_t         RxMsgAttr, RxByteLen, RxNibbleLen;

  if(bitRead(RxDbg, 0))
  {
    if((RxMsgAttr = MyRcRxSerial.msgAvailable((char *)&RxMsg, RX_MSG_LEN_MAX)))
    {
      RculStat.RxFrameCnt++;
      if(RcRxSerial::msgChecksumIsValid((char *)&RxMsg, RxMsgAttr))
      {
        Serial.println();
        RxByteLen   = (RxMsgAttr & ~RC_RX_SERIAL_PENDING_NIBBLE_INDICATOR);
        RxNibbleLen = (RxByteLen * 2);
        if(RxMsgAttr & RC_RX_SERIAL_PENDING_NIBBLE_INDICATOR)
        {
          RxNibbleLen++;
        }
        if(RxNibbleLen == (EXPECTED_NIBBLE_LEN + 2)) // +2 for Checksum
        {
          Serial.print(F("RCUL Msg[TotNbl="));Serial.print(RxNibbleLen);Serial.print(F("]="));PrintBin((uint8_t *)&RxMsg, (RxNibbleLen / 2) - 1, RxNibbleLen & 1);Serial.print(F(" T="));Serial.println(millis() - LastMsgStartMs);
          uint16_t Word;
          uint8_t *Ptr;
          Word = GetSwitch(RxMsg);
          Serial.print(F("Switch["));Serial.print("16");Serial.print(F("..1]="));
          Word = HTONS(Word);
          Ptr = (uint8_t *)&Word;
          PrintBin(Ptr, 2, 0);
          Serial.println();
          LastMsgStartMs = millis();
        }
        else
        {
          RculStat.ErrorCnt++;
          Serial.print(F("RCUL Length error!"));
          Serial.print(" ExpNibbleLen=");Serial.print(EXPECTED_NIBBLE_LEN + 2);
          Serial.print(" (RxNibbleLen =");Serial.print(RxNibbleLen);Serial.println(F(")"));
        }      
      }
      else
      {
          RculStat.ErrorCnt++;
          Serial.println(F("RCUL Length or Checksum error!"));
      }
      if(RculStat.RxFrameCnt >= 100)
      {
        Serial.println();
        Serial.println(F("**********************"));
        Serial.print(  F(" RCUL quality = "));Serial.print(RculStat.RxFrameCnt - RculStat.ErrorCnt);Serial.println(F(" %"));
        Serial.println(F("**********************"));
        RculStat.RxFrameCnt = 0;
        RculStat.ErrorCnt = 0;
      }
    }
  }

  if(bitRead(RxDbg, 1))
  {
    MyRcRxSerial.available();
    if(MyRcRxSerial.nibbleAvailable())
    {
      lastWidth_us = MyRcRxSerial.lastWidth_us();
      Serial.print(lastWidth_us);Serial.print(F(" -> "));
      Serial.write(NibbleIdx2Nibble(PulseWithToNibbleIdx(lastWidth_us - NEUTRAL_WIDTH_US)));Serial.print(" T=");Serial.print(millis() - StartMs);Serial.println();
      StartMs = millis();
    }
  }

}

static void MaintainRcFrame(void)
{
  uint8_t Idx;

  switch(EepSt.Trainer.Mode)
  {
    case TRAINER_MODE_PPM_OUT_PLUS_PPM_IN:
    /****************/
    /* Forward Part */
    /****************/
    if(CppmReader.isSynchro())
    {
      for(uint8_t Ch = 1; Ch <= CppmReader.detectedChannelNb(); Ch++)
      {
        for(Idx = 0; Idx < RCUL_INSTANCE_MAX_NB; Idx++)
        {
          if(EepSt.Rcul[Idx].Channel == Ch) break;
        }
        if(Idx >= RCUL_INSTANCE_MAX_NB)
        {
          CPPM_FORWARD_CH_AS_IS(Ch); /* Ch is not used by an Rcul instance -> Forward it as is */
        }
        /* The channels used by Rcul instance(s) will be sent by RcTxSerial::process() */
      }
    }
    break;
    
    case TRAINER_MODE_PPM_IN:
    /* Nothing to do: Tx is done in interrupt */
    break;
    
    case TRAINER_MODE_SBUS_IN:
    if(SBusStartMs16)
    {
      if(SBusStartMs16 == 1)
      {
        if(ElapsedMs16Since(SBusStartMs16) >= SBUS_START_DELAY_MS)
        {
          /* Reconfigure Serial Baud Rate for SBUS */
          SBusStartMs16 = 0;
          Serial.begin(100000, SERIAL_8E2);
          SBusTx.serialAttach(&Serial, EepSt.SbusSpeedMs? SBUS_TX_HIGH_SPEED_FRAME_RATE_MS: SBUS_TX_NORMAL_FRAME_RATE_MS);
          RculDstInit(1);
        }
      }
    }
    else
    {
      /* Send periodically the SBUS frame */
      if(SBusTx.isSynchro())
      {
        SBusTx.sendChannels(); /* SHALL be the last Synchro Client */
      }
      SBusTx.process(); /* SHALL be AFTER SBusTx.isSynchro() since SBusTx.sendChannels() will clear _Synchro */
    }
    break;

    default:
    /* Should not arrive here */
    break;
  }
}

static void BuildContactMsg(uint8_t RculIdx)
{
  for(uint8_t Id = 1; Id <= CONTACT_NB; Id++)
  {
    switch(RculIdx)
    {
      case 0:
      bitWrite(RcTxMsg[RculIdx].Contacts, Id - 1, !digitalRead(InputPin[Id])); /* ! because 1=open */
      break;
      
      case 1:
      bitWrite(RcTxMsg[RculIdx].Contacts, Id - 1, !digitalRead(InputPin[Id])); /* ! because 1=open */
      break;

      case 2:
      bitWrite(RcTxMsg[RculIdx].Contacts, Id - 1, !digitalRead(InputPin[Id])); /* ! because 1=open */
      break;
      
      case 3:
      bitWrite(RcTxMsg[RculIdx].Contacts, Id - 1, !digitalRead(InputPin[Id])); /* ! because 1=open */
      break;

      default:
      break;
    }
  }
  RcTxMsg[RculIdx].Contacts = HTONS(RcTxMsg[RculIdx].Contacts);
}

/* Command List */
enum {ACTION_NONE = 0, ACTION_ANSWER_WITH_REPONSE, ACTION_ANSWER_UNKNOWN_COMMAND, ACTION_ANSWER_OUT_OF_RANGE, ACTION_NOT_POSSIBLE_IN_THIS_CONTEXT, ACTION_ANSWER_ERROR};

#define ACTION()          (*Fld.Action)
#define ACTION_GET()      (*Fld.Action == '?')
#define ACTION_SET()      (*Fld.Action == '=')
#define ARG()             (Fld.Action + 1)

void InterpretAndExecute(char *TxRxBuf)
{
  GenCliCmdFldSt_t Fld;
  int8_t           CmdCodeIdx, ArgCodeIdx;
  uint8_t          Action = ACTION_ANSWER_WITH_REPONSE;
  uint8_t          RculId, RculRepeatNb, Ch;
  uint16_t         CppmHeader_us, Period_us;
  
  if(!*TxRxBuf) /* If Hit Enter (Empty command) */
  {
    if(EepSt.Trainer.Mode == TRAINER_MODE_SBUS_IN)
    {
      if((millis() < SBUS_START_DELAY_MS) && (SBusStartMs16 == 1))
      {
        if((EepSt.Lang == LANG_UK))
        {
          PRINT_P("SBUS frame generation suspended until next Enter hit or reboot!\n");
        }
        else
        {
          PRINT_P("Generation trame SBUS suspendue jusqu'au prochain appui sur Entree ou reboot!\n");
        }
        SBusStartMs16 = 2; /* Prevent changing Serial Baud rate to 100 000 bauds and generating SBUS frame */
      }
      else
      {
        if((millis() >= SBUS_START_DELAY_MS) && (SBusStartMs16 == 2))
        {
          if((EepSt.Lang == LANG_UK))
          {
            PRINT_P("SBUS frame generation resumed!\n");
          }
          else
          {
            PRINT_P("Reprise generation trame SBUS!\n");
          }
          delay(500);
          SBusStartMs16 = 1; /* Resume changing Serial Baud rate to 100 000 bauds and generating SBUS frame */
        }
      }
      return;
    }
  }
  CmdCodeIdx = GenCli_getCmdCodeIdx(TxRxBuf, CMD_FLD_SEPARATOR, CmdFldListTbl, TBL_AND_ITEM_NB(CmdTbl), &Fld, GEN_CLI_CMD_FIELD_MAX_NB);
  if(CmdCodeIdx < 0)
  {
    GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, -(CmdCodeIdx + 1), 0);
  }
  else
  {
//    Serial.print(F("CmdCodeIdx="));Serial.println(CmdCodeIdx);
//    GenInter_displayCmdParsedInFld(&Serial, &Fld);
/*
TRAINER.MODE=Mode (avec Mode: PPM_OUT+PPM_IN, PPM_IN, SBUS_IN)
RCULx.CHANNEL=y
RCULx.REPEAT=n
CPPM.MODU=Modu (avec Mode: POS ou NEG)
CPPM.HEADER=300
CPPM.CHANNEL.NB=4
CPPM.PERIOD=22500
*/

    switch(CmdCodeIdx)
    {
      case _CONF: /* Displays the full configuration */
      strcpy_P(TxRxBuf, _VERSION_);
      InterpretAndExecute(TxRxBuf);
      strcpy_P(TxRxBuf, _LANG_);
      InterpretAndExecute(TxRxBuf);
      strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("TRAINER.MODE"): PSTR("ECOLAGE.MODE"));
      InterpretAndExecute(TxRxBuf);
      if(EepSt.Trainer.Mode < TRAINER_MODE_SBUS_IN)
      {
        strcpy_P(TxRxBuf, PSTR("CPPM?"));
        InterpretAndExecute(TxRxBuf);
      }
      else
      {
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("SBUS.SPEED?"): PSTR("SBUS.VITESSE?"));
        InterpretAndExecute(TxRxBuf);
      }
      strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("RCUL.REPEAT?"): PSTR("RCUL.REPET?"));
      InterpretAndExecute(TxRxBuf);
      for(uint8_t RculId = 1; RculId <= RCUL_INSTANCE_MAX_NB; RculId++)
      {
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("RCUL_.CHANNEL?"): PSTR("RCUL_.VOIE?"));
        TxRxBuf[4] = '0' + RculId;
        InterpretAndExecute(TxRxBuf);
        strcpy_P(TxRxBuf, PSTR("RCUL_.TEST?"));
        TxRxBuf[4] = '0' + RculId;
        InterpretAndExecute(TxRxBuf);
      }
      strcpy_P(TxRxBuf, PSTR("RX.MODE?"));
      InterpretAndExecute(TxRxBuf);
      strcpy_P(TxRxBuf, PSTR("RX.DBG?"));
      InterpretAndExecute(TxRxBuf);
      PRINT_P("\n");
      Action = ACTION_NONE;
      break;
      
      case _HELP:
      case _AIDE:
      if(EepSt.Lang == LANG_UK) DisplayHelp();
      else                      DisplayAide();
      Action = ACTION_NONE;
      break;
      
      case _VERSION:
      DisplayVersionRevision();
      Action = ACTION_NONE;
      break;
      
      case _LANG:
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(LangTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < LANG_MAX_NB))
        {
          EepSt.Lang = ArgCodeIdx;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }
      }
      else
      {
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(LangTbl), EepSt.Lang, ARG(), 20);
      }
      break;
      
      case _TRAINER_MODE:
      case _ECOLAGE_MODE:
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(TrainerModeTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < TRAINER_MODE_MAX_NB))
        {
          EepSt.Trainer.Mode = ArgCodeIdx;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          if(EepSt.Trainer.Mode == TRAINER_MODE_SBUS_IN)
          {
            SBusStartMs16 = 1; /* The timer will call RculDstInit(1) */
          }
          else
          {
            RculDstInit();
          }
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }
      }
      else
      {
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(TrainerModeTbl), EepSt.Trainer.Mode, ARG(), 20);
      }
      break;
      
      case _RCUL:
      RculId = Fld.Id[0];
      if((RculId < 1) || (RculId > RCUL_MAX_NB))
      {
        GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ID);
        return;
      }
      if(ACTION_GET())
      {
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("RCUL.REPEAT?"): PSTR("RCUL.REPET?"));
        InterpretAndExecute(TxRxBuf);
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("RCUL_.CHANNEL"): PSTR("RCUL_.VOIE"));
        TxRxBuf[4] = '0' + RculId;
        InterpretAndExecute(TxRxBuf);
        Action = ACTION_NONE;
      }
      break;
      
      case _RCUL_CHANNEL:
      case _RCUL_VOIE:
      RculId = Fld.Id[0];
      if((RculId < 1) || (RculId > RCUL_MAX_NB))
      {
        GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ID);
        return;
      }
      if(ACTION_SET())
      {
        if(!strcmp_P(ARG(), PSTR("OFF"))) Ch = 0;
        else                              Ch = atoi(ARG());
        if(Ch > CH_MAX_NB) /* 0 is used to disable RCUL */
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          return;
        }
        EepSt.Rcul[RculId - 1].Channel = Ch;
        EEPROM.put(EEP_ST_LOCATION, EepSt);
        RculDstInit();
        ACTION() = 0;
      }
      else
      {
        ACTION() = '=';
        if(!EepSt.Rcul[RculId - 1].Channel || (EepSt.Rcul[RculId - 1].Channel == 255)) strcpy_P(ARG(),  PSTR("OFF"));
        else                                                                           sprintf_P(ARG(), PSTR("%u"), EepSt.Rcul[RculId - 1].Channel);
      }
      break;
      
      case _RCUL_REPEAT:
      case _RCUL_REPET:
      RculId = Fld.Id[0];
      if(RculId)
      {
        GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ID);
        return;
      }
      if(ACTION_SET())
      {
        RculRepeatNb = atoi(ARG());
        if(RculRepeatNb > RCUL_REPEAT_MAX_NB)
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          return;
        }
        EepSt.RculRepeatNb = RculRepeatNb;
        EEPROM.put(EEP_ST_LOCATION, EepSt);
        RculDstInit();
        ACTION() = 0;
      }
      else
      {
        ACTION() = '=';
        sprintf_P(ARG(), PSTR("%u"), EepSt.RculRepeatNb);
      }
      break;

      case _RCUL_TEST:
      RculId = Fld.Id[0];
      if((RculId < 1) || (RculId > RCUL_MAX_NB))
      {
        GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ID);
        return;
      }
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(RculTestTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < RCUL_TEST_MAX_NB))
        {
          MyRcTxSerial[RculId - 1].setSweepTest(ArgCodeIdx);
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }
      }
      else
      {
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(RculTestTbl), MyRcTxSerial[RculId - 1].getSweepTest(), ARG(), 20);
      }
      break;

      case _CPPM:
      if(ACTION_GET())
      {
        strcpy_P(TxRxBuf, PSTR("CPPM.MODU?"));
        InterpretAndExecute(TxRxBuf);
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("CPPM.HEADER?"): PSTR("CPPM.ENTETE?"));
        InterpretAndExecute(TxRxBuf);
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("CPPM.CHANNEL.NB?"): PSTR("CPPM.VOIE.NB?"));
        InterpretAndExecute(TxRxBuf);
        strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("CPPM.PERIOD?"): PSTR("CPPM.PERIODE?"));
        InterpretAndExecute(TxRxBuf);
        Action = ACTION_NONE;
      }
      break;
      
      case _CPPM_MODU:
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(CppmModuTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < CPPM_MODU_MAX_NB))
        {
          EepSt.Cppm.Modulation = ArgCodeIdx;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          EepSt.Cppm.FinalPeriod_us = CppmGen.begin(EepSt.Cppm.Modulation, EepSt.Cppm.ChannelNb, EepSt.Cppm.InitialPeriod_us, EepSt.Cppm.Header_us);    
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }
      }
      else
      {
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(CppmModuTbl), EepSt.Cppm.Modulation, ARG(), 20);
      }
      break;
      
      case _CPPM_HEADER:
      case _CPPM_ENTETE:
      if(ACTION_SET())
      {
        CppmHeader_us = atoi(ARG());
        if((CppmHeader_us >= 100) && (CppmHeader_us <= 500))
        {
          EepSt.Cppm.Header_us = CppmHeader_us;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          EepSt.Cppm.FinalPeriod_us = CppmGen.begin(EepSt.Cppm.Modulation, EepSt.Cppm.ChannelNb, EepSt.Cppm.InitialPeriod_us, EepSt.Cppm.Header_us);    
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }        
      }
      else
      {
        ACTION() = '=';
        sprintf_P(ARG(), PSTR("%u"), EepSt.Cppm.Header_us);
      }
      break;
      
      case _CPPM_CHANNEL_NB:
      case _CPPM_VOIE_NB:
      if(ACTION_SET())
      {
        Ch = atoi(ARG());
        if((Ch >= 1) && (Ch <= CH_MAX_NB))
        {
          EepSt.Cppm.ChannelNb = Ch;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          EepSt.Cppm.FinalPeriod_us = CppmGen.begin(EepSt.Cppm.Modulation, EepSt.Cppm.ChannelNb, EepSt.Cppm.InitialPeriod_us, EepSt.Cppm.Header_us);    
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }        
      }
      else
      {
        ACTION() = '=';
        sprintf_P(ARG(), PSTR("%u"), EepSt.Cppm.ChannelNb);
      }
      break;
      
      case _CPPM_PERIOD:
      case _CPPM_PERIODE:
      if(ACTION_SET())
      {
        Period_us = atoi(ARG());
        if((Period_us >= CPPM_PERIOD_US_FOR_CH_NB(4)) && (Period_us <= CPPM_PERIOD_US_FOR_CH_NB(12)))
        {
          EepSt.Cppm.InitialPeriod_us = Period_us;
          /* CppmGen.begin() may increases the FinalPeriod_us if InitialPeriod_us too small */
          EepSt.Cppm.FinalPeriod_us = CppmGen.begin(EepSt.Cppm.Modulation, EepSt.Cppm.ChannelNb, EepSt.Cppm.InitialPeriod_us, EepSt.Cppm.Header_us);    
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }        
      }
      else
      {
        ACTION() = '=';
        sprintf_P(ARG(), PSTR("%u"), EepSt.Cppm.FinalPeriod_us);
      }
      break;

      case _CPPM_INITIAL:
      DisplayCppmCharacteristics(INITIAL_CPPM_FRAME, CppmReader.detectedChannelNb());
      Action = ACTION_NONE;
      break;

      case _CPPM_FINAL:
      DisplayCppmCharacteristics(FINAL_CPPM_FRAME, EepSt.Cppm.ChannelNb);
      Action = ACTION_NONE;
      break;

      case _SBUS_SPEED:
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(SbusSpeedUkTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < SBUS_SPEED_NB))
        {
          EepSt.SbusSpeedMs = ArgCodeIdx? SBUS_TX_HIGH_SPEED_FRAME_RATE_MS: SBUS_TX_NORMAL_FRAME_RATE_MS;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          RculDstInit();
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }
      }
      else
      {
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(SbusSpeedUkTbl), (EepSt.SbusSpeedMs == SBUS_TX_HIGH_SPEED_FRAME_RATE_MS), ARG(), 20);
      }
      break;

      case _SBUS_VITESSE:
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(SbusSpeedFrTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < SBUS_SPEED_NB))
        {
          EepSt.SbusSpeedMs = ArgCodeIdx? SBUS_TX_HIGH_SPEED_FRAME_RATE_MS: SBUS_TX_NORMAL_FRAME_RATE_MS;
          EEPROM.put(EEP_ST_LOCATION, EepSt);
          RculDstInit();
          ACTION() = 0;
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }
      }
      else
      {
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(SbusSpeedFrTbl), (EepSt.SbusSpeedMs == SBUS_TX_HIGH_SPEED_FRAME_RATE_MS), ARG(), 20);
      }
      break;

      case _RX_MODE:
      if(ACTION_SET())
      {
        ArgCodeIdx = GetKeywordIdFromTbl(ARG(), TBL_AND_ITEM_NB(RxModeTbl));
        if((ArgCodeIdx >= 0) && (ArgCodeIdx < RX_MODE_MAX_NB))
        {
          if(ArgCodeIdx == RX_MODE_CPPM)
          {
            uint8_t Ch;
            Ch = atoi(ARG() + 4);
            if(!Ch || (Ch > 8))
            {
              GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
              Action = ACTION_NONE;
            }
            else
            {
              EepSt.Rx.Ch = Ch;
            }
          }
          if(Action == ACTION_ANSWER_WITH_REPONSE)
          {
            /* All is OK */
            RculRxInit(EepSt.Rx.Mode, ArgCodeIdx);
            EepSt.Rx.Mode = ArgCodeIdx;
            EEPROM.put(EEP_ST_LOCATION, EepSt);
            ACTION() = 0;
          }
        }
        else
        {
          GenCli_displayCmdErr(&Serial, TxRxBuf, &Fld, 0, GEN_CLI_ERR_ARG);
          Action = ACTION_NONE;
        }       
      }
      else
      {
        char ChStr[4];
        ChStr[0] = 0; // End of string
        if(EepSt.Rx.Mode == RX_MODE_CPPM) sprintf_P(ChStr, PSTR("%u"), EepSt.Rx.Ch);
        ACTION() = '=';
        GetKeywordFromTbl(TBL_AND_ITEM_NB(RxModeTbl), EepSt.Rx.Mode, ARG(), 20);
        if(EepSt.Rx.Mode == RX_MODE_CPPM) strcat(ARG(), ChStr);
      }
      break;

      case _RX_DBG:
      if(ACTION_SET())
      {
        RxDbg = (uint8_t)atoi(ARG());
        ACTION() = 0;        
      }
      else
      {
        ACTION() = '=';
        sprintf_P(ARG(), PSTR("%u"), RxDbg);
      }
      break;

      default:
      break;
    }
    switch(Action)
    {
      case ACTION_NONE: /* Keep quiet */
      break;
      
      case ACTION_ANSWER_WITH_REPONSE:
//      strcat(TxRxBuf, "\n"); /* Just to have a carriage return */
      break;
  
      case ACTION_ANSWER_UNKNOWN_COMMAND:
      strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("UNKNOWN CMD"):  PSTR("CMD INCONNUE"));
      break;
      
      case ACTION_ANSWER_OUT_OF_RANGE:
      strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("OUT OF RANGE"): PSTR("HORS LIMITE"));
      break;
  
      case ACTION_NOT_POSSIBLE_IN_THIS_CONTEXT:
      strcpy_P(TxRxBuf, (EepSt.Lang == LANG_UK)? PSTR("CONTEXT ERR"): PSTR("ERR CONTEXTE"));
      break;
      
      case ACTION_ANSWER_ERROR:
      strcpy_P(TxRxBuf, PSTR("ERR"));
      break;
    }
    if(Action != ACTION_NONE)
    {
      Serial.print(TxRxBuf);Serial.print(F("\n"));
    }
  }
}

int8_t PulseWithToNibbleIdx(int16_t ExcursionWidth)
{
  int8_t  Ret = -1;
  uint8_t Idx;

  for(Idx = 0; Idx < TABLE_ITEM_NBR(ExcursionHalf_us); Idx++)
  {
    if( (ExcursionWidth >= ((GET_EXC_HALF_US(Idx)/2) - 28)) && (ExcursionWidth <= ((GET_EXC_HALF_US(Idx)/2) + 28)) )
    {
      Ret = Idx;
      break;
    }
  }
  return(Ret);
}

uint8_t NibbleIdx2Nibble(uint8_t NibbleIdx)
{
  uint8_t Nibble;
  
  switch(NibbleIdx)
  {      
    case 16:
    Nibble = 'R';
    break;
    
    case 17:
    Nibble = 'I';
    break;
    
    default:
    if(NibbleIdx < 10)
    {
      Nibble = '0' + NibbleIdx;
    }
    else
    {
      Nibble = 'A' - 10 + NibbleIdx;
    }      
    break;
  }
  return(Nibble);
}

void PrintBin(uint8_t *Buf, uint8_t BufSize, uint8_t RemainingNibble/* = 0*/)
{
  uint8_t ByteIdx;
  for(ByteIdx =0; ByteIdx < BufSize; ByteIdx++)
  {
    if(ByteIdx) Serial.print(F("."));
    PrintByteBin(Buf[ByteIdx]);
  }
  if(RemainingNibble)
  {
    if(BufSize) Serial.print(F("."));
    PrintByteBin(Buf[ByteIdx], RemainingNibble);
  }
}

void PrintByteBin(uint8_t Byte, uint8_t RemainingNibble/* = 0*/)
{
  for(uint8_t Idx = 0; Idx < 8; Idx++)
  {
    Serial.print(bitRead(Byte, 7 - Idx));
    if(Idx == 3)
    {
      if(RemainingNibble) break;
      Serial.print(F("."));
    }
  }
}

uint16_t GetSwitch(uint8_t *RxMsg)
{
  uint16_t Sw = 0;

  /* Sw.16 */
    Sw |= RxMsg[0] << 8;
    Sw |= RxMsg[1];

  return(Sw);
}
