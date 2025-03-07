
/* 
  Universal Software Controlled PWM function library  for Arduino nano
  
  UniPWMControl - High level control class
  --------------------------------------------------------------------
  
  Copyright (C) 2014 Bernd Wokoeck
  
  Version history:
  1.02   07/09/2014  hilevel control class UniPWMControl
  1.03   01/15/2015  Sleep mode changes - dont let CPU sleep
                     Analog pin for voltage measurement can be defined in
                     SetLowBatt(...)
  1.04   03/04/2015  multiple input channels
  
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

**************************************************************/

#include "UniPWMControl.h"
#include <avr/sleep.h>


void UniPWMControl::Init( int nOutChannels, int nInpChannels )
{
  UniPWMInpChannel *pInpChannels = 0;
  UniPWMChannel    *pOutChannels = 0; 

  if( nOutChannels )
    pOutChannels = new UniPWMChannel[ nOutChannels ]; 

  if( nInpChannels )
    pInpChannels = new UniPWMInpChannel[ nInpChannels ];

  m_pPwm = new UniPWM( pOutChannels, nOutChannels, pInpChannels, nInpChannels );
}


void UniPWMControl::SetInpChannelPin( int pin, UniPWMInpChannel::enInpType logicType )
{
  m_pPwm->SetInpChannel( pin, logicType );  
}


void UniPWMControl::SetLowBatt( int lowVal, int patternNo, int pinAnalog  )
{
  m_lowBattVal       = lowVal;
  m_lowBattPattern   = patternNo;
  m_analogPinVoltage = pinAnalog;
}

    
void UniPWMControl::AddSequence( int patternNo, int pin, UNIPWMPHASE_PTR * ppPhases, int phaseCount, UniPWMChannel::enPwmType pwmType )
{
  Sequence * pSeq    = new Sequence;
  pSeq->m_patternNo  = patternNo;
  pSeq->m_pin        = pin;
  pSeq->m_ppPhases   = ppPhases;
  pSeq->m_phaseCount = phaseCount;
  pSeq->m_pwmType    = pwmType;
  Append( pSeq );
}


void UniPWMControl::Append( Sequence * pNewSeq )
{
  if( m_pFirstSeq == NULL )
  {
    m_pFirstSeq = pNewSeq;
  }
  else
  {
    Sequence * pSeq;
    for( pSeq = m_pFirstSeq; pSeq != NULL; pSeq = pSeq->m_pNext )
    {
      if( pSeq->m_pNext == NULL )
      {
        pSeq->m_pNext = pNewSeq;
        break;
      }
    }
  }
}


void UniPWMControl::AddInputSwitchPos( int low, int high, int patternNo )
{
  SwitchPos * pPos  = new SwitchPos;
  pPos->m_low       = low;
  pPos->m_high      = high;
  pPos->m_patternNo = patternNo;
  Append( pPos );
}


void UniPWMControl::Append( SwitchPos * pNewSwitchPos )
{
  if( m_pFirstSwitchPos == NULL )
  {
    m_pFirstSwitchPos = pNewSwitchPos;
  }
  else
  {
    SwitchPos * pSwitchPos;
    for( pSwitchPos = m_pFirstSwitchPos; pSwitchPos != NULL; pSwitchPos = pSwitchPos->m_pNext )
    {
      if( pSwitchPos->m_pNext == NULL )
      {
        pSwitchPos->m_pNext = pNewSwitchPos;
        break;
      }
    }
  }
}


void UniPWMControl::ActivatePattern( int patternNo )
{
  uint16_t channelMask = 0;

  if( patternNo < 0 )
    return;

  // determine channels to activate
  Sequence * pSeq;
  for( pSeq = m_pFirstSeq; pSeq != NULL; pSeq = pSeq->m_pNext )
    if( pSeq->m_patternNo == patternNo )
      channelMask |= _BV( pSeq->m_pin );

  m_pPwm->Stop( channelMask ); 
  for( pSeq = m_pFirstSeq; pSeq != NULL; pSeq = pSeq->m_pNext )
  {
    if( pSeq->m_patternNo == patternNo )
      m_pPwm->SetOutChannel( pSeq->m_pin, pSeq->m_ppPhases, pSeq->m_phaseCount, pSeq->m_pwmType ); 
  }
  m_pPwm->Start( channelMask ); 

  if( channelMask ) // were there channel activations at all ?
    m_activePatternNo = patternNo;
}


int UniPWMControl::GetPatternFromInputVal( int inpVal )
{
  SwitchPos * pSwitchPos;
  for( pSwitchPos = m_pFirstSwitchPos; pSwitchPos != NULL; pSwitchPos = pSwitchPos->m_pNext )
  {
    if( inpVal >= pSwitchPos->m_low && inpVal <= pSwitchPos->m_high )
      return pSwitchPos->m_patternNo;  
  }
  return -1;
}


void UniPWMControl::Sleep()
{
  ActivatePattern( m_lowBattPattern ); 
  /* delay( 1000 );
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode(); */
}


void UniPWMControl::DoLoop( uint16_t overrideReceiverInpVal )
{
  static uint16_t receiverInpVal = 0;
  static int      battVal = 0;
  static int      curPattern = 0;

  // battery low handling
  battVal = analogRead( m_analogPinVoltage ); 
  Serial.print( "batt: " ); 
  Serial.println( battVal ); 
  if( battVal < m_lowBattVal )
  {
    Serial.print( "low batt" ); 
    Sleep();
    return;
  }

  // get receiver input
  if( overrideReceiverInpVal )
    receiverInpVal = overrideReceiverInpVal;
  else 
    receiverInpVal = m_pPwm->GetInpChannelValue(); // input value from receiver (default pin)

  if( receiverInpVal != 0 )
  {
    Serial.print( "in " ); 
    Serial.println( receiverInpVal ); 
  }

  // evaluate switch position from receiver input
  curPattern = GetPatternFromInputVal( receiverInpVal );
  if( m_activePatternNo != curPattern )
  {
    Serial.print( "pattern " ); 
    Serial.println( curPattern ); 
    ActivatePattern( curPattern ); 
  }
}
