#include <FutPcm1024Rx.h>

/*
 Update 06/08/2022: add checksum check for each of the 4 received pcm packets in a frame V0.1 -> V0.2

 English: by RC Navy (2022)
 =======
  <FutPcm1024Rx>: a library for decoding FUTABA PCM1024 protocol
  Input Pin is 8 (to be connected to the PCM1024 signal)
  It should run on all Arduino based on the ATmega328 (UNO, Nano, Pro Mini)

 Francais: par RC Navy (2022)
 ========
  <FutPcm1024Rx>: une bibliotheque pour decoder le protocol PCM1024 de FUTABA
  La broche d'entree est la 8 (a coonecter sur le signal PCM1024)
  Elle devrait fonctionner sur tous les Arduinos bases sur l'ATmega328 (UNO, Nano, Pro Mini)
*/

#if not defined(__AVR_ATmega328P__)
#error This sketch SHALL use an Arduino based on the ATmega328 (UNO, Nano, Pro Mini)!
#endif

#define FUT_PCM1024_INPUT_PIN    8 // PB0 (ICP)

static uint16_t start, stop;
static uint16_t counts, width_us;

static uint8_t update = 0, inde = 0, bits = 0, last_index = 0;

#define DATA_BYTE_NB    60

static uint8_t  Data[DATA_BYTE_NB];
static uint16_t channel[10];

const uint8_t PcmCrcTbl[16] PROGMEM = {0x6B, 0xD6, 0xC7, 0xE5, 0xA1, 0x29, 0x52, 0xA4,
                                       0x23, 0x46, 0x8C, 0x73, 0xE6, 0xA7, 0x25, 0x4A};

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

/*******************************/
/* PRIVATE FUNCTION PROTOTYPES */
/*******************************/
static uint8_t ten2six(uint16_t word);
static uint8_t Extract_Data(void);

/********************/
/* PUBLIC FUNCTIONS */
/********************/
void FutPcm1024Rx_begin(void)
{
  pinMode(FUT_PCM1024_INPUT_PIN, INPUT_PULLUP);
  /* Timer1 configuration */
//  TCCR1A  = 0;           // normal counting mode
TCCR1A = _BV(COM1A0); // Toggle OC1A/OC1B on Compare Match.

  TCCR1B  = _BV(CS11);   // set prescaler of 8 -> 0.5 us per tick @ 16MHz
  TCNT1   = 0;           // clear the timer count
  TIFR1  |= _BV(ICF1);   // clear any pending interrupts;
  TIMSK1 |= _BV(ICIE1) ; // enable the input capture interrupt
}

uint8_t FutPcm1024Rx_available(void)
{
  uint8_t Ret = 0;

  if(update)
  {
    Ret = Extract_Data();
    update = 0;
  }

  return(Ret);
}

uint16_t FutPcm1024Rx_channelRaw(uint8_t ChId)
{
  return(channel[ChId]);
}

uint16_t FutPcm1024Rx_channelWidthUs(uint8_t ChId)
{
  return(920 + ((FutPcm1024Rx_channelRaw(ChId) * 20) / 17));
}

/*********************/
/* PRIVATE FUNCTIONS */
/*********************/
static uint8_t ten2six(uint16_t word)  // Convert ten bits to six bits
{
  uint8_t high = (word & 0x300) >> 8;
  uint8_t low = (word & 0x0FF) >> 0;

  if( high == 0x03 )
  {
    if( low == 0xF8 ) return 0x00; /* 1111111000 */
    if( low == 0xF3 ) return 0x01; /* 1111110011 */
    if( low == 0xE3 ) return 0x02; /* 1111100011 */
    if( low == 0xE7 ) return 0x03; /* 1111100111 */
    if( low == 0xC7 ) return 0x04; /* 1111000111 */
    if( low == 0xCF ) return 0x05; /* 1111001111 */
    if( low == 0x8F ) return 0x06; /* 1110001111 */
    if( low == 0x9F ) return 0x07; /* 1110011111 */

    if( low == 0x3F ) return 0x0B; /* 1100111111 */
    if( low == 0x1F ) return 0x0C; /* 1100011111 */
    if( low == 0x0F ) return 0x0D; /* 1100001111 */
    if( low == 0x87 ) return 0x0E; /* 1110000111 */
    if( low == 0xC3 ) return 0x0F; /* 1111000011 */

    if( low == 0xCC ) return 0x14; /* 1111001100 */
    if( low == 0x9C ) return 0x15; /* 1110011100 */
    if( low == 0x3C ) return 0x16; /* 1100111100 */
    if( low == 0x33 ) return 0x17; /* 1100110011 */
    if( low == 0xF0 ) return 0x18; /* 1111110000 */
    if( low == 0xE0 ) return 0x19; /* 1111100000 */
    if( low == 0x83 ) return 0x1A; /* 1110000011 */
    if( low == 0x07 ) return 0x1B; /* 1100000111 */
    if( low == 0x1C ) return 0x1C; /* 1100011100 */
    if( low == 0x98 ) return 0x1D; /* 1110011000 */
    if( low == 0x8C ) return 0x1E; /* 1110001100 */
    if( low == 0x38 ) return 0x1F; /* 1100111000 */

    if( low == 0x30 ) return 0x2C; /* 1100110000 */
    if( low == 0x18 ) return 0x2D; /* 1100011000 */
    if( low == 0x0C ) return 0x2E; /* 1100001100 */
    if( low == 0x03 ) return 0x2F; /* 1100000011 */

    if( low == 0xC0 ) return 0x35; /* 1111000000 */
    if( low == 0x80 ) return 0x36; /* 1110000000 */
    if( low == 0x00 ) return 0x37; /* 1100000000 */
  } else
  if( high == 0x00 )
  {
    if( low == 0xFF ) return 0x08; /* 0011111111 */
    if( low == 0x7F ) return 0x09; /* 0001111111 */
    if( low == 0x3F ) return 0x0A; /* 0000111111 */

    if( low == 0xFC ) return 0x10; /* 0011111100 */
    if( low == 0xF3 ) return 0x11; /* 0011110011 */
    if( low == 0xE7 ) return 0x12; /* 0011100111 */
    if( low == 0xCF ) return 0x13; /* 0011001111 */
    
    if( low == 0xC7 ) return 0x20; /* 0011000111 */
    if( low == 0x73 ) return 0x21; /* 0001110011 */
    if( low == 0x67 ) return 0x22; /* 0001100111 */
    if( low == 0xE3 ) return 0x23; /* 0011100011 */
    if( low == 0xF8 ) return 0x24; /* 0011111000 */
    if( low == 0x7C ) return 0x25; /* 0001111100 */
    if( low == 0x1F ) return 0x26; /* 0000011111 */
    if( low == 0x0F ) return 0x27; /* 0000001111 */
    if( low == 0xCC ) return 0x28; /* 0011001100 */
    if( low == 0xC3 ) return 0x29; /* 0011000011 */
    if( low == 0x63 ) return 0x2A; /* 0001100011 */
    if( low == 0x33 ) return 0x2B; /* 0000110011 */

    if( low == 0x3C ) return 0x30; /* 0000111100 */
    if( low == 0x78 ) return 0x31; /* 0001111000 */
    if( low == 0xF0 ) return 0x32; /* 0011110000 */
    if( low == 0xE0 ) return 0x33; /* 0011100000 */
    if( low == 0xC0 ) return 0x34; /* 0011000000 */

    if( low == 0x60 ) return 0x38; /* 0001100000 */
    if( low == 0x70 ) return 0x39; /* 0001110000 */
    if( low == 0x30 ) return 0x3A; /* 0000110000 */
    if( low == 0x38 ) return 0x3B; /* 0000111000 */
    if( low == 0x18 ) return 0x3C; /* 0000011000 */
    if( low == 0x1C ) return 0x3D; /* 0000011100 */
    if( low == 0x0C ) return 0x3E; /* 0000001100 */
    if( low == 0x07 ) return 0x3F; /* 0000000111 */
  }

  return 0xFF;
}

static uint8_t Extract_Data(void)
{
  uint16_t  start_data = 0, ind = 0, m, check_sum = 0, pos[4], byte[16];
  uint8_t value = 0 , bit_counter = 0, n, packet_byte[16], frame = 0, aux[4], dif[4], crc[4], ComputedEcc;
  Fut24BitPcmPacket_Union Fut24BitPcmPacket;

  if (Data[0] != 18)
  {
    return 0;
  }
  if (Data[1] == 4)
  {
    frame = 1;
  }
  else
  {
    frame = 2;
  }
  if (Data[2] > 2)
  {
    Data[2] -= 2;
    start_data = 2;
    value = 1;
  }
  else
  {
    start_data = 3;
    value = 0;
  }
  
  for (ind = 0; ind <= 15; ind++)
  {
    for (m = start_data; m <= (last_index - 1); m++)
    { 
      for (n = 1; n <= Data[m]; n++)
      {
        byte[ind]  = byte[ind] << 1;
        byte[ind] |= value;
        bit_counter++;
        if (bit_counter == 10)
        {
          if (n < Data[m])
          {
            Data[m] -= n;
            start_data = m;
            value--;
          }
          else
          {
            start_data = m + 1;
          }
          bit_counter = 0;
          break;
        }
      }
      value++;
      if(value >= 2)
      {
        value = 0;
      }
      if (bit_counter == 0)
      {
        break;
      }
    }
  }
  
  for (ind = 0; ind <= 15; ind++)
  {
    packet_byte[ind] = ten2six(byte[ind]);
    if (packet_byte[ind] == 0xff)
    {
      return 0;
    }
  }
  
  for (ind = 0; ind <= 3; ind++)
  {
    aux[ind]=(packet_byte[4 * ind + 0] & 0x30) >> 4;
    dif[ind]=(packet_byte[4 * ind + 0] & 0x0F) >> 0;
    pos[ind]=(packet_byte[4 * ind + 1] & 0xFF) << 4 | (packet_byte[4 * ind + 2] & 0x3C) >> 2;
    crc[ind]=(packet_byte[4 * ind + 2] & 0x03) << 6 | (packet_byte[4 * ind + 3] & 0x3F) >> 0;

    Fut24BitPcmPacket.AuxBit1  = !!(aux[ind] & 0x02);
    Fut24BitPcmPacket.AuxBit0  = !!(aux[ind] & 0x01);
    Fut24BitPcmPacket.Delta    = dif[ind];
    Fut24BitPcmPacket.Position = pos[ind];
    Fut24BitPcmPacket.Ecc      = crc[ind];
    //Compute ECC
    ComputedEcc = 0;
    for(uint8_t BitIdx = 0; BitIdx < 16; BitIdx++)
    {
      if(Fut24BitPcmPacket.Value & (1UL << (8 + BitIdx)))
      {
        ComputedEcc ^= (uint8_t)pgm_read_byte(&PcmCrcTbl[BitIdx]);
      }
    }
    if(ComputedEcc != Fut24BitPcmPacket.Ecc)
    {
      Serial.print(F("Error CRC for index "));Serial.println(ind); // To be cleaned later: remove display and jus ignore corrupted packets
    }
  }

  if (frame == 1)
  {
    if(aux[0] == 2)
    {
      channel[1] = pos[0];
      channel[2] += (dif[0] - 8) * 4;
    }
    if(aux[1] == 0)
    {
      channel[3] = pos[1];
      channel[4] += (dif[1] - 8) * 4;
    }
    if(aux[2] == 2)
    {
      channel[5] = pos[2];
      channel[6] += (dif[2] - 8) * 4;
    }
    if(aux[3] == 0)
    {
      channel[7] = pos[3];
      channel[8] += (dif[3] - 8) * 4;
      channel[9] = 40;
    }
    if(aux[3]==1)
    {
      channel[7] = pos[3];
      channel[8] += (dif[3] - 8) * 4;
      channel[9] = 984;
    }
    
  }
  if (frame == 2)
  {
    if(aux[0] == 2)
    {
      channel[2] = pos[0];
      channel[1] += (dif[0] - 8) * 4;
    }
    if(aux[1] == 0)
    {
      channel[4] = pos[1];
      channel[3] += (dif[1] - 8) * 4;
    }
    if(aux[2] == 2)
    {
      channel[6] = pos[2];
      channel[5] += (dif[2] - 8) * 4;
    }
    if(aux[3] == 0)
    {
      channel[8] = pos[3];
      channel[7] += (dif[3] - 8) * 4;
      channel[9] = 40;
    }
    if(aux[3] == 1)
    {
      channel[8]  = pos[3];
      channel[7] += (dif[3] - 8) * 4;
      channel[9]  = 984;
    }
  }
  for (ind = 1; ind <= 9; ind++)
  {
    check_sum ^= channel[ind];
  }
#if 0
  Serial.println(channel[1]);
  Serial.println(channel[2]);
  Serial.println(channel[3]);
  Serial.println(channel[4]);
  Serial.println(channel[5]);
  Serial.println(channel[6]);
  Serial.println(channel[7]);
  Serial.println(channel[8]);
  Serial.println(channel[9]);
  Serial.println(check_sum);
  Serial.println();
#endif
  
  return 1;
}

/*****************************/
/* INTERRUPT SERVICE ROUTINE */
/*****************************/
ISR(TIMER1_CAPT_vect)
{
  stop = ICR1;                    
  counts = (stop - start);    //Calculate Ticks per width 
  width_us = counts >> 1;     //convert ticks to us
  bits = width_us / 150;
  if ( (width_us % 150) > 120)//convert width to bit (each bit 150 us)
  {
    bits++;
  }
  if (bits == 18)         // start of header 18 bit = 2700 us
  {
    last_index = inde;
    inde = 0;
    update = 1;
  }
  else
  {
    if(inde < (DATA_BYTE_NB - 1))
    {
      inde++; // Prevent Data buffer overflow!
    }
  }
  Data[inde] = bits;       //store bits in array
  
  if (PINB & 0x01)         // ICP pin is high
  {
    TCCR1B &= ~_BV(ICES1); //set to trigger on falling edge
  }
  else
  {
    TCCR1B |= _BV(ICES1);  //rising edge triggers next
  }
  start = ICR1;
}
