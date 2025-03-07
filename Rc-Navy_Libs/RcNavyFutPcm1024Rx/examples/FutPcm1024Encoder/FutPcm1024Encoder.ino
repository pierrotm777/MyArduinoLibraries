#include <Misclib.h>
#include <FutPcm1024Rx.h>

#define PCM_OC1A_PIN  9 // PCM signal output on pin 9

/*
  920 us -> 0
 2120 us -> 1023

 2120 - 920 = 1200
    0 -> 0
 1200 -> 1023
 Coef = 1200 / 1023 = 1.17307
 61/52 = 1.17307 -> Quasi error free, but computing needs to be done with long
 or:
 20/17 = 1.17647 -> 0.3% of error -> Acceptable, and computing can be done with int16_t -> Better
 DeltaBin = DeltaUs x 17 / 20
*/
#define PWM_US_MIN       920
#define PWM_US_MAX      2120
#define PWM_US_CENTER   ((PWM_US_MIN + PWM_US_MAX) / 2) // 1520 us

/*
 PcmPacketIdx=0                      PcmPacketIdx=4                      PcmPacketIdx=0
   .---^---.                           .---^---.                           .---^---.
   .-------.-------.-------.-------.   .-------.-------.-------.-------.   .-------.-------.-------.-------.
   | Pos=1 | Pos=3 | Pos=5 | Pos=7 |   | Pos=2 | Pos=4 | Pos=6 | Pos=8 |   | Pos=1 | Pos=3 | Pos=5 | Pos=7 |
   |Delta=2|Delta=4|Delta=6|Delta=8|   |Delta=1|Delta=3|Delta=5|Delta=7|   |Delta=2|Delta=4|Delta=6|Delta=8|
   '-------'-------'-------'-------'   '-------'-------'-------'-------'   '-------'-------'-------'-------'
            Frame1=Odd                          Frame2=Even                         Frame1=Odd
   |---------------------------------|-----------------------------------|-----------------------------------> Time
                                 30ms                                60ms
   PosChId   = [2 x (PcmPacketIdx + 1)] / 2
   DeltaChId = PosChId + 1
                                       DeltaChId = [2 x (PcmPacketIdx - 3)] / 2
                                       PosChId   = DeltaChId + 1
*/

typedef struct{
  int16_t Min;
  int16_t Max;
}MinMaxSt_t;

const MinMaxSt_t DeltaPcmTbl[] PROGMEM = {
                                  /*  0 */{-1023, -116  },
                                  /*  1 */{ -115, -88   },
                                  /*  2 */{  -87, -64   },
                                  /*  3 */{  -63, -44   },
                                  /*  4 */{  -43, -28   },
                                  /*  5 */{  -27, -16   },
                                  /*  6 */{  -15, -8    },
                                  /*  7 */{   -7, -4    },
                                  /*  8 */{   -3, +4    },
                                  /*  9 */{   +5, +8    },
                                  /* 10 */{   +9, +16   },
                                  /* 11 */{  +17, +28   },
                                  /* 12 */{  +29, +44   },
                                  /* 13 */{  +45, +64   },
                                  /* 14 */{  +65, +87   },
                                  /* 15 */{  +88, +1023 },
                                          };

const uint16_t SixToTenTbl[] PROGMEM = {
                              /* 0x00 */ (0b00000011 << 8) | 0b11111000,
                              /* 0x01 */ (0b00000011 << 8) | 0b11110011,
                              /* 0x02 */ (0b00000011 << 8) | 0b11100011,
                              /* 0x03 */ (0b00000011 << 8) | 0b11100111,
                              /* 0x04 */ (0b00000011 << 8) | 0b11000111,
                              /* 0x05 */ (0b00000011 << 8) | 0b11001111,
                              /* 0x06 */ (0b00000011 << 8) | 0b10001111,
                              /* 0x07 */ (0b00000011 << 8) | 0b10011111,
                              /* 0x08 */ (0b00000000 << 8) | 0b11111111,
                              /* 0x09 */ (0b00000000 << 8) | 0b01111111,
                              /* 0x0A */ (0b00000000 << 8) | 0b00111111,
                              /* 0x0B */ (0b00000011 << 8) | 0b00111111,
                              /* 0x0C */ (0b00000011 << 8) | 0b00011111,
                              /* 0x0D */ (0b00000011 << 8) | 0b00001111,
                              /* 0x0E */ (0b00000011 << 8) | 0b10000111,
                              /* 0x0F */ (0b00000011 << 8) | 0b11000011,
                              /* 0x10 */ (0b00000000 << 8) | 0b11111100,
                              /* 0x11 */ (0b00000000 << 8) | 0b11110011,
                              /* 0x12 */ (0b00000000 << 8) | 0b11100111,
                              /* 0x13 */ (0b00000000 << 8) | 0b11001111,
                              /* 0x14 */ (0b00000011 << 8) | 0b11001100,
                              /* 0x15 */ (0b00000011 << 8) | 0b10011100,
                              /* 0x16 */ (0b00000011 << 8) | 0b00111100,
                              /* 0x17 */ (0b00000011 << 8) | 0b00110011,
                              /* 0x18 */ (0b00000011 << 8) | 0b11110000,
                              /* 0x19 */ (0b00000011 << 8) | 0b11100000,
                              /* 0x1A */ (0b00000011 << 8) | 0b10000011,
                              /* 0x1B */ (0b00000011 << 8) | 0b00000111,
                              /* 0x1C */ (0b00000011 << 8) | 0b00011100,
                              /* 0x1D */ (0b00000011 << 8) | 0b10011000,
                              /* 0x1E */ (0b00000011 << 8) | 0b10001100,
                              /* 0x1F */ (0b00000011 << 8) | 0b00111000,
                              /* 0x20 */ (0b00000000 << 8) | 0b11000111,
                              /* 0x21 */ (0b00000000 << 8) | 0b01110011,
                              /* 0x22 */ (0b00000000 << 8) | 0b01100111,
                              /* 0x23 */ (0b00000000 << 8) | 0b11100011,
                              /* 0x24 */ (0b00000000 << 8) | 0b11111000,
                              /* 0x25 */ (0b00000000 << 8) | 0b01111100,
                              /* 0x26 */ (0b00000000 << 8) | 0b00011111,
                              /* 0x27 */ (0b00000000 << 8) | 0b00001111,
                              /* 0x28 */ (0b00000000 << 8) | 0b11001100,
                              /* 0x29 */ (0b00000000 << 8) | 0b11000011,
                              /* 0x2A */ (0b00000000 << 8) | 0b01100011,
                              /* 0x2B */ (0b00000000 << 8) | 0b00110011,
                              /* 0x2C */ (0b00000011 << 8) | 0b00110000,
                              /* 0x2D */ (0b00000011 << 8) | 0b00011000,
                              /* 0x2E */ (0b00000011 << 8) | 0b00001100,
                              /* 0x2F */ (0b00000011 << 8) | 0b00000011,
                              /* 0x30 */ (0b00000000 << 8) | 0b00111100,
                              /* 0x31 */ (0b00000000 << 8) | 0b01111000,
                              /* 0x32 */ (0b00000000 << 8) | 0b11110000,
                              /* 0x33 */ (0b00000000 << 8) | 0b11100000,
                              /* 0x34 */ (0b00000000 << 8) | 0b11000000,
                              /* 0x35 */ (0b00000011 << 8) | 0b11000000,
                              /* 0x36 */ (0b00000011 << 8) | 0b10000000,
                              /* 0x37 */ (0b00000011 << 8) | 0b00000000,
                              /* 0x38 */ (0b00000000 << 8) | 0b01100000,
                              /* 0x39 */ (0b00000000 << 8) | 0b01110000,
                              /* 0x3A */ (0b00000000 << 8) | 0b00110000,
                              /* 0x3B */ (0b00000000 << 8) | 0b00111000,
                              /* 0x3C */ (0b00000000 << 8) | 0b00011000,
                              /* 0x3D */ (0b00000000 << 8) | 0b00011100,
                              /* 0x3E */ (0b00000000 << 8) | 0b00001100,
                              /* 0x3F */ (0b00000000 << 8) | 0b00000111,
                                      };

#define six2ten(SixIdx)    (uint16_t)pgm_read_word(&SixToTenTbl[(SixIdx)])  // Convert six bits to ten bits

#define FUT_24_BIT_PCM_PACKET_BIT_NB          24

union Fut24BitPcmPacket_Union
{
  uint32_t  Value;
  struct {
  uint32_t
            Ecc:          8,  // ^
            Position:     10, // |
            Delta:        4,  // | bits
            AuxBit0:      1,  // |  24
            AuxBit1:      1,  // v
            Reserved:     8;
  };
  struct {
  uint32_t
            SixBitBlock3: 6,  // ^
            SixBitBlock2: 6,  // |  24
            SixBitBlock1: 6,  // | bits
            SixBitBlock0: 6,  // v
            Reserved2:    8;
  };
}__attribute__((__packed__));

#define FUT_40_BIT_RADIO_PCM_PACKET_BIT_NB    40

union Fut40BitRadioPcmPacket_Union
{
  uint64_t Value;
  struct{
  uint64_t
           TenBitBlock3: 10,
           TenBitBlock2: 10,
           TenBitBlock1: 10,
           TenBitBlock0: 10,
           Reserved:     24;
  };
}__attribute__((__packed__));

union PcmStreamBitNbCouple_Union
{
  uint8_t Value;
  struct{
    uint8_t
            LowNibbleBitNb:  4,
            HighNibbleBitNb: 4;
  };
};

#define PCM_STREAM_BYTE_NB   30
#define PCM_NBL_MAX_IDX      ((2 * PCM_STREAM_BYTE_NB) - 1)
static PcmStreamBitNbCouple_Union PcmStreamConsecBitTbl[PCM_STREAM_BYTE_NB]; // Will be volatile (use in Tx Output Compare Interrupt)

inline void PcmStreamSetConsecBitNb(uint8_t NblIdx, uint8_t ConsecBitNb)
{
  if(NblIdx <= PCM_NBL_MAX_IDX)
  {
    if((NblIdx) & 1) PcmStreamConsecBitTbl[(NblIdx)/2].LowNibbleBitNb  = ConsecBitNb;
    else             PcmStreamConsecBitTbl[(NblIdx)/2].HighNibbleBitNb = ConsecBitNb;
  }
}

inline uint8_t PcmStreamGetConsecBitNb(uint8_t NblIdx)
{
  if((NblIdx) & 1) return(PcmStreamConsecBitTbl[(NblIdx)/2].LowNibbleBitNb);
  else             return(PcmStreamConsecBitTbl[(NblIdx)/2].HighNibbleBitNb);
}

enum {BUILD_PCM_DO_NOTHING = 0, BUILD_PCM_2_FIRST, BUILD_PCM_2_LAST};

typedef struct{
  uint8_t 
          BuildState:   4, // 0: Nothing to do, 1: 0 & 1 or 4 & 5 , 2: 2 & 3 or 6 & 7
          PacketIdx:    3,
          BitVal:       1;
  uint8_t BuildNblIdx;
  uint8_t BuildEndNblIdx; // For the ISR to know PCM frame is fully sent
  uint8_t TxNblIdx;
}PcmSt_t;

#define HALF_TX_NBL_IDX 30

static volatile PcmSt_t Pcm;
#if 1
                                 /* CH1  CH2   CH3  CH4   CH5   CH6   CH7    CH8 */
static uint16_t Channels[16] = { /* 0     1     2     3     4     5     6     7 */
                                  1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                                 /* 8     9    10    11    12    13    14    15 */
                                  1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,
                               };
#else
                                 /* CH1  CH2   CH3  CH4   CH5   CH6   CH7    CH8 */
static uint16_t Channels[16] = { /* 0     1     2     3     4     5     6     7 */
                                  1480, 1490, 1500, 1510, 1520, 1530, 1540, 1550,
                                 /* 8     9    10    11    12    13    14    15 */
                                  1440, 1460, 1480, 1500, 1520, 1540, 1560, 1580,
                               };
#endif
void BuildRadioPcmBitStream(void);


void FutPcm1024Tx_begin(void)
{
  Pcm.BuildState     = BUILD_PCM_DO_NOTHING;
  Pcm.PacketIdx      = 0;
  Pcm.BitVal         = 0;
  Pcm.BuildNblIdx    = 0;
  Pcm.BuildEndNblIdx = PCM_NBL_MAX_IDX;
  Pcm.TxNblIdx       = HALF_TX_NBL_IDX;
  /* Timer1 configuration for Tx */
  digitalWrite(PCM_OC1A_PIN, HIGH/*_Modu? LOW: HIGH*/);
  pinMode(PCM_OC1A_PIN, OUTPUT);
  
  // Enable Timer1 output compare interrupt...
  bitSet(TIFR1, OCF1A); // clr pending interrupt
  bitSet(TIMSK1, OCIE1A); // enable interrupt

}

void setup()
{
  Serial.begin(115200);
  Serial.println(F("FUTABA_PCM1024_ENCODER V0.2"));
  FutPcm1024Tx_begin();
  FutPcm1024Rx_begin();
  memset(&PcmStreamConsecBitTbl, 0x22, sizeof(PcmStreamConsecBitTbl));
}

#define UP_DIRECTION      (int16_t)+1
#define DOWN_DIRECTION    (int16_t)-1
#define WIDTH_US_MIN      (int16_t)-500
#define WIDTH_US_MAX      (int16_t)+500
#define STEP_US           (int16_t)10
void loop()
{
  static uint32_t StartUs = micros(), SweepStartMs = millis();
  static int16_t  StepUs = STEP_US;
  static int16_t  WidthUs = WIDTH_US_MIN;
  uint32_t DurUs;
  uint8_t  ChIdx;
  
  /* Tx part */
  if((millis() - SweepStartMs) >= 40L)
  {
    SweepStartMs = millis();
      WidthUs += StepUs;
//if(!ChIdx){Serial.print(F("Exc="));Serial.println(WidthUs);}
      if(WidthUs >= WIDTH_US_MAX) StepUs = STEP_US * DOWN_DIRECTION; //+500 us reached -> Change direction
      if(WidthUs <= WIDTH_US_MIN) StepUs = STEP_US * UP_DIRECTION;   //-500 us reached -> Change direction
    for(ChIdx = 0; ChIdx < 8; ChIdx++)
    {
      Channels[ChIdx] = 1500 + WidthUs;
//if(!ChIdx){Serial.print(F("Channels[1]="));Serial.println(Channels[ChIdx]);};
    }
  }
  if(Pcm.BuildState != BUILD_PCM_DO_NOTHING)
  {
    FutPcm1024Tx_buildRadioPcmBitStream();
  }
  
  /* Rx part */
  if(FutPcm1024Rx_available())
  {
#if 0
    DurUs = micros() - StartUs;
    StartUs = micros();
    Serial.print(F("T(us)="));Serial.println(DurUs);
    for(ChIdx = 0; ChIdx < 8; ChIdx++)
    {
      Serial.print(F("CH"));Serial.print(ChIdx + 1);Serial.print(F("="));Serial.print(FutPcm1024Rx_channelRaw(ChIdx + 1));Serial.print(F(" -> "));Serial.print(FutPcm1024Rx_channelWidthUs(ChIdx + 1));Serial.println(F(" us"));
    }
#else
  Serial.print(FutPcm1024Rx_channelRaw(1));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(2));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(3));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(4));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(5));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(6));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(7));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(8));Serial.print("\t");
  Serial.print(FutPcm1024Rx_channelRaw(9));Serial.print("\t");
//  Serial.print(check_sum);
  Serial.println();
#endif    
  }

#if 0
#if 0
  static uint8_t Done = 0;
  if(!Done)
  {
    Pcm.BuildState = BUILD_PCM_2_FIRST;
    FutPcm1024Tx_buildRadioPcmBitStream();
    Pcm.BuildState = BUILD_PCM_2_LAST;
    FutPcm1024Tx_buildRadioPcmBitStream();
    Pcm.BuildState = BUILD_PCM_2_FIRST;
    FutPcm1024Tx_buildRadioPcmBitStream();
    Pcm.BuildState = BUILD_PCM_2_LAST;
    FutPcm1024Tx_buildRadioPcmBitStream();
    Done = 1;
  }
#else
static uint8_t Cnt = 0, StopIsr = 0;
if(!StopIsr)
{
  PROTO_PCM_1024(); // Simu ISR for test only
  if(Pcm.BuildState != BUILD_PCM_DO_NOTHING)
  {
    FutPcm1024Tx_buildRadioPcmBitStream();
    Cnt++;
    if(Cnt >= 8)
    {
      Serial.println(F("STOP!"));
      StopIsr = 1;
    }
  }
}
#endif
#endif
}

uint16_t UsToPcmValue(int16_t PwmUs, uint8_t Delta)
{
  int16_t PcmBin;
  
  if(!Delta) PwmUs -= PWM_US_MIN;
  PcmBin = ((PwmUs * 17) / 20); // 20/17 = 1.1764 -> Can use int16_t for computation
  return(PcmBin);
}

uint8_t DeltaUsToDeltaCode(int16_t DeltaUs) // DeltaUs = NewWidthUs - PrevWidthUs
{
  int16_t SignedDeltaPcm, Min, Max;
  uint8_t Idx;
  
  SignedDeltaPcm = UsToPcmValue(DeltaUs, 1);
//  Serial.print("SignedDeltaPcm=");Serial.print(SignedDeltaPcm);Serial.print(" ");
  for(Idx = 0; Idx < TBL_ITEM_NB(DeltaPcmTbl); Idx++)
  {
    Min = (int16_t)pgm_read_word(&DeltaPcmTbl[Idx].Min);
    Max = (int16_t)pgm_read_word(&DeltaPcmTbl[Idx].Max);
    if(SignedDeltaPcm >= Min && SignedDeltaPcm <= Max) break;
  }
  if(Idx >= TBL_ITEM_NB(DeltaPcmTbl)) Idx = 8; //Error -> Do NOT move

  return(Idx);
}

const uint8_t PcmCrcTbl[16] PROGMEM = {0x6B, 0xD6, 0xC7, 0xE5, 0xA1, 0x29, 0x52, 0xA4,
                                       0x23, 0x46, 0x8C, 0x73, 0xE6, 0xA7, 0x25, 0x4A};

void FutPcm1024Tx_buildRadioPcmBitStream(void)
{
  uint8_t  Cnt = 0, PosChId, DeltaChId, IsOdd, BitIdx, BitCnt;
  uint64_t BitMask;
  Fut24BitPcmPacket_Union      Fut24BitPcmPacket;
  Fut40BitRadioPcmPacket_Union Fut40BitRadioPcmPacket[2];
  
uint32_t StartUs = micros(), EndUs;
  // Here, we will build 2 PCM Packets at a time to share the Bit Stream buffer with ISR
  if(Pcm.BuildState == BUILD_PCM_2_FIRST)
  {
    IsOdd = (Pcm.PacketIdx < 4); // Pcm.PacketIdx 0 to 3 -> frame is odd
    // OK: the 4 Radio PCM Packets are ready -> Build the Bit stream of the Radio PCM Frame
    Pcm.BuildNblIdx    = 0;
//      Pcm.BuildEndNblIdx = PCM_NBL_MAX_IDX;
    // Preamble -> Odd: 1100, Even: 110000
    PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, 2);PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, IsOdd? 2: 4);
    // Sync Pulse: 18 x '1' of 150 us
    PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, 0); // 0 means 18 as ConsecBitNb coded on 4 bits -> Take this into account in ISR
    // Odd and Even Frame Code -> Odd: 00000011,  Even: 000011
    PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, IsOdd? 6: 4);PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, 2);
    // Continue to populate the Bit stream table with the 4 x Fut40BitRadioPcmPacket
    Pcm.BitVal = 1;
  }
  while (Cnt < 2)
  {
//Serial.print(F("\nPcm.PacketIdx="));Serial.println(Pcm.PacketIdx);
    Fut24BitPcmPacket.AuxBit1 = !(Pcm.PacketIdx & 1);
    Fut24BitPcmPacket.AuxBit0 = 0;
    if(Pcm.PacketIdx < 4)
    {
      PosChId   = 1 + (2 * (Pcm.PacketIdx % 4)); // 1, 3, 5, 7
      DeltaChId = PosChId + 1;                   // 2, 4, 6, 8
    }
    else
    {
      DeltaChId = 1 + (2 * (Pcm.PacketIdx % 4)); // 1, 3, 5, 7
      PosChId   = DeltaChId + 1;                 // 2, 4, 6, 8
    }
//Serial.print(F("PosChId="));Serial.print(PosChId);Serial.print(F(" DeltaChId="));Serial.println(DeltaChId);
    Fut24BitPcmPacket.Position  = UsToPcmValue(Channels[PosChId - 1], 0);
    Fut24BitPcmPacket.Delta     = DeltaUsToDeltaCode(Channels[DeltaChId - 1] - Channels[8 + DeltaChId - 1]);
    Channels[8 + DeltaChId - 1] = Channels[DeltaChId - 1]; // Memo for next Delta (Ch1 to Ch8 are memorized in Ch9 to Ch16)
    //Compute ECC
    Fut24BitPcmPacket.Ecc = 0;
    for(uint8_t BitIdx = 0; BitIdx < 16; BitIdx++)
    {
      if(Fut24BitPcmPacket.Value & (1UL << (8 + BitIdx)))
      {
        Fut24BitPcmPacket.Ecc ^= (uint8_t)pgm_read_byte(&PcmCrcTbl[BitIdx]);
      }
    }
    // OK: PCM Frame is ready -> Build Radio PCM Packets by translating 6 bits to 10 bits
    Fut40BitRadioPcmPacket[Pcm.PacketIdx % 2].TenBitBlock0 = six2ten(Fut24BitPcmPacket.SixBitBlock0);
    Fut40BitRadioPcmPacket[Pcm.PacketIdx % 2].TenBitBlock1 = six2ten(Fut24BitPcmPacket.SixBitBlock1);
    Fut40BitRadioPcmPacket[Pcm.PacketIdx % 2].TenBitBlock2 = six2ten(Fut24BitPcmPacket.SixBitBlock2);
    Fut40BitRadioPcmPacket[Pcm.PacketIdx % 2].TenBitBlock3 = six2ten(Fut24BitPcmPacket.SixBitBlock3);
    Pcm.PacketIdx++; // Pcm.PacketIdx will automatically overlap to 0 as soon it will reach 8 (since it is coded on 3 bits)
    Cnt++;
  }
#if 0
  if((Pcm.PacketIdx == 2) || (Pcm.PacketIdx == 6))
  {
    Serial.println();
    // Preamble -> Odd: 1100, Even: 110000
    Serial.print(F("1100"));if(!IsOdd) Serial.print(F("00"));Serial.println(F(" (Preamble)"));
    Serial.print(F("111111111111111111"));Serial.println(F(" (Sync Pulse)"));
    // Odd and Even Frame Code -> Odd: 00000011,  Even: 000011
    if(IsOdd) Serial.print(F("00"));Serial.print(F("000011"));Serial.println(F(" (Odd/Even Code)"));
  }
  for(Cnt = 0; Cnt < 2; Cnt++)
  {
    for(uint8_t Idx = (FUT_40_BIT_RADIO_PCM_PACKET_BIT_NB - 1); Idx < 255; Idx--)
    {
      Serial.print((uint8_t)bitRead(Fut40BitRadioPcmPacket[Cnt].Value, Idx));
      if(!(Idx % 10)) Serial.print(F(" "));
    }
    Serial.print(F("(40 bit Radio PCM Packet["));Serial.print(Pcm.PacketIdx? Pcm.PacketIdx - 2 + Cnt: 6 + Cnt);Serial.println(F("])"));
  }
#endif
  for(Cnt = 0; Cnt < 2; Cnt++)
  {
    Pcm.BuildNblIdx--;
    BitCnt = PcmStreamGetConsecBitNb(Pcm.BuildNblIdx); // Retrieve last BitCnt in case subsequent bit(s) is(are) identical
    
    for(BitIdx = (FUT_40_BIT_RADIO_PCM_PACKET_BIT_NB - 1); BitIdx < 255; BitIdx--)
    {
      BitMask = (1LL << BitIdx);
    //Serial.print(F("BitVal["));Serial.print(BitIdx);Serial.print(F("]="));Serial.println(!!(Fut40BitRadioPcmPacket[0].Value & BitMask));
      if(Pcm.BitVal == !!(Fut40BitRadioPcmPacket[Cnt].Value & BitMask))
      {
        BitCnt++;
      }
      else
      {
        PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, BitCnt);
        Pcm.BitVal = !Pcm.BitVal;
        BitCnt = 1;
      }
    }
    PcmStreamSetConsecBitNb(Pcm.BuildNblIdx++, BitCnt); // Last bit(s)
  }
EndUs = micros();
Serial.print(F("\nDurationUs="));Serial.println(EndUs - StartUs);
#if 0
  for(uint8_t Idx = 0; Idx < (Pcm.BuildNblIdx + 1) / 2; Idx++)
  {
  //  Serial.print(PcmStreamGetConsecBitNb(Idx));
  Serial.print(F("PcmStreamConsecBitTbl["));Serial.print(Idx);Serial.print(F("].Value=0x"));Serial.println(PcmStreamConsecBitTbl[Idx].Value, HEX);
  Serial.print(F("PcmStreamConsecBitTbl["));Serial.print(Idx);Serial.print(F("].HighNibbleBitNb=0x"));Serial.println(PcmStreamConsecBitTbl[Idx].HighNibbleBitNb);
  Serial.print(F("PcmStreamConsecBitTbl["));Serial.print(Idx);Serial.print(F("].LowNibbleBitNb=0x"));Serial.println(PcmStreamConsecBitTbl[Idx].LowNibbleBitNb);
  }
#endif
  Serial.println();
#if 0
  for(uint8_t Idx = 0; Idx < Pcm.BuildNblIdx; Idx++)
  {
    Serial.print(PcmStreamGetConsecBitNb(Idx), HEX);
  }
  Serial.println();
#endif
  if(!(Pcm.PacketIdx % 4))
  {
    Pcm.BuildEndNblIdx = Pcm.BuildNblIdx - 1; // Used by ISR to ask main to compute BUILD_PCM_2_LAST
    Serial.print(F("Set Pcm.BuildEndNblIdx to "));Serial.println(Pcm.BuildEndNblIdx);
  }
  if(Pcm.BuildState == BUILD_PCM_2_LAST)
  {
    // Schedule here since this part is shorter than the first part (no Preamble, Sync, Odd/Even code)
#if defined(X_ANY)
//      Xany_scheduleTx_AllInstance();
#endif
  }
  Pcm.BuildState = BUILD_PCM_DO_NOTHING;
}

#define PCM_BIT_DURATION_HALF_US    (150 * 2)
const uint16_t ConsecBitDuration[] PROGMEM = {
                                     /*  0 */ (18 * PCM_BIT_DURATION_HALF_US), /* 0 is for Sync (18 bits) */
                                     /*  1 */ (1  * PCM_BIT_DURATION_HALF_US),
                                     /*  2 */ (2  * PCM_BIT_DURATION_HALF_US),
                                     /*  3 */ (3  * PCM_BIT_DURATION_HALF_US),
                                     /*  4 */ (4  * PCM_BIT_DURATION_HALF_US),
                                     /*  5 */ (5  * PCM_BIT_DURATION_HALF_US),
                                     /*  6 */ (6  * PCM_BIT_DURATION_HALF_US),
                                     /*  7 */ (7  * PCM_BIT_DURATION_HALF_US),
                                     /*  8 */ (8  * PCM_BIT_DURATION_HALF_US),
                                     /*  9 */ (9  * PCM_BIT_DURATION_HALF_US),
                                     /* 10 */ (10 * PCM_BIT_DURATION_HALF_US),
                                     /* 11 */ (11 * PCM_BIT_DURATION_HALF_US),
                                     /* 12 */ (12 * PCM_BIT_DURATION_HALF_US),
                                     /* 13 */ (13 * PCM_BIT_DURATION_HALF_US),
                                     /* 14 */ (14 * PCM_BIT_DURATION_HALF_US),
                                     /* 15 */ (15 * PCM_BIT_DURATION_HALF_US),
                                              };

static void PROTO_PCM_1024(void)
{
  uint8_t  ConsecBitNb;
  uint16_t half_us;

  ConsecBitNb = PcmStreamGetConsecBitNb(Pcm.TxNblIdx);
  half_us = (uint16_t)pgm_read_word(&ConsecBitDuration[ConsecBitNb]); // Use pre-computed values
//Serial.print(F("Pcm.TxNblIdx="));Serial.print(Pcm.TxNblIdx);Serial.print(F(" BitNb="));Serial.println(half_us/300);
  OCR1A  += half_us;
  if(Pcm.TxNblIdx == HALF_TX_NBL_IDX)
  {
    Pcm.BuildState = BUILD_PCM_2_FIRST;
//    Serial.println(F("\nISR -> BUILD_PCM_2_FIRST\n"));
  }
  else
  {
    if(Pcm.TxNblIdx >= Pcm.BuildEndNblIdx)
    {
      Pcm.TxNblIdx = 255; // Will become 0 after incrementation
      Pcm.BuildState = BUILD_PCM_2_LAST;
//      Serial.print(F("\nISR -> BUILD_PCM_2_LAST BuildEndNblIdx="));Serial.println(Pcm.BuildEndNblIdx);Serial.println(F("\n"));
    }
  }
  Pcm.TxNblIdx++;
//  delayMicroseconds(half_us / 2); // For test only
}

/* CPPM GENERATION INTERRUPT ROUTINE */
ISR(TIMER1_COMPA_vect)
{
  PROTO_PCM_1024();
}
