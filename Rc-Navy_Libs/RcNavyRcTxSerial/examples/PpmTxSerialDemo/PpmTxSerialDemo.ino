#include <Rcul.h>

#include <TinyPinChange.h>
#include <TinyCppmReader.h>

#include <TinyCppmGen.h>
#include <RcTxSerial.h>
/*
This sketch demonstrates how to inject an additional RC channel to a PPM frame and how to transport message over this additional channel.
At reception side, the RxSerial library is needed to extract the message from the channel corresponding to the additional channel.
The serial port can be used to transport a message containing the status of a set of switches: this allows to add a multi-switch to the 
Radio-Control set.


                                   <TinyPinChange.h>
                                   <TinyCppmReader.h>           <TinyCppmGen.h>
                                   .---------------.          .---------------.
                                   |               | Forward  |               |
  Input PPM Frame (4 channels) --->| TinyCppmReader |--------->|  TinyCppmGen   |----> Output PPM Frame (5 channels)
                                   |               | Channels |               |
                                   '---------------'          '---------------'
                                                                      ^
                                                                      | Use the additional 5th channel to send messsages
                                                              .---------------.
                                                              |               |
                                MyRcTxSerial.print("msg") --->| MyRcTxSerial  |
                                                              |               |
                                                              '---------------'
                                                               <RcTxSerial.h>
*/
#define PPM_INPUT_PIN   2

#define DATA_RC_CHANNEL 5

RcTxSerial MyRcTxSerial(&TinyCppmGen, RC_TX_SERIAL_REPEAT1, 16, DATA_RC_CHANNEL); /* Create a serial port on the channel#5 of the TinyCppmGen with a tx fifo of 16 bytes (/!\ Data rate = 200 bauds /!\) */

uint32_t StartMs = millis();

void setup()
{
  TinyCppmReader::attach(PPM_INPUT_PIN); /* Attach MyPpmReader to PPM_INPUT_PIN pin */
  TinyCppmGen.begin(TINY_CPPM_GEN_POS_MOD, DATA_RC_CHANNEL); /* Generate 5 channels. Change TINY_PPM_GEN_POS_MOD to TINY_PPM_GEN_NEG_MOD for NEGative PPM modulation */
}

void loop()
{
  if((TinyCppmReader::detectedChannelNb() >= 4) && TinyCppmGen.isSynchro())
  {
    TinyCppmGen.setChWidth_us(1, TinyCppmReader::width_us(1)); /* RC Channel#1: forward rx value */
    TinyCppmGen.setChWidth_us(2, TinyCppmReader::width_us(2)); /* RC Channel#2: forward rx value */
    TinyCppmGen.setChWidth_us(3, TinyCppmReader::width_us(3)); /* RC Channel#3: forward rx value */
    TinyCppmGen.setChWidth_us(4, TinyCppmReader::width_us(4)); /* RC Channel#4: forward rx value */
  }
  if(millis() - StartMs >= 250) /* /!\ 250ms Clock used to not flood the serial link (Data rate = 200 Bauds) /!\ */
  {
    StartMs = millis(); /* Restart chrono */
    MyRcTxSerial.print(millis()); /* RC Channel#5: send a message (milliseconds elapsed since the power-up) using PCM over PPM (TM: RC Navy) */
  }
  RcTxSerial::process();
}

