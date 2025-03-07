
/* 
  Universal Software Controlled PWM function library  for Arduino nano
  
  UniPWMControl - high level control class
  --------------------------------------------------------------------
  
  Copyright (C) 2014 Bernd Wokoeck
  
  Version history:
  1.02   07/09/2014  hilevel control class UniPWMControl
  1.03   01/15/2015  Sleep mode changes - dont let CPU sleep
                     Analog pin for voltage measurement can be defined in
                     SetLowBatt(...)
                     You can provide the receiver input value in DoLoop(...)
                     now, in order for easier testing and debugging
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

#ifndef UNIPWMCONTROL_H
#define UNIPWMCONTROL_H

#include "UniPWM.h"
#include <new.h>

class UniPWMControl
{
public:
  UniPWMControl() : m_pPwm( 0 ), m_pFirstSeq( 0 ), m_pFirstSwitchPos( 0 ), m_activePatternNo( -1 ), m_lowBattVal( 0 ), m_lowBattPattern( -1 ), m_analogPinVoltage( 0 ) {}

  void Init( int nOutChannels, int nInpChannels = 1 );
  void SetInpChannelPin( int pin, UniPWMInpChannel::enInpType logicType = UniPWMInpChannel::INP_NORMAL );
  void SetLowBatt( int lowVal, int patternNo, int pinAnalog = 0x0 );
    
  void AddSequence( int patternNo, int pin, UNIPWMPHASE_PTR * ppPhases, int phaseCount, UniPWMChannel::enPwmType pwmType  = UniPWMChannel::SOFTPWM );
  void AddInputSwitchPos( int low, int high, int patternNo );

  void ActivatePattern( int patternNo ); 

  uint16_t GetInputChannelValue( int pin ){ if( m_pPwm ) return m_pPwm->GetInpChannelValue( pin ); return 0; };

  void DoLoop( uint16_t overrideReceiverInpVal = 0 );

  UniPWM           * m_pPwm; // ( m_pChannels, numChannels );
protected:

  int                GetPatternFromInputVal( int inpVal );
  int                m_activePatternNo;

  int                m_lowBattVal;
  int                m_lowBattPattern;
  int                m_analogPinVoltage;

  void Sleep();

  class Sequence
  {
  public:
    Sequence() : m_patternNo( -1 ), m_pin( 0 ), m_ppPhases( 0 ), m_phaseCount( 0 ), m_pwmType( UniPWMChannel::SOFTPWM ), m_pNext( 0 ) {}

    int                      m_patternNo;
    int                      m_pin;
    UNIPWMPHASE_PTR *        m_ppPhases;
    int                      m_phaseCount;
    UniPWMChannel::enPwmType m_pwmType;
    Sequence *               m_pNext;
  };
  Sequence * m_pFirstSeq;
  void Append( Sequence * pSeq ); 

  class SwitchPos
  {
  public:
    SwitchPos() : m_low( 0 ), m_high( 0 ), m_patternNo( 0 ), m_pNext( 0 ) {}

    int         m_low;
    int         m_high;
    int         m_patternNo;
    SwitchPos * m_pNext;
  };
  SwitchPos * m_pFirstSwitchPos;
  void Append( SwitchPos * pSwitchPos ); 
};

#endif // UNIPWMCONTROL_H

