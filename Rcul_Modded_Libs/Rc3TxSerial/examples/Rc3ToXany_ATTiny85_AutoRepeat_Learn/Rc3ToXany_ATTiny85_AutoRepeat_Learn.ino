/*
  Rc3ToXany v1.2 - ATTiny85
  =========================

  Transparent RC3 -> historical RcTxSerial PWM bridge.

  Hardware (existing XANY2MSX adapter):
    PB2 / physical pin 7 : RC3 input (J1 signal through R1)
    PB0 / physical pin 5 : historical RcSerial output (J2 / N1)
    PB1 / physical pin 6 : LEARN input, jumper to GND at power-up
    PB5 / RESET          : NOT USED - remains RESET/ISP
    PB3/PB4              : 16 MHz crystal

  LEARN:
    - normal boot: PB1 open -> internal pull-up -> EEPROM calibration loaded;
    - learn boot : PB1 strapped to GND -> automatic 3-level measurement;
    - if Learn succeeds, 3 centers are stored in EEPROM;
    - if Learn fails, the previous EEPROM calibration is NOT overwritten;
    - if EEPROM has no valid calibration, defaults are 1000/1500/2000 us.

  The bridge is TRANSPARENT to message semantics:
    - it does NOT know SW8, SW16, PROP or ANGLE;
    - Rc3TxSerial V1.1 carries only NibbleNb + packed legacy bytes;
    - this ATTiny reconstructs the historical checksum and PWM symbols;
    - the old receiver firmware can remain unchanged:
          #include <RcRxSerial.h>

  Required libraries:
    RcNavyRcul
    RcNavyTinyPinChange
    RcNavySoftRcPulseIn  V1.8
    RcNavySoftRcPulseOut
    Rc3RxSerial          V1.1 (AUTO Repeat)
    EEPROM               (Arduino AVR core)

  ATTiny85 clock:
    external 16 MHz crystal, matching the existing PCB.
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <Rcul.h>
#include <TinyPinChange.h>
#include <SoftRcPulseIn.h>
#include <SoftRcPulseOut.h>
#include <Rc3RxSerial.h>

/* Existing PCB pinout. */
#define RC3_INPUT_PIN                 2   /* PB2, DIP pin 7 */
#define RCSERIAL_OUTPUT_PIN           0   /* PB0, DIP pin 5 */
#define LEARN_INPUT_PIN               1   /* PB1, DIP pin 6: jumper to GND */

/*
  Rc3RxSerial V1.1 detects RepeatNb automatically from each physical SYNC.
  The ATTiny application does not configure, store or know RepeatNb.
*/

/*
  Wide physical capture range.
  The actual 3 RC3 levels are learned inside this interval.
*/
#define RC3_CAPTURE_MIN_US            700U
#define RC3_CAPTURE_MAX_US            2300U

/* Default centers when no valid EEPROM calibration exists. */
#define RC3_DEFAULT_LEVEL0_US         1000U
#define RC3_DEFAULT_LEVEL1_US         1500U
#define RC3_DEFAULT_LEVEL2_US         2000U

/*
  Dead zone around the midpoint between two learned levels.
  Example with 1000/1500/2000:
    threshold midpoints = 1250 / 1750 us
    windows become roughly:
      0 = 700..1210
      1 = 1290..1710
      2 = 1790..2300
*/
#define RC3_WINDOW_GUARD_US           40U

/* Sanity check for a learned calibration. */
#define RC3_LEARN_MIN_LEVEL_GAP_US    250U

/*
  Automatic Learn histogram:
    700..2300 us in 20 us bins -> 81 bytes of RAM.
*/
#define RC3_LEARN_TIME_MS             8000UL
#define RC3_LEARN_BIN_US              20U
#define RC3_LEARN_BIN_NB              (((RC3_CAPTURE_MAX_US - RC3_CAPTURE_MIN_US) / RC3_LEARN_BIN_US) + 1U)
#define RC3_LEARN_MIN_PEAK_SEP_BINS   8U    /* 160 us */
#define RC3_LEARN_MIN_PEAK_COUNT      6U
#define RC3_LEARN_AVG_RADIUS_BINS     2U    /* +/- 40 us around each peak */

/*
  EEPROM layout: 10 bytes
    0..1 : magic 0xC3A5
    2    : format version
    3..4 : level 0 center
    5..6 : level 1 center
    7..8 : level 2 center
    9    : CRC8 over bytes 0..8
*/
#define RC3_EEPROM_ADDR               0
#define RC3_EEPROM_MAGIC              0xC3A5U
#define RC3_EEPROM_VERSION            1U
#define RC3_EEPROM_SIZE               10U

/*
  Historical RcTxSerial physical-symbol repetition.

  2 means each historical symbol is presented 3 times, matching the robust
  RcTxSerial-style setting and also allowing RcRxSerial FILTER0/FILTER1/FILTER2.
*/
#define LEGACY_REPEAT_NB              2

/* Historical RcTxSerial symbol indexes. */
enum
{
  LEGACY_NIBBLE_0 = 0,
  LEGACY_NIBBLE_F = 15,
  LEGACY_NIBBLE_R = 16,
  LEGACY_NIBBLE_I = 17
};

#define LEGACY_MAX_USEFUL_NIBBLES     6
#define LEGACY_MAX_OUTPUT_NIBBLES     (LEGACY_MAX_USEFUL_NIBBLES + 2)

/* Same compatibility envelope as Rc3TxSerial V1.1. */
#define RC3_COMPAT_MAGIC_MASK         0xE0
#define RC3_COMPAT_MAGIC              0xA0
#define RC3_COMPAT_CHECKSUM_BIT       0x10
#define RC3_COMPAT_NIBBLE_MASK        0x0F

/*
  Exact historical RcTxSerial centers:
    0=1024, 1=1080, ... F=1864, R=1920, I=1976 us.
*/
#define LEGACY_NEUTRAL_US             1500
#define LEGACY_SYMBOL_WIDTH_US        56
#define LEGACY_SYMBOL_NB              18
#define LEGACY_FULL_EXCURSION_US      (LEGACY_SYMBOL_WIDTH_US * LEGACY_SYMBOL_NB)
#define LEGACY_PULSE_MIN_US           (LEGACY_NEUTRAL_US - (LEGACY_FULL_EXCURSION_US / 2))
#define LEGACY_PULSE_US(Index)        \
  (uint16_t)(LEGACY_PULSE_MIN_US + (LEGACY_SYMBOL_WIDTH_US / 2) + ((uint16_t)(Index) * LEGACY_SYMBOL_WIDTH_US))

static SoftRcPulseIn  Rc3PulseIn;
/*
  Rc3RxSerial v1.1:
  RepeatNb is AUTO by default inside the library.
  The sketch does not configure or know the repetition count.
*/
static Rc3RxSerial Rc3Rx(&Rc3PulseIn);

static SoftRcPulseOut RcSerialOut;

/* Current RC3 physical centers, loaded from EEPROM or defaults. */
static uint16_t Rc3LevelUs[3] =
{
  RC3_DEFAULT_LEVEL0_US,
  RC3_DEFAULT_LEVEL1_US,
  RC3_DEFAULT_LEVEL2_US
};

/* Learn histogram: global/static to avoid using ATTiny stack RAM. */
static uint8_t Rc3LearnHistogram[RC3_LEARN_BIN_NB];

/* Fully reconstructed logical legacy nibble message, checksum included. */
static uint8_t LegacyNibbles[LEGACY_MAX_OUTPUT_NIBBLES];
static uint8_t LegacyNibbleCount = 0;
static uint8_t LegacyNibblePos = 0;

/* Historical physical symbol repetition state. */
static uint8_t LegacyPhysicalSymbol = LEGACY_NIBBLE_I;
static uint8_t LegacyPrevPhysicalSymbol = LEGACY_NIBBLE_I;
static uint8_t LegacyRepeatLeft = 0;
static uint8_t LegacyMessageActive = 0;

/* Diagnostics available in a debugger; no LED and no serial are required. */
static volatile uint16_t Rc3CompatGood = 0;
static volatile uint16_t Rc3CompatIgnored = 0;
static volatile uint16_t LegacyMessagesStarted = 0;
static volatile uint8_t  Rc3LearnRequested = 0;
static volatile uint8_t  Rc3LearnSucceeded = 0;

/* ------------------------------------------------------------------ */
/* EEPROM + RC3 LEARN                                                  */
/* ------------------------------------------------------------------ */

static uint8_t rc3Crc8Update(uint8_t Crc, uint8_t Data)
{
  Crc ^= Data;

  for(uint8_t Bit = 0; Bit < 8; Bit++)
  {
    if(Crc & 0x80U)
      Crc = (uint8_t)((Crc << 1) ^ 0x07U);
    else
      Crc <<= 1;
  }

  return Crc;
}

static uint8_t rc3LevelsAreValid(uint16_t L0,
                                 uint16_t L1,
                                 uint16_t L2)
{
  if((L0 < RC3_CAPTURE_MIN_US) ||
     (L2 > RC3_CAPTURE_MAX_US))
  {
    return 0;
  }

  if((L1 <= L0) || (L2 <= L1))
  {
    return 0;
  }

  if(((uint16_t)(L1 - L0) < RC3_LEARN_MIN_LEVEL_GAP_US) ||
     ((uint16_t)(L2 - L1) < RC3_LEARN_MIN_LEVEL_GAP_US))
  {
    return 0;
  }

  return 1;
}

static void rc3UseDefaultLevels(void)
{
  Rc3LevelUs[0] = RC3_DEFAULT_LEVEL0_US;
  Rc3LevelUs[1] = RC3_DEFAULT_LEVEL1_US;
  Rc3LevelUs[2] = RC3_DEFAULT_LEVEL2_US;
}

static uint8_t rc3LoadLevelsFromEeprom(void)
{
  uint8_t Data[9];

  for(uint8_t Idx = 0; Idx < sizeof(Data); Idx++)
  {
    Data[Idx] = EEPROM.read(RC3_EEPROM_ADDR + Idx);
  }

  uint8_t Crc = 0;
  for(uint8_t Idx = 0; Idx < sizeof(Data); Idx++)
  {
    Crc = rc3Crc8Update(Crc, Data[Idx]);
  }

  if(Crc != EEPROM.read(RC3_EEPROM_ADDR + 9))
  {
    return 0;
  }

  const uint16_t Magic =
      (uint16_t)Data[0] |
      ((uint16_t)Data[1] << 8);

  if((Magic != RC3_EEPROM_MAGIC) ||
     (Data[2] != RC3_EEPROM_VERSION))
  {
    return 0;
  }

  const uint16_t L0 =
      (uint16_t)Data[3] |
      ((uint16_t)Data[4] << 8);

  const uint16_t L1 =
      (uint16_t)Data[5] |
      ((uint16_t)Data[6] << 8);

  const uint16_t L2 =
      (uint16_t)Data[7] |
      ((uint16_t)Data[8] << 8);

  if(!rc3LevelsAreValid(L0, L1, L2))
  {
    return 0;
  }

  Rc3LevelUs[0] = L0;
  Rc3LevelUs[1] = L1;
  Rc3LevelUs[2] = L2;

  return 1;
}

static void rc3SaveLevelsToEeprom(uint16_t L0,
                                  uint16_t L1,
                                  uint16_t L2)
{
  uint8_t Data[9];

  Data[0] = (uint8_t)(RC3_EEPROM_MAGIC & 0xFFU);
  Data[1] = (uint8_t)(RC3_EEPROM_MAGIC >> 8);
  Data[2] = RC3_EEPROM_VERSION;

  Data[3] = (uint8_t)(L0 & 0xFFU);
  Data[4] = (uint8_t)(L0 >> 8);
  Data[5] = (uint8_t)(L1 & 0xFFU);
  Data[6] = (uint8_t)(L1 >> 8);
  Data[7] = (uint8_t)(L2 & 0xFFU);
  Data[8] = (uint8_t)(L2 >> 8);

  uint8_t Crc = 0;

  for(uint8_t Idx = 0; Idx < sizeof(Data); Idx++)
  {
    Crc = rc3Crc8Update(Crc, Data[Idx]);
    EEPROM.update(RC3_EEPROM_ADDR + Idx, Data[Idx]);
  }

  EEPROM.update(RC3_EEPROM_ADDR + 9, Crc);
}

/*
  Convert the 3 learned centers into 3 non-overlapping Rc3RxSerial windows.

  Windows are separated by a small dead-zone around each midpoint.
  This keeps the decoder tolerant to normal receiver jitter while avoiding
  ambiguous classification.
*/
static void rc3ApplyPulseWindows(void)
{
  const uint16_t Mid01 =
      (uint16_t)((Rc3LevelUs[0] + Rc3LevelUs[1]) / 2U);

  const uint16_t Mid12 =
      (uint16_t)((Rc3LevelUs[1] + Rc3LevelUs[2]) / 2U);

  uint16_t W0Max =
      (Mid01 > RC3_WINDOW_GUARD_US) ?
      (uint16_t)(Mid01 - RC3_WINDOW_GUARD_US) :
      Mid01;

  uint16_t W1Min =
      (uint16_t)(Mid01 + RC3_WINDOW_GUARD_US);

  uint16_t W1Max =
      (Mid12 > RC3_WINDOW_GUARD_US) ?
      (uint16_t)(Mid12 - RC3_WINDOW_GUARD_US) :
      Mid12;

  uint16_t W2Min =
      (uint16_t)(Mid12 + RC3_WINDOW_GUARD_US);

  if(W0Max > RC3_CAPTURE_MAX_US) W0Max = RC3_CAPTURE_MAX_US;
  if(W1Min < RC3_CAPTURE_MIN_US) W1Min = RC3_CAPTURE_MIN_US;
  if(W1Max > RC3_CAPTURE_MAX_US) W1Max = RC3_CAPTURE_MAX_US;
  if(W2Min < RC3_CAPTURE_MIN_US) W2Min = RC3_CAPTURE_MIN_US;

  Rc3Rx.setPulseWindows(
      RC3_CAPTURE_MIN_US, W0Max,
      W1Min,              W1Max,
      W2Min,              RC3_CAPTURE_MAX_US);
}

static uint16_t rc3LearnWeightedCenter(uint8_t PeakBin)
{
  uint32_t WeightedSum = 0;
  uint16_t SampleSum = 0;

  int16_t First =
      (int16_t)PeakBin - (int16_t)RC3_LEARN_AVG_RADIUS_BINS;

  int16_t Last =
      (int16_t)PeakBin + (int16_t)RC3_LEARN_AVG_RADIUS_BINS;

  if(First < 0) First = 0;
  if(Last >= (int16_t)RC3_LEARN_BIN_NB)
    Last = (int16_t)RC3_LEARN_BIN_NB - 1;

  for(int16_t Bin = First; Bin <= Last; Bin++)
  {
    const uint8_t Count = Rc3LearnHistogram[Bin];

    if(Count)
    {
      const uint16_t WidthUs =
          (uint16_t)(RC3_CAPTURE_MIN_US +
                     ((uint16_t)Bin * RC3_LEARN_BIN_US));

      WeightedSum +=
          (uint32_t)WidthUs * (uint32_t)Count;

      SampleSum += Count;
    }
  }

  if(!SampleSum)
  {
    return (uint16_t)(RC3_CAPTURE_MIN_US +
                      ((uint16_t)PeakBin * RC3_LEARN_BIN_US));
  }

  return (uint16_t)(WeightedSum / SampleSum);
}

/*
  Automatic 3-level Learn.

  The transmitter does NOT need to know that Learn is running.
  We simply observe the actual RC3 traffic for RC3_LEARN_TIME_MS and find
  the three strongest pulse-width populations.

  Returns 1 only when 3 sufficiently separated valid centers are found.
*/
static uint8_t rc3RunLearn(void)
{
  memset(Rc3LearnHistogram, 0, sizeof(Rc3LearnHistogram));

  const uint32_t StartMs = millis();

  while((uint32_t)(millis() - StartMs) < RC3_LEARN_TIME_MS)
  {
    if(Rc3PulseIn.available())
    {
      const uint16_t WidthUs = Rc3PulseIn.width_us();

      /*
        Keep the downstream legacy receiver safely in Idle during Learn.
        We emit one Idle frame for each valid incoming RC pulse.
      */
      RcSerialOut.write_us(
          LEGACY_PULSE_US(LEGACY_NIBBLE_I));

      SoftRcPulseOut::refresh(1);

      if((WidthUs >= RC3_CAPTURE_MIN_US) &&
         (WidthUs <= RC3_CAPTURE_MAX_US))
      {
        uint16_t Offset =
            (uint16_t)(WidthUs - RC3_CAPTURE_MIN_US);

        uint8_t Bin =
            (uint8_t)((Offset + (RC3_LEARN_BIN_US / 2U)) /
                      RC3_LEARN_BIN_US);

        if(Bin >= RC3_LEARN_BIN_NB)
        {
          Bin = RC3_LEARN_BIN_NB - 1U;
        }

        if(Rc3LearnHistogram[Bin] != 0xFFU)
        {
          Rc3LearnHistogram[Bin]++;
        }
      }
    }
  }

  int16_t Peak[3] = {-1, -1, -1};

  for(uint8_t P = 0; P < 3; P++)
  {
    uint8_t BestCount = 0;
    int16_t BestBin = -1;

    for(uint8_t Bin = 0; Bin < RC3_LEARN_BIN_NB; Bin++)
    {
      uint8_t FarEnough = 1;

      for(uint8_t Prev = 0; Prev < P; Prev++)
      {
        int16_t Delta =
            (int16_t)Bin - Peak[Prev];

        if(Delta < 0) Delta = -Delta;

        if(Delta < (int16_t)RC3_LEARN_MIN_PEAK_SEP_BINS)
        {
          FarEnough = 0;
          break;
        }
      }

      if(FarEnough &&
         (Rc3LearnHistogram[Bin] > BestCount))
      {
        BestCount = Rc3LearnHistogram[Bin];
        BestBin = Bin;
      }
    }

    if((BestBin < 0) ||
       (BestCount < RC3_LEARN_MIN_PEAK_COUNT))
    {
      return 0;
    }

    Peak[P] = BestBin;
  }

  /* Sort physical levels from low to high. */
  for(uint8_t A = 0; A < 2; A++)
  {
    for(uint8_t B = A + 1; B < 3; B++)
    {
      if(Peak[B] < Peak[A])
      {
        const int16_t Tmp = Peak[A];
        Peak[A] = Peak[B];
        Peak[B] = Tmp;
      }
    }
  }

  const uint16_t L0 =
      rc3LearnWeightedCenter((uint8_t)Peak[0]);

  const uint16_t L1 =
      rc3LearnWeightedCenter((uint8_t)Peak[1]);

  const uint16_t L2 =
      rc3LearnWeightedCenter((uint8_t)Peak[2]);

  if(!rc3LevelsAreValid(L0, L1, L2))
  {
    return 0;
  }

  /*
    IMPORTANT:
    only now, after full validation, overwrite the stored calibration.
  */
  rc3SaveLevelsToEeprom(L0, L1, L2);

  Rc3LevelUs[0] = L0;
  Rc3LevelUs[1] = L1;
  Rc3LevelUs[2] = L2;

  return 1;
}

/* ------------------------------------------------------------------ */
/* Legacy RcSerial reconstruction                                     */
/* ------------------------------------------------------------------ */

static uint8_t legacyPayloadNibble(const uint8_t *Packed,
                                   uint8_t NibbleIdx)
{
  const uint8_t B = Packed[NibbleIdx >> 1];

  if((NibbleIdx & 1U) == 0U)
  {
    return (uint8_t)((B >> 4) & 0x0F);
  }

  return (uint8_t)(B & 0x0F);
}

static uint16_t legacySymbolWidthUs(uint8_t Symbol)
{
  if(Symbol > LEGACY_NIBBLE_I)
  {
    Symbol = LEGACY_NIBBLE_I;
  }

  return LEGACY_PULSE_US(Symbol);
}

/*
  Build exactly the logical nibble stream generated by the old
  RcTxSerial::sendNibbleMsg():

      useful nibbles
      + optional checksum MSN
      + optional checksum LSN

  Checksum = XOR(payload bytes) ^ 0x55.
  For an odd useful nibble count the unused low nibble of the last payload
  byte is forced to zero before the XOR, exactly as in RcTxSerial.
*/
static uint8_t buildLegacyMessage(const uint8_t *Packed,
                                  uint8_t NibbleNb,
                                  uint8_t AddChecksum)
{
  if((Packed == NULL) ||
     (NibbleNb < 1) ||
     (NibbleNb > LEGACY_MAX_USEFUL_NIBBLES))
  {
    return 0;
  }

  uint8_t Out = 0;

  for(uint8_t N = 0; N < NibbleNb; N++)
  {
    LegacyNibbles[Out++] =
        legacyPayloadNibble(Packed, N);
  }

  if(AddChecksum)
  {
    const uint8_t ByteNb =
        (uint8_t)((NibbleNb + 1U) / 2U);

    uint8_t Checksum = 0;

    for(uint8_t B = 0; B < ByteNb; B++)
    {
      uint8_t V = Packed[B];

      if(((NibbleNb & 1U) != 0U) &&
         (B == (uint8_t)(ByteNb - 1U)))
      {
        V &= 0xF0U;
      }

      Checksum ^= V;
    }

    Checksum ^= 0x55U;

    LegacyNibbles[Out++] =
        (uint8_t)((Checksum >> 4) & 0x0F);

    LegacyNibbles[Out++] =
        (uint8_t)(Checksum & 0x0F);
  }

  LegacyNibbleCount = Out;
  LegacyNibblePos = 0;
  LegacyRepeatLeft = 0;
  LegacyPrevPhysicalSymbol = LEGACY_NIBBLE_I;
  LegacyMessageActive = 1;
  LegacyMessagesStarted++;

  return 1;
}

/*
  Load one new historical physical symbol.

  This reproduces the old RcTxSerial repeat-symbol rule:
      if the next nibble index equals the previous transmitted physical
      symbol index, transmit R instead.

  Because previous is updated with the PHYSICAL index, AAA becomes:
      A, R, A
  exactly like the historical implementation.
*/
static void loadNextLegacyPhysicalSymbol(void)
{
  if(LegacyMessageActive &&
     (LegacyNibblePos < LegacyNibbleCount))
  {
    uint8_t Symbol =
        LegacyNibbles[LegacyNibblePos++];

    if(Symbol == LegacyPrevPhysicalSymbol)
    {
      Symbol = LEGACY_NIBBLE_R;
    }

    LegacyPhysicalSymbol = Symbol;
    LegacyPrevPhysicalSymbol = Symbol;

    LegacyRepeatLeft =
        (uint8_t)(LEGACY_REPEAT_NB + 1U);

    return;
  }

  /*
    End-of-message / no message:
    emit the historical Idle level continuously.
    The first Idle also resets the repeat-symbol history so that the first
    nibble of the next message can never be mistaken for a continuation.
  */
  LegacyMessageActive = 0;
  LegacyPhysicalSymbol = LEGACY_NIBBLE_I;
  LegacyPrevPhysicalSymbol = LEGACY_NIBBLE_I;
  LegacyRepeatLeft = 1;
}

/*
  Called ONCE for every fresh receiver PWM pulse.

  The output pulse is forced immediately after the input pulse has been
  captured.  This keeps the blocking SoftRcPulseOut pulse in the quiet part
  of the ~20 ms RC frame instead of overlapping the next RC3 input edge.
*/
static void emitOneLegacyFrame(void)
{
  if(LegacyRepeatLeft == 0)
  {
    loadNextLegacyPhysicalSymbol();
  }

  RcSerialOut.write_us(
      legacySymbolWidthUs(
          LegacyPhysicalSymbol));

  SoftRcPulseOut::refresh(1);

  if(LegacyRepeatLeft)
  {
    LegacyRepeatLeft--;
  }
}

/*
  Accept one RC3 compatibility payload:
    [0] metadata
    [1..] packed legacy payload
*/
static void processCompatMessage(const uint8_t *Msg,
                                 uint8_t Len)
{
  if((Msg == NULL) ||
     (Len < 2))
  {
    Rc3CompatIgnored++;
    return;
  }

  const uint8_t Meta = Msg[0];

  if((Meta & RC3_COMPAT_MAGIC_MASK) !=
     RC3_COMPAT_MAGIC)
  {
    /* Native RC3 message: not a legacy bridge message. */
    Rc3CompatIgnored++;
    return;
  }

  const uint8_t NibbleNb =
      (uint8_t)(Meta & RC3_COMPAT_NIBBLE_MASK);

  const uint8_t AddChecksum =
      (uint8_t)!!(Meta & RC3_COMPAT_CHECKSUM_BIT);

  if((NibbleNb < 1) ||
     (NibbleNb > LEGACY_MAX_USEFUL_NIBBLES))
  {
    Rc3CompatIgnored++;
    return;
  }

  const uint8_t ByteNb =
      (uint8_t)((NibbleNb + 1U) / 2U);

  if(Len != (uint8_t)(ByteNb + 1U))
  {
    Rc3CompatIgnored++;
    return;
  }

  if(buildLegacyMessage(
        &Msg[1],
        NibbleNb,
        AddChecksum))
  {
    Rc3CompatGood++;
  }
  else
  {
    Rc3CompatIgnored++;
  }
}

/* ------------------------------------------------------------------ */
/* Arduino                                                             */
/* ------------------------------------------------------------------ */

void setup()
{
  /*
    PB5 / RESET is deliberately untouched.
    PB3/PB4 are occupied by the external 16 MHz crystal.
  */

  /*
    PB1 is sampled ONLY at startup.
    Open          -> HIGH through internal pull-up -> normal mode
    Jumper to GND -> LOW                      -> Learn mode
  */
  pinMode(LEARN_INPUT_PIN, INPUT_PULLUP);
  delay(20);
  Rc3LearnRequested =
      (digitalRead(LEARN_INPUT_PIN) == LOW);

  /*
    Wide input limits are essential: do not reject a legitimate learned
    low/high level before Rc3RxSerial has a chance to classify it.
  */
  Rc3PulseIn.attach(
      RC3_INPUT_PIN,
      RC3_CAPTURE_MIN_US,
      RC3_CAPTURE_MAX_US);

  RcSerialOut.attach(
      RCSERIAL_OUTPUT_PIN);

  RcSerialOut.write_us(
      legacySymbolWidthUs(
          LEGACY_NIBBLE_I));

  /*
    Normal source of calibration:
      1) valid EEPROM values if present;
      2) otherwise safe 1000/1500/2000 defaults.
  */
  if(!rc3LoadLevelsFromEeprom())
  {
    rc3UseDefaultLevels();
  }

  rc3ApplyPulseWindows();

  /*
    RepeatNb is entirely managed inside Rc3RxSerial V1.1.
    The sketch simply feeds pulse measurements to the library.
  */

  /*
    Learn is intentionally explicit.
    EEPROM is overwritten ONLY if the complete 3-level Learn succeeds.
  */
  if(Rc3LearnRequested)
  {
    Rc3LearnSucceeded = rc3RunLearn();

    if(Rc3LearnSucceeded)
    {
      rc3ApplyPulseWindows();
    }
    else
    {
      /*
        Learn failed: keep the calibration that was already loaded before
        Learn (EEPROM or defaults). Nothing has been written to EEPROM.
      */
      rc3ApplyPulseWindows();
    }
  }
}

void loop()
{
  uint8_t Rc3Msg[RC3_RX_SERIAL_MAX_PAYLOAD];

  /*
    msgAvailable() also polls SoftRcPulseIn through the Rcul interface.
    A valid RC3 message is published only after CRC4 validation.
  */
  const uint8_t Len =
      Rc3Rx.msgAvailable(
          (char *)Rc3Msg,
          sizeof(Rc3Msg));

  if(Len)
  {
    processCompatMessage(
        Rc3Msg,
        Len);
  }

  /*
    tritAvailable() tells us that Rc3RxSerial consumed one fresh physical
    receiver pulse.  lastWidth_us() clears that one-shot indication.
  */
  if(Rc3Rx.tritAvailable())
  {
    (void)Rc3Rx.lastWidth_us();
    emitOneLegacyFrame();
  }
}
