#include "Conrad7Tx.h"
/*

 English: by RC Navy (2021)
 =======
 09/05/2021: V0.0 First release of the Conrad7Tx library
 <Conrad7Tx>: a library to simply control with 2 pins the 7 outputs of the 7-channel multi-switcher Ref 0115 541(Kit)/0231 517(Assembled/Tested) from Conrad
 IMPORTANT: a 100K pulldown resistor is required between K1 Signal and GND and K2 Signal and GND, otherwise, the neutral auto calibration may fail!
 26/06/2021: V1.1 Correction of a problem in case of too close commands
 http://p.loussouarn.free.fr/contact.html

 
 Francais: par RC Navy (2021)
 ========
 09/05/2021: V0.0 Premiere release de la bibliotheque Conrad7Tx
 <Conrad7Tx>: une bibliotheque pour controler simplement a partir de 2 broches les 7 sorties du module Commutateur multiple 7 canaux Ref 0115 541(kit)/0231 517(Assemble/teste) de chez Conrad
 IMPORTANT: une resistance de 100K est requise entre le Signal de K1 et GND ainsi qu'entre Signal de K2 et GND, sinon, l'auto calibration du neutre peut echouer!
 26/06/2021: V1.1 Correction d'un probleme en cas de commandes trop rapprochees
 http://p.loussouarn.free.fr/contact.html
 
*/

/*

Channel K1 for Horizontal potentiomenter
    _____                           _____ 
 __|  H  |_______  around 18 ms  __|  H  |___________
   <------------------------------>
             around 20 ms

Channel K2 Vertical potentiomenter
         _____                            _____             
 _______|  V  |__  around 18 ms  ________|  V  |_____

 Note:
 ====
 The J1 jumper shall be placed close to the LED8.
 
*/

/* Global variables */
Conrad7TxClass    Conrad7Tx = Conrad7TxClass();

#define CONRAD7_OUTPUT_NB                7

#define NEUTRAL_AUTO_LEARNING_TIME_MS    3000
#define EXC_MAX_US                       400
#define ORDER_CONFIRMATION_PULSE_NB      10 /* Do not reduce this value: it is set at the minimum expected by the Conrad Module */

enum {HORIZONTAL = 0, VERTICAL, DIR_NB};

enum {RC_GEN_WAIT_FOR_SYNCHRO = 0, RC_GEN_START_H_PULSE, RC_GEN_WAIT_FOR_END_OF_H_PULSE, RC_GEN_START_V_PULSE, RC_GEN_WAIT_FOR_END_OF_V_PULSE};

enum {C7_STATE_INIT = 0, C7_STATE_NEUTRAL_AUTO_LEARNING, C7_STATE_INTER_CMD, C7_STATE_UPDATE_ORDER, C7_STATE_CONFIRM_ORDER};

/*
  Stick positions:
  ===============
  
  7        1        2
  ^        ^        ^
   \       |       /
      \    |    /
         \ | /
 6<--------O-------->3
         / | \
      /    |    \
   /       |       \
  v        v        v
  5     Special     4
        Function

Note:
====

With the firmware of the PIC16F54 provided with my sample of 7-channel multi-switcher Ref 231517 from Conrad, the Output id didn't match with the expected RC pulses.

In my case:
- Position#1 activated Output#7
- Position#2 activated Output#6
  ...
- Position#7 activated Output#1

The excursion table below fixes this issue.

*/
const int8_t Exc[] PROGMEM = {
                               -1,+1, // Idx = 0 -> Pos#1 -> Ouptput#1
                               -1,+0, // Idx = 1 -> Pos#2 -> Ouptput#2
                               -1,-1, // Idx = 2 -> Pos#3 -> Ouptput#3
                               +1,-1, // Idx = 3 -> Pos#4 -> Ouptput#4
                               +1,+0, // Idx = 4 -> Pos#5 -> Ouptput#5
                               +1,+1, // Idx = 5 -> Pos#6 -> Ouptput#6
                               +0,+1, // Idx = 6 -> Pos#7 -> Ouptput#7
                               +0,+0  // Idx = 7 -> For neutral auto learning
                             };

#define GET_EXC_US(OutIdx, Dir) (int8_t)pgm_read_byte(&Exc[(2 * (OutIdx)) + (Dir)])

/* Constructor */
Conrad7TxClass::Conrad7TxClass()
{

}

/* Conrad7Tx.begin() shall be called in the setup() */
void Conrad7TxClass::begin(uint8_t Hpin, uint8_t Vpin, uint8_t *InCmd)
{
  C7.Hpin  = Hpin;
  C7.Vpin  = Vpin;
  C7.SrcInCmd = InCmd;
  pinMode(C7.Hpin, OUTPUT); /* Hpin is connected to K1 channel of the Conrad Module: IMPORTANT: This pin SHALL have an external 100K pull-down! */
  pinMode(C7.Vpin, OUTPUT); /* Vpin is connected to K2 channel of the Conrad Module */
  C7.InCmd  = 0;
  C7.OutCmd = 0;
  C7.MainState   = C7_STATE_INIT;
  C7.RcGen.State = RC_GEN_WAIT_FOR_SYNCHRO;
}

/* Conrad7Tx.begin() shall be called every #20ms */
void Conrad7TxClass::updateOrder(void) /* This method shall be called every #20 ms */
{
  uint8_t BitIdx, OutChange;

  switch(C7.MainState)
  {
    case C7_STATE_INIT:
#if (CONRAD7TX_DEBUG > 0)
    if(C7.RcGen.Debug)
    {
        Serial.print(millis());Serial.println(F(": Init"));
    }
#endif
    C7.InitStartMs = millis16();
    C7.RcGen.OutputIdx = CONRAD7_OUTPUT_NB;
    C7.MainState = C7_STATE_NEUTRAL_AUTO_LEARNING;
    C7.RcGen.State = RC_GEN_START_H_PULSE;
    break;

    case C7_STATE_NEUTRAL_AUTO_LEARNING:
    if(ElapsedMs16Since(C7.InitStartMs) >= NEUTRAL_AUTO_LEARNING_TIME_MS)
    {
      if(C7.RcGen.State == RC_GEN_WAIT_FOR_SYNCHRO)
      {
#if (CONRAD7TX_DEBUG > 0)
        if(C7.RcGen.Debug)
        {
            Serial.print(millis());Serial.println(F(": Init done"));
        }
#endif
        C7.MainState = C7_STATE_UPDATE_ORDER;
      }
    }
    else
    {
      if(C7.RcGen.State == RC_GEN_WAIT_FOR_SYNCHRO)
      {
        C7.RcGen.State = RC_GEN_START_H_PULSE; // Restart pulse generation
      }
    }
    break;
    
    case C7_STATE_INTER_CMD: /* This step is mandatory to avoid inconsistant final status when 2 same consecutive commands are too closed together */
    C7.RcGen.OutputIdx = CONRAD7_OUTPUT_NB;
    C7.RcGen.State = RC_GEN_START_H_PULSE;
    C7.MainState = C7_STATE_UPDATE_ORDER;
    break;
    
    case C7_STATE_UPDATE_ORDER:
    OutChange = C7.InCmd ^ C7.OutCmd;
    if(!OutChange)
    {
      C7.InCmd = *C7.SrcInCmd; //Make here with a copy since C7.SrcInCmd may change quickly (to mark it later as done)
#if (CONRAD7TX_DEBUG > 0)
      if(C7.RcGen.Debug)
      {
        Serial.print(F("Reload cmd: 0x"));Serial.println(C7.InCmd, HEX);
      }
#endif
    }
    OutChange = C7.InCmd ^ C7.OutCmd;
    for(BitIdx = 0; BitIdx < CONRAD7_OUTPUT_NB; BitIdx++)
    {
      if(bitRead(OutChange, BitIdx)) break;
    }
    C7.RcGen.OutputIdx = BitIdx;
    if(OutChange)
    {
#if (CONRAD7TX_DEBUG > 0)
      if(C7.RcGen.Debug && (C7.RcGen.OutputIdx != CONRAD7_OUTPUT_NB))
      {
          Serial.print(millis());Serial.print(F(": Bit["));Serial.print(C7.RcGen.OutputIdx);Serial.print(F("] changed to "));Serial.println(bitRead(C7.InCmd, BitIdx));
      }
#endif
      C7.RcGen.ConfirmNb = 0;
      C7.MainState = C7_STATE_CONFIRM_ORDER;
    }
    C7.RcGen.State = RC_GEN_START_H_PULSE;
    break;

    case C7_STATE_CONFIRM_ORDER:
    if(C7.RcGen.ConfirmNb >= ORDER_CONFIRMATION_PULSE_NB)
    {
      if(C7.RcGen.State == RC_GEN_WAIT_FOR_SYNCHRO)
      {
        bitWrite(C7.OutCmd, C7.RcGen.OutputIdx, bitRead(C7.InCmd, C7.RcGen.OutputIdx)); // Mark cmd as done
        C7.MainState = C7_STATE_INTER_CMD;
      }
    }
    else
    {
      if(C7.RcGen.State == RC_GEN_WAIT_FOR_SYNCHRO)
      {
        C7.RcGen.State = RC_GEN_START_H_PULSE; // Restart pulse generation
      }
    }
    break;
  }
}

/* Conrad7Tx.process() shall be in the loop() (Blocking functions such as delay() are forbidden in the loop() to make it work) */
void Conrad7TxClass::process(void) // This method SHALL be called in the loop()
{
  int8_t  Hexc, Vexc;
  
  switch(C7.RcGen.State)
  {
    case RC_GEN_WAIT_FOR_SYNCHRO:
    /* Do nothing, just wait */
    break;
    
    case RC_GEN_START_H_PULSE:
#if (CONRAD7TX_DEBUG > 0)
    Hexc = GET_EXC_US(C7.RcGen.OutputIdx, HORIZONTAL);
    Vexc = GET_EXC_US(C7.RcGen.OutputIdx, VERTICAL);
    if(C7.RcGen.Debug && (Hexc || Vexc))
    {
        Serial.print(millis());Serial.print(F(": H="));Serial.print((uint16_t)(1500 + (Hexc * EXC_MAX_US)));
    }
#endif
    C7.RcGen.StartUs = micros16();
    C7.RcGen.State = RC_GEN_WAIT_FOR_END_OF_H_PULSE;
    digitalWrite(C7.Hpin, HIGH);
    break;
    
    case RC_GEN_WAIT_FOR_END_OF_H_PULSE:
    Hexc = GET_EXC_US(C7.RcGen.OutputIdx, HORIZONTAL);
    if(ElapsedUs16Since(C7.RcGen.StartUs) >= (uint16_t)(1500 + (Hexc * EXC_MAX_US)))
    {
        digitalWrite(C7.Hpin, LOW);
        C7.RcGen.State = RC_GEN_START_V_PULSE;
    }
    break;
        
    case RC_GEN_START_V_PULSE:
#if (CONRAD7TX_DEBUG > 0)
    Hexc = GET_EXC_US(C7.RcGen.OutputIdx, HORIZONTAL);
    Vexc = GET_EXC_US(C7.RcGen.OutputIdx, VERTICAL);
    if(C7.RcGen.Debug && (Hexc || Vexc))
    {
        Vexc = GET_EXC_US(C7.RcGen.OutputIdx, VERTICAL);
        Serial.print(F(" V="));Serial.println((uint16_t)(1500 + (Vexc * EXC_MAX_US)));
    }
#endif
    C7.RcGen.StartUs = micros16();
    digitalWrite(C7.Vpin, HIGH);
    C7.RcGen.State = RC_GEN_WAIT_FOR_END_OF_V_PULSE;
    break;
    
    case RC_GEN_WAIT_FOR_END_OF_V_PULSE:
    Vexc = GET_EXC_US(C7.RcGen.OutputIdx, VERTICAL);
    if(ElapsedUs16Since(C7.RcGen.StartUs) >= (uint16_t)(1500 + (Vexc * EXC_MAX_US)))
    {
        digitalWrite(C7.Vpin, LOW);
        C7.RcGen.State = RC_GEN_WAIT_FOR_SYNCHRO;
        C7.RcGen.ConfirmNb++;
    }
    break;
  }
}

#if (CONRAD7TX_DEBUG > 0)
void Conrad7TxClass::debugProtocol(uint8_t OffOn)
{
    C7.RcGen.Debug = !!OffOn;
}
#endif
