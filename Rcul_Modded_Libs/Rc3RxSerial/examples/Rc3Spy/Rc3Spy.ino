/*
  Rc3Spy Uno/Nano PWM
  ================

  Adaptation of Rc3Spy for:
    - Arduino Uno/Nano / ATmega328P only
    - PWM receiver output only
    - Rc3RxSerial V1.1
    - AUTO Repeat
    - RC3 compatibility messages produced by Rc3TxSerial::sendNibbleMsg()

  Wiring:
    Receiver PWM output -> Uno/Nano D2
    Receiver GND        -> Uno/Nano GND

  Serial monitor:
    115200 bauds
    Press Enter to toggle:
      - Message mode: decoded X-Any payload
      - Debug mode  : every physical RC3 PWM sample

  IMPORTANT:
    Rc3RxSerial validates the RC3 CRC4 before publishing a message.
    The RC3 compatibility payload is:
      byte 0 : metadata
      byte 1..3 : packed historical X-Any useful nibbles

    For display compatibility with the old XanySpy, this sketch reconstructs
    the historical 2 checksum nibbles locally when AddChecksum is set.
*/

#include <Arduino.h>
#include <Rcul.h>
#include <TinyPinChange.h>
#include <SoftRcPulseIn.h>
#include <Rc3RxSerial.h>

/* ------------------------------------------------------------------ */
/* User configuration                                                  */
/* ------------------------------------------------------------------ */

#define RC3_INPUT_PIN        2

/*
  Expected X-Any payload, exactly as configured in the encoder.
*/
#define ANGLE                1   /* 0: absent, 1: present */
#define PROP                 1   /* 0: absent, 1: present */
#define SW_NB                0   /* 0, 4, 8 or 16 */

/*
  Set to 1 only if you intentionally want to accept any useful-nibble count.
*/
#define IGNORE_NIBBLE_LEN    0

#define INACTIVITY_MS        5000UL
#define QUALITY_COUNT        20U

/*
  Wide physical capture range for the receiver PWM.
*/
#define RC3_CAPTURE_MIN_US   700U
#define RC3_CAPTURE_MAX_US   2300U

/*
  Rc3RxSerial trit classification windows.
  These also cover the measured PTR-6A receiver levels around:
      1116 / 1532 / 1908 us
*/
#define RC3_TRIT0_MIN_US     800U
#define RC3_TRIT0_MAX_US     1200U
#define RC3_TRIT1_MIN_US     1300U
#define RC3_TRIT1_MAX_US     1700U
#define RC3_TRIT2_MIN_US     1800U
#define RC3_TRIT2_MAX_US     2200U

/* ------------------------------------------------------------------ */
/* RC3 compatibility envelope                                          */
/* ------------------------------------------------------------------ */

#define RC3_COMPAT_MAGIC_MASK          0xE0U
#define RC3_COMPAT_MAGIC               0xA0U
#define RC3_COMPAT_CHECKSUM_BIT        0x10U
#define RC3_COMPAT_NIBBLE_MASK         0x0FU

#define EXPECTED_NIBBLE_LEN \
  ((ANGLE * 3) + (PROP * 2) + (SW_NB / 4))

#define RC3_MSG_LEN_MAX      RC3_RX_SERIAL_MAX_PAYLOAD
#define XANY_NIBBLE_MAX      8U

/* ------------------------------------------------------------------ */
/* Objects                                                             */
/* ------------------------------------------------------------------ */

static SoftRcPulseIn PwmRcSignal;

/*
  Rc3RxSerial V1.1:
  AUTO Repeat is the constructor default.
*/
static Rc3RxSerial Rc3Rx(&PwmRcSignal);

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

typedef struct
{
  uint8_t RxFrameCnt;
  uint8_t ErrorCnt;
} XanyStatSt_t;

static XanyStatSt_t XanyStat = {0, 0};

static uint8_t DebugMode = 0;

static uint32_t LastActivityStartMs = 0;
static uint32_t LastMsgStartMs = 0;
static uint32_t LastPhysicalStartMs = 0;

static uint8_t LastReportedRepeat = 0xFFU;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static uint8_t packedNibble(const uint8_t *Packed, uint8_t NibbleIdx)
{
  const uint8_t B = Packed[NibbleIdx >> 1];

  if((NibbleIdx & 1U) == 0U)
    return (uint8_t)((B >> 4) & 0x0FU);

  return (uint8_t)(B & 0x0FU);
}

static void printNibbleBin(uint8_t Nibble)
{
  for(int8_t Bit = 3; Bit >= 0; --Bit)
    Serial.print((Nibble >> Bit) & 0x01U);
}

static void printNibbleStream(const uint8_t *Nibbles, uint8_t NibbleNb)
{
  for(uint8_t Idx = 0; Idx < NibbleNb; ++Idx)
  {
    if(Idx) Serial.print(F("."));
    printNibbleBin(Nibbles[Idx]);
  }
}

/*
  Reconstruct the exact historical X-Any nibble stream for display:
      useful nibbles
      + optional historical checksum MSN
      + optional historical checksum LSN

  Old RcTxSerial checksum:
      XOR(payload bytes) ^ 0x55
  with the unused low nibble forced to zero for an odd useful nibble count.
*/
static uint8_t buildDisplayNibbles(const uint8_t *Packed,
                                   uint8_t NibbleNb,
                                   uint8_t AddChecksum,
                                   uint8_t *Out,
                                   uint8_t OutMax)
{
  if(!Packed || !Out || !NibbleNb)
    return 0;

  const uint8_t Needed =
      (uint8_t)(NibbleNb + (AddChecksum ? 2U : 0U));

  if(Needed > OutMax)
    return 0;

  for(uint8_t N = 0; N < NibbleNb; ++N)
    Out[N] = packedNibble(Packed, N);

  if(AddChecksum)
  {
    const uint8_t ByteNb =
        (uint8_t)((NibbleNb + 1U) / 2U);

    uint8_t Checksum = 0;

    for(uint8_t B = 0; B < ByteNb; ++B)
    {
      uint8_t V = Packed[B];

      if((NibbleNb & 1U) &&
         (B == (uint8_t)(ByteNb - 1U)))
      {
        V &= 0xF0U;
      }

      Checksum ^= V;
    }

    Checksum ^= 0x55U;

    Out[NibbleNb] =
        (uint8_t)((Checksum >> 4) & 0x0FU);

    Out[NibbleNb + 1U] =
        (uint8_t)(Checksum & 0x0FU);
  }

  return Needed;
}

static int8_t debugWidthToTrit(uint16_t WidthUs)
{
  if((WidthUs >= RC3_TRIT0_MIN_US) &&
     (WidthUs <= RC3_TRIT0_MAX_US))
    return 0;

  if((WidthUs >= RC3_TRIT1_MIN_US) &&
     (WidthUs <= RC3_TRIT1_MAX_US))
    return 1;

  if((WidthUs >= RC3_TRIT2_MIN_US) &&
     (WidthUs <= RC3_TRIT2_MAX_US))
    return 2;

  return -1;
}

static void reportAutoRepeatIfNeeded(void)
{
  if(!Rc3Rx.repeatDetected())
    return;

  const uint8_t RepeatNb = Rc3Rx.getRepeatNb();

  if(RepeatNb != LastReportedRepeat)
  {
    LastReportedRepeat = RepeatNb;

    Serial.println();
    Serial.print(F("RC3 AUTO Repeat detected: RepeatNb="));
    Serial.print(RepeatNb);
    Serial.print(F(" ("));
    Serial.print(Rc3Rx.getSamplesPerTrit());
    Serial.println(F(" physical samples/trit)"));
  }
}

/* ------------------------------------------------------------------ */
/* X-Any payload decoding                                              */
/* ------------------------------------------------------------------ */

static uint16_t GetAngle(const uint8_t *RxMsg)
{
  return (uint16_t)(((uint16_t)RxMsg[0] << 4) |
                    ((uint16_t)RxMsg[1] >> 4));
}

static uint16_t GetProp(const uint8_t *RxMsg)
{
#if (ANGLE == 1)
  uint8_t Prop;
  Prop  = (uint8_t)((RxMsg[1] & 0x0FU) << 4);
  Prop |= (uint8_t)((RxMsg[2] & 0xF0U) >> 4);
  return Prop;
#else
  return RxMsg[0];
#endif
}

static uint16_t GetSwitch(const uint8_t *RxMsg)
{
  uint16_t Sw = 0;

#if (SW_NB < 16)

  #if (ANGLE == 1)

    #if (SW_NB == 4)
      Sw |= RxMsg[1 + !!PROP] & 0x0FU;
    #elif (SW_NB == 8)
      Sw |= (uint16_t)(RxMsg[1 + !!PROP] & 0x0FU) << 4;
      Sw |= (uint16_t)(RxMsg[2 + !!PROP] & 0xF0U) >> 4;
    #endif

  #else

    #if (SW_NB == 4)
      Sw |= (uint16_t)(RxMsg[0 + !!PROP] & 0xF0U) >> 4;
    #elif (SW_NB == 8)
      Sw |= RxMsg[0 + !!PROP];
    #endif

  #endif

#else

  #if (ANGLE == 1)
    Sw |= (uint16_t)(RxMsg[1 + !!PROP] & 0x0FU) << 12;
    Sw |= (uint16_t) RxMsg[2 + !!PROP] << 4;
    Sw |= (uint16_t)(RxMsg[3 + !!PROP] & 0xF0U) >> 4;
  #else
    Sw |= (uint16_t)RxMsg[0 + !!PROP] << 8;
    Sw |= RxMsg[1 + !!PROP];
  #endif

#endif

  return Sw;
}

static void printSwitchBits(uint16_t Word)
{
#if (SW_NB > 0)
  Serial.print(F("Switch["));
  Serial.print(SW_NB);
  Serial.print(F("..1]="));

  for(int8_t Bit = SW_NB - 1; Bit >= 0; --Bit)
  {
    Serial.print((Word >> Bit) & 0x01U);

    if((Bit != 0) && ((Bit & 3) == 0))
      Serial.print(F("."));
  }
#else
  (void)Word;
#endif
}

/* ------------------------------------------------------------------ */
/* RC3 compatibility message processing                               */
/* ------------------------------------------------------------------ */

static void processRc3Message(const uint8_t *Rc3Msg, uint8_t Len)
{
  if(!Rc3Msg || (Len < 2U))
  {
    XanyStat.ErrorCnt++;
    Serial.println(F("RC3/X-Any message too short"));
    return;
  }

  const uint8_t Meta = Rc3Msg[0];

  if((Meta & RC3_COMPAT_MAGIC_MASK) != RC3_COMPAT_MAGIC)
  {
    XanyStat.ErrorCnt++;

    Serial.print(F("RC3 native/non-X-Any payload ignored: Len="));
    Serial.println(Len);
    return;
  }

  const uint8_t NibbleNb =
      (uint8_t)(Meta & RC3_COMPAT_NIBBLE_MASK);

  const uint8_t AddChecksum =
      (uint8_t)!!(Meta & RC3_COMPAT_CHECKSUM_BIT);

  const uint8_t ByteNb =
      (uint8_t)((NibbleNb + 1U) / 2U);

  if((NibbleNb < 1U) ||
     (NibbleNb > 6U) ||
     (Len != (uint8_t)(ByteNb + 1U)))
  {
    XanyStat.ErrorCnt++;

    Serial.print(F("RC3 compatibility length error: Len="));
    Serial.print(Len);
    Serial.print(F(" NibbleNb="));
    Serial.println(NibbleNb);
    return;
  }

  if(!IGNORE_NIBBLE_LEN &&
     (NibbleNb != EXPECTED_NIBBLE_LEN))
  {
    XanyStat.ErrorCnt++;

    Serial.print(F("X-Any Length error! ExpUsefulNibbleLen="));
    Serial.print(EXPECTED_NIBBLE_LEN);
    Serial.print(F(" RxUsefulNibbleLen="));
    Serial.println(NibbleNb);
    return;
  }

  const uint8_t *Packed = &Rc3Msg[1];

  uint8_t DisplayNibbles[XANY_NIBBLE_MAX];

  const uint8_t DisplayNibbleNb =
      buildDisplayNibbles(
          Packed,
          NibbleNb,
          AddChecksum,
          DisplayNibbles,
          sizeof(DisplayNibbles));

  if(!DisplayNibbleNb)
  {
    XanyStat.ErrorCnt++;
    Serial.println(F("Display reconstruction error"));
    return;
  }

  LastActivityStartMs = millis();

  Serial.println();

  Serial.print(F("X-Any Msg[TotNbl="));
  Serial.print(DisplayNibbleNb);
  Serial.print(F("]="));

  printNibbleStream(
      DisplayNibbles,
      DisplayNibbleNb);

  Serial.print(F(" T="));
  Serial.println(millis() - LastMsgStartMs);

#if (ANGLE == 1)
  const uint16_t Angle = GetAngle(Packed);
  const float AngleDeg = (360.0F * Angle) / 4095.0F;

  Serial.print(F("Angle="));
  Serial.print(Angle);
  Serial.print(F(" ("));
  Serial.print(AngleDeg, 1);
  Serial.print(F(" deg) "));
#endif

#if (PROP == 1)
  Serial.print(F("Prop="));
  Serial.print(GetProp(Packed));
  Serial.print(F(" "));
#endif

#if (SW_NB > 0)
  printSwitchBits(GetSwitch(Packed));
#endif

  Serial.println();

  LastMsgStartMs = millis();
}

/* ------------------------------------------------------------------ */
/* Arduino                                                             */
/* ------------------------------------------------------------------ */

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println(F("Rc3Spy Uno/Nano PWM"));
  Serial.print(F("Rc3RxSerial V"));
  Serial.println(Rc3RxSerial::version());

  Serial.print(F("PWM input       : D"));
  Serial.println(RC3_INPUT_PIN);

  Serial.println(F("Repeat          : AUTO"));

  Serial.print(F("Expected X-Any  : ANGLE="));
  Serial.print(ANGLE);
  Serial.print(F(" PROP="));
  Serial.print(PROP);
  Serial.print(F(" SW_NB="));
  Serial.println(SW_NB);

  /*
    SoftRcPulseIn only captures the physical PWM.
    Rc3RxSerial performs trit classification, AUTO Repeat, majority vote,
    frame reconstruction and CRC4 validation.
  */
  PwmRcSignal.attach(
      RC3_INPUT_PIN,
      RC3_CAPTURE_MIN_US,
      RC3_CAPTURE_MAX_US);

  Rc3Rx.setPulseWindows(
      RC3_TRIT0_MIN_US, RC3_TRIT0_MAX_US,
      RC3_TRIT1_MIN_US, RC3_TRIT1_MAX_US,
      RC3_TRIT2_MIN_US, RC3_TRIT2_MAX_US);

  Rc3Rx.setAutoRepeat(1);

  LastActivityStartMs = millis();
  LastMsgStartMs = millis();
  LastPhysicalStartMs = millis();

  Serial.println();
  Serial.println(F("Press Enter to toggle Debug / Message mode."));
  Serial.println(F("Start spying..."));
  Serial.println();
}

void loop()
{
  uint8_t Rc3Msg[RC3_MSG_LEN_MAX];

  /*
    Press Enter to toggle display mode.
  */
  if(Serial.available())
  {
    while(Serial.available())
      Serial.read();

    DebugMode = !DebugMode;

    Serial.println();
    Serial.print(F("Mode: "));
    Serial.println(DebugMode ? F("DEBUG PWM") : F("X-ANY MESSAGE"));
    Serial.println();
  }

  /*
    One call polls one fresh SoftRcPulseIn sample when available.
    In Debug mode valid RC3 messages are intentionally consumed but not decoded.
  */
  const uint8_t Len =
      Rc3Rx.msgAvailable(
          (char *)Rc3Msg,
          sizeof(Rc3Msg));

  reportAutoRepeatIfNeeded();

  /*
    Raw physical PWM debug.
  */
  if(Rc3Rx.tritAvailable())
  {
    const uint16_t WidthUs =
        Rc3Rx.lastWidth_us();

    if(DebugMode)
    {
      const int8_t Trit =
          debugWidthToTrit(WidthUs);

      Serial.print(WidthUs);
      Serial.print(F(" -> "));

      if(Trit >= 0)
        Serial.print(Trit);
      else
        Serial.print(F("?"));

      Serial.print(F(" T="));
      Serial.print(millis() - LastPhysicalStartMs);

      if(Rc3Rx.repeatDetected())
      {
        Serial.print(F(" R="));
        Serial.print(Rc3Rx.getRepeatNb());
      }

      Serial.println();

      LastPhysicalStartMs = millis();
    }
  }

  if(DebugMode)
    return;

  if(Len)
  {
    XanyStat.RxFrameCnt++;

    processRc3Message(
        Rc3Msg,
        Len);

    if(XanyStat.RxFrameCnt >= QUALITY_COUNT)
    {
      const uint8_t Good =
          (uint8_t)(XanyStat.RxFrameCnt - XanyStat.ErrorCnt);

      Serial.println();
      Serial.println(F("**********************"));
      Serial.print(F(" Xany quality = "));
      Serial.print((uint16_t)Good * 100U / XanyStat.RxFrameCnt);
      Serial.println(F(" %"));

      Serial.print(F(" RC3 Repeat    = "));
      if(Rc3Rx.repeatDetected())
        Serial.println(Rc3Rx.getRepeatNb());
      else
        Serial.println(F("AUTO searching"));

      Serial.print(F(" RC3 CRC err   = "));
      Serial.println(Rc3Rx.crcErrors());

      Serial.print(F(" RC3 Sym err   = "));
      Serial.println(Rc3Rx.symbolErrors());

      Serial.print(F(" RC3 Maj fix   = "));
      Serial.println(Rc3Rx.majorityCorrected());

      Serial.println(F("**********************"));

      XanyStat.RxFrameCnt = 0;
      XanyStat.ErrorCnt = 0;
    }
  }

  /*
    Failsafe indication: no valid matching X-Any compatibility message.
  */
  if((millis() - LastActivityStartMs) > INACTIVITY_MS)
  {
    LastActivityStartMs = millis();
    Serial.println(F("X-Any Failsafe! -> Outputs = 0"));
  }
}
