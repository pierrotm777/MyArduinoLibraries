
/* 
  Universal Software Controlled PWM function library  for Arduino nano

  UniPWM base library classes
  --------------------------------------------------------------------
  
  Copyright (C) 2014 Bernd Wokoeck
  
  Version history:
  1.00   06/04/2014  created
  1.02   07/09/2014  extensions for hilevel control class UniPWMControl
  1.04   03/04/2015  multiple input channels
  1.05   03/07/2015  storage structure modified to separate workdata from static data
                     (no separate SEQUENCE() for each pin needed anymore)
  
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

#include <avr/io.h>
#include <avr/interrupt.h>

#include "UniPWM.h"

// #define _DEBUG
#ifdef _DEBUG
  #define IRQ_DEBUG_PIN 0x0d
#endif 

#define UNIPWM_FREQ 96UL // irq frequency 48 = 80us PWM clock, use 48UL or 96UL
#define UNIPWM_OCR_VAL (F_CPU/(UNIPWM_FREQ * 2048UL)) // output control rate

// static workspace
volatile uint8_t  s_irq_cnt = 0xff;  // full PWM cycle = 256 * 80us ~ 20 ms = 50 Hz
UniPWM *          s_pInstance = 0;   // instance pointer to find the UniPWM object

// timer2 interrupt service routine
///////////////////////////////////
ISR( TIMER2_COMPA_vect )
{
#ifdef _DEBUG
  digitalWrite(IRQ_DEBUG_PIN, 1);
#endif 

  int i; 

  // process input channel
  // scan input pin for high level period
  UniPWMInpChannel * pInpChannel; 
  for( pInpChannel = s_pInstance->m_pInpChannels, i = 0; i < s_pInstance->m_inpChannelCount; i++, pInpChannel++ ) // for all input channels
  {
    pInpChannel->ISR_MeasurePulse();
  }

  // process output channels
  // start pwm cycle and control sequence through phases
  // every 256 pwm clock cycles
  UniPWMChannel * pChannel;
  if( ++s_irq_cnt == 0 )
  {
	  for( pChannel = s_pInstance->m_pChannels, i = 0; i < s_pInstance->m_channelCount; i++, pChannel++ ) // for all output channels
	  {
		  // reset all outputs
		  if( pChannel->m_pin ) // pin is initalized
		  {
        // get pwm value for current phase
        pChannel->m_channelPwmVal = pChannel->m_pCurPhase->getPwmVal( pChannel->m_curCycle, &pChannel->m_workData );  // get new pwm val for this cycle

        // software pwm
				if( pChannel->m_pwmType & UniPWMChannel::SOFTPWM_BIT )
        {
          if( pChannel->m_channelPwmVal ) // if not "off" (=0)
          {
            // switch pin on
    				if( pChannel->m_pwmType & UniPWMChannel::INVERTED_BIT )
   				    *(pChannel->m_pPort) &= ~pChannel->m_pinMask; // change pin state to low
            else
   				    *(pChannel->m_pPort) |= pChannel->m_pinMask; // change pin state to high
          }
        }
        // hardware pwm
				else if( pChannel->m_pwmType & UniPWMChannel::FASTPWM16_BIT )
        {
          // 16 bit (timer 1 and pin 9)
          pChannel->SetFastPWMValue( (8*256UL) + pChannel->m_channelPwmVal * 8UL ); // servo minimim value (=8*256) + 8 * pwm value (max 255) --> 0...4095 out of 65535
        }
        else
        {
          // 8 bit arduino standard pwm function
          analogWrite( pChannel->m_pin, pChannel->m_channelPwmVal ); // hardware PWM - DO NOT USE PINS 3 and 11
        }

			  // next sequence
			  if( pChannel->m_maxCycles ) // if pwm is not constant
			  {
			    pChannel->m_curCycle++;
				  if( pChannel->m_curCycle >= pChannel->m_maxCycles ) // cycle overrun
				  {
					  pChannel->m_curCycle = 0;

					  pChannel->m_idxCurPhase++;
					  if( pChannel->m_idxCurPhase >= pChannel->m_phaseCount ) // phase overrun
            {
						  pChannel->m_idxCurPhase = 0;
              // todo here we could switch to a new sequence with no phase loss
            }

					  // working set for new phase
					  pChannel->m_pCurPhase = pChannel->m_ppPhases[ pChannel->m_idxCurPhase ];
					  pChannel->m_maxCycles = pChannel->m_pCurPhase->m_nCycles; 
				  }
			  }
		  }
	  }
  }

  // generate pwm signal
  // every 80 us for all channels
  for( pChannel = s_pInstance->m_pChannels, i = 0; i < s_pInstance->m_channelCount; i++, pChannel++ ) 
  {
	  // check for pulse end...
		if( pChannel->m_channelPwmVal == s_irq_cnt ) // do this check first to accelerate execution time, since this is the least likely case to be true
    {
	    if( pChannel->m_pwmType & UniPWMChannel::SOFTPWM_BIT && pChannel->m_pin )
	    {
        // ... and switch off
        if( pChannel->m_pwmType & UniPWMChannel::INVERTED_BIT )
		     *(pChannel->m_pPort) |= pChannel->m_pinMask;                              // change pin state to high
        else
		     *(pChannel->m_pPort) &= ~pChannel->m_pinMask;                             // change pin state to low
      }
	  }
  }  

#ifdef _DEBUG
  digitalWrite(IRQ_DEBUG_PIN, 0);
#endif
}


// UniPWM
/////////
void UniPWM::Start( uint16_t channelMask )
{
	cli();

#ifdef _DEBUG
    pinMode( IRQ_DEBUG_PIN, OUTPUT );
#endif 
    
	s_pInstance =  this;

	int i;
	for( i = 0; i < m_channelCount; i++ ) // for all channels
	{
		if( m_pChannels[ i ].m_pin && ( channelMask & _BV( m_pChannels[ i ].m_pin ) ) )
		{
			// reset working set
			m_pChannels[ i ].m_curCycle = 0;
			m_pChannels[ i ].m_idxCurPhase = 0;
			m_pChannels[ i ].m_pCurPhase = m_pChannels[ i ].m_ppPhases[ 0 ];
			m_pChannels[ i ].m_maxCycles = m_pChannels[ i ].m_pCurPhase->m_nCycles; 
		}
	}

	// start irq timer 
  UNIPWM_TIMER_INIT( UNIPWM_OCR_VAL );
  
  sei();
}


void UniPWM::Stop( uint16_t channelMask )
{
	cli();

	int i;
	for( i = 0; i < m_channelCount; i++ )
	{
    if( channelMask & _BV( m_pChannels[ i ].m_pin ) )
	    m_pChannels[ i ].SetFree();
	}

	sei();
}


void UniPWM::SetOutChannel( int pin, UNIPWMPHASE_PTR * ppPhases, int phaseCount, UniPWMChannel::enPwmType pwmType /* = 1 */ )
{
	int i;
	for( i = 0; i < m_channelCount; i++ )
	{
		// first reuse existing channel...
		if( m_pChannels[ i ].IsPin( pin ) )
		{
			m_pChannels[ i ].Init( pin, ppPhases, phaseCount, pwmType );
			goto EXIT;
		}
	}

	for( i = 0; i < m_channelCount; i++ )
	{
		// ... then search for free channel 
		if( m_pChannels[ i ].IsFree() )
		{
			m_pChannels[ i ].Init( pin, ppPhases, phaseCount, pwmType );
			break;
		}
	}

EXIT:
	;
}


void UniPWM::SetInpChannel( int pin, UniPWMInpChannel::enInpType inpType )
{
	int i;
	for( i = 0; i < m_inpChannelCount; i++ )
	{
		// first reuse existing channel...
		if( m_pInpChannels[ i ].IsPin( pin ) )
		{
      if( inpType != UniPWMInpChannel::INP_DISABLED )
			  m_pInpChannels[ i ].Init( pin, inpType );
      else
			  m_pInpChannels[ i ].SetFree();
			goto EXIT;
		}
	}

  if( inpType != UniPWMInpChannel::INP_DISABLED )
  {
	  for( i = 0; i < m_inpChannelCount; i++ )
	  {
		  // ... then search for free channel 
		  if( m_pInpChannels[ i ].IsFree() )
		  {
			  m_pInpChannels[ i ].Init( pin, inpType );
			  break;
		  }
	  }
  }

EXIT:
	;
}


uint16_t UniPWM::GetInpChannelValue( int pin )
{
  // get default input pin value (the first one which has been set)
  if( pin == 0 )
  {
    if( m_inpChannelCount && !m_pInpChannels[ 0 ].IsFree() )
      return m_pInpChannels[ 0 ].GetValue();
  }
  // get a specific pin value
  else
  {
    int i;
	  for( i = 0; i < m_inpChannelCount; i++ )
	  {
		  if( m_pInpChannels[ i ].IsPin( pin ) )
        return m_pInpChannels[ i ].GetValue();
    }
  }

  return 0;  
}


// InpChannel
/////////////
void UniPWMInpChannel::Init( int pin, enInpType inpType )
{
  if( m_pin != pin )
  {
    pinMode( pin, INPUT_PULLUP );
    m_pin     = pin;
    m_inpType = inpType;

    m_inpVal   = 0;
    m_lastVal  = 0;
    m_pPort    = portInputRegister(digitalPinToPort(pin)); // do not use A6/A7 as digital input ports !!!
    m_pinMask  = digitalPinToBitMask(pin);
  }
}

inline void UniPWMInpChannel::ISR_MeasurePulse()
{
  if( m_pin ) 
  {
    uint8_t in = *m_pPort & m_pinMask;
    if( m_inpType & UniPWMInpChannel::INP_INVERTED )
      in ^= m_pinMask;

    if( in && m_lastIn == 0 )             // positive edge, reset counter
    {
      m_cnt = 0;
    }
    else if( in && m_lastIn )             // positive level count ISR ticks
    {
      m_cnt++;
    }
    else if( in == 0 && m_lastIn )        // negativ edge --> result
    {
      m_lastVal = m_inpVal;   
      m_inpVal  = m_cnt;                  // value is number of ticks
    }
    m_lastIn = in;
  }
}


// Channel
//////////
void UniPWMChannel::Init( int pin, UNIPWMPHASE_PTR * ppPhases, int phaseCount, enPwmType pwmType /* = SOFTPWM */) 
{
  cli();

  m_pwmType = pwmType;

  // initialize port and pinmask
  if( m_pin != pin )
  {
	  m_pin     = pin;
    m_pPort   = portOutputRegister(digitalPinToPort(pin));
    m_pinMask = digitalPinToBitMask(pin);

    pinMode(pin, OUTPUT);
  }

  if( m_pwmType & INVERTED_BIT )
    *m_pPort |= m_pinMask;  // set pin to high
  else
   	*m_pPort &= ~m_pinMask; // set pin state to low

  // analog servo on pin 9
  if( pin == 9 && (pwmType & FASTPWM16_BIT) )
    InitTimer1Pin9();

  // check, if servo already is in final position to avoid jerking due to sequence change.
  if( m_pwmType & SERVO_BIT )
  {
    int newFinalPwmVal = CalcFinalPwmVal( ppPhases, phaseCount );
    if( newFinalPwmVal && newFinalPwmVal == m_finalPwmVal )
    {
      // ignore new aperiodic sequence, if already in final position
      m_ppPhases   = &ppPhases[ phaseCount - 1 ];
	    m_phaseCount = 1;
      goto EXIT; 
    }
    m_finalPwmVal = newFinalPwmVal;
  }

	// initialize pwm phases
	m_ppPhases   = ppPhases;
	m_phaseCount = phaseCount;
	for( int i = 0; i < phaseCount; i++ )
		ppPhases[ i ]->construct(); 

EXIT:
  sei();
}


void UniPWMChannel::SetFree()
{
  m_pin = 0;
  m_phaseCount = 0;
}


// retrieve final pwm value of an aperiodic sequence
////////////////////////////////////////////////////
uint8_t UniPWMSequence::CalcFinalPwmVal(  UNIPWMPHASE_PTR * ppPhases, int phaseCount )
{
  if( ppPhases  )
  {
	  for( int i = 0; i < phaseCount; i++ )
    {
		  if( ppPhases[ i ]->m_nCycles == 0 ) // search for constant value with infinite duration
        return ppPhases[ i ]->getPwmVal(0, &m_workData );
    }
  }
  return 0;
}


// UniPWMRamp
/////////////
void UniPWMRamp::construct()
{
   m_inc16 = ((m_endPwmVal - m_startPwmVal) * 256L ) / m_nCycles;
}


uint8_t UniPWMRamp::getPwmVal( uint16_t cycle, UniPWMWorkData * pWorkData )
{
  if( cycle == 0  )
    pWorkData->m_curPwmVal16 = m_startPwmVal * 256L;
  else
    pWorkData->m_curPwmVal16 += m_inc16; 

  return pWorkData->m_curPwmVal16 / 256L;
}

