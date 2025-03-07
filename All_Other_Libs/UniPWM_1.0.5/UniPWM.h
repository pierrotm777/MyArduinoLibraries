
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

#ifndef UNIPWM_H
#define UNIPWM_H

// make C++ classes accessible for ISR
extern "C" void TIMER2_COMPA_vect(void) __attribute__ ((signal));

// #include <avr/pgmspace.h> // http://arduino.cc/de/Reference/PROGMEM

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

// see http://blog.oscarliang.net/arduino-timer-and-interrupt-tutorial/
//     http://www.instructables.com/id/Arduino-Timer-Interrupts/step2/Structuring-Timer-Interrupts/
// explanation on hardware resources:
// PWM-Pins: 3, 5, 6, 9, 10, and 11
// Timer0: Output (PWM) on Pin 5 and 6, used for delay() and millis()
// Timer1: Output (PWM) on Pin 9 and 10, used by servo library
// Timer2: Output (PWM) on Pin 3 and 11, used by tone() function
// THIS LIBRARY USES Timer2 - DO NOT USE PINS 3 and 11 FOR HARDWARE PWM (=analogWrite) when SOFTPWM is enabled 

#ifndef F_CPU
#define F_CPU 16000000L // for nano
#endif

#define UNIPWM_COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x]))))) // number of elements in an array

#define UNIPWM_TIMER_INIT( ocrval ) ({\
  TIFR2  = (1 << TOV2);    /* clear interrupt */ \
  TCCR2B = (1 << CS21);    /* set prescaler 8 */ \
  TCCR2A = (1 << WGM21);   /* CTC mode */ \
  OCR2A  = (ocrval);       /* set frequency to cpuclock/prescaler/timerrange/ocrval */ \
  TIMSK2 = (1 << OCIE2A);  /* enable timer compare interrupt */ \
})


class UniPWMWorkData // phase specific workdata (used by ISR via getPwmValue())
{
public: 
  UniPWMWorkData() : m_curPwmVal16( 0 ) {}
  int16_t m_curPwmVal16; // 16 bit pwm value for UniPWMRamp
};


class UniPWMPhase 
{
public:
	UniPWMPhase( uint16_t nCycles ) : m_nCycles(nCycles) {}
	virtual uint8_t  getPwmVal( uint16_t cycle, UniPWMWorkData * pWorkData ) = 0; // calculate pwmvalue for a specific time/phase value
  virtual void     construct() {}                   // do calculations for pwmvals (i.e. ramp) --> constructor code is not executed for static classes
	const uint16_t   m_nCycles;
};
typedef UniPWMPhase * UNIPWMPHASE_PTR; 


class UniPWMConst : public UniPWMPhase 
{
public:
	UniPWMConst( uint8_t pwmVal = 0, uint16_t nCycles = 0 ) : m_pwmVal( pwmVal ), UniPWMPhase( nCycles ) {}
	virtual uint8_t getPwmVal( uint16_t cycle, UniPWMWorkData * pWorkData ) { return m_pwmVal; }  // pwm val is always constant
protected:
		const uint8_t m_pwmVal;
};


class UniPWMRamp : public UniPWMPhase 
{
public:
	UniPWMRamp( uint8_t startPwmVal, uint8_t endPwmVal, uint16_t nCycles ) : m_startPwmVal( startPwmVal ), m_endPwmVal( endPwmVal ), m_inc16( 0 ), UniPWMPhase( nCycles ) {}
	virtual uint8_t getPwmVal( uint16_t cycle, UniPWMWorkData * pWorkData );
	virtual void    construct(); // calculate initial ramp values
protected:
		const uint8_t  m_startPwmVal;
		const uint8_t  m_endPwmVal;
		int16_t        m_inc16;       // 16 bit increment for UniPWMWorkData::m_curPwmVal16
};


class UniPWMSequence
{
public:
	UniPWMSequence() : m_ppPhases( 0 ), m_idxCurPhase( 0 ), m_maxCycles( 0 ), m_curCycle( 0 ), m_phaseCount( 0 ), m_finalPwmVal( 0 ), m_pCurPhase( 0 ) {}

protected:
	UNIPWMPHASE_PTR * m_ppPhases; // array with phases
	int               m_phaseCount;
  
  uint8_t           CalcFinalPwmVal( UNIPWMPHASE_PTR * ppPhases, int phaseCount );
  uint8_t           m_finalPwmVal; // final pwm value of aperiodic sequences

	// working set for UniPWMPhases (used by ISR)
	int               m_idxCurPhase;  // curent phase is m_ppOrder[ idxCurPhase]
	UNIPWMPHASE_PTR   m_pCurPhase;    
	uint16_t          m_maxCycles;    // number of cycles for current phase --> m_ppOrder[ idxCurPhase]
	uint16_t          m_curCycle;     // current pwm clock cycle during this phase

  UniPWMWorkData    m_workData;     // used by UniPWMRamp only
};


class UniPWMFastPWM // hardware PWM mode of ATMega 
{
protected:
  inline void InitTimer1Pin9()
  {
    // 16 bit PWM with timer 1, set Mode 14 on Pin 9
    TCCR1A = (1 << COM1A1) | (1 << WGM11);                 // Fast PWM on OC1A
    TCCR1B = (1 << WGM13)  | (1 << WGM12) | (1 << CS11);   // Fast PWM with TOP = ICR1, prescaler = 8
    ICR1  = 40000;     // 50 Hz
    OCR1A = 0;
  }
  inline void SetFastPWMValue( int value )
  {
    OCR1A = value;
  }
};


class UniPWMChannel : public UniPWMSequence, protected UniPWMFastPWM 
{
	friend class UniPWM;
  friend void TIMER2_COMPA_vect(void);

protected:
  enum
  {
    SOFTPWM_BIT   = 1,  // for internal use only
    HARDPWM_BIT   = 2,
    INVERTED_BIT  = 4,
    FASTPWM16_BIT = 8,
    SERVO_BIT     = 16,
  };
public:
  enum enPwmType
  { 
    SOFTPWM          = SOFTPWM_BIT,                               // software controlled PWM, 8 bit resolution, usable on all digital pins
    HARDPWM          = HARDPWM_BIT,                               // hardware controlled PWM, 8 bit resolution, on Pins 3, 5, 6, 9, 10, and 11 only
    SOFTPWM_INVERTED = SOFTPWM_BIT | INVERTED_BIT,                // software controlled PWM inverted output
    DIGITAL_SERVO    = HARDPWM_BIT | SERVO_BIT,                   // position control, hardware controlled PWM, 8 bit resolution 100Hz, on Pins 3, 5, 6, 9, 10, and 11 only
    ANALOG_SERVO     = HARDPWM_BIT | FASTPWM16_BIT | SERVO_BIT,   // position control, hardware controlled PWM, 16 bit resolution 50Hz, on Pin 9 only
    SOFTPWM_SERVO    = SOFTPWM_BIT | SERVO_BIT,                   // position control, software controlled PWM 
  }; 

  UniPWMChannel() : m_pin( 0 ), m_pPort( 0 ), m_pinMask( 0 ), m_pwmType( SOFTPWM ), m_channelPwmVal( 0 )  {}
	
protected:
	inline bool IsFree() { return m_pin == 0; }
	inline bool IsPin( int pin ) { return m_pin == pin; }
	inline void SetFree();

	void Init( int pin, UNIPWMPHASE_PTR * ppPhases, int phaseCount, enPwmType pwmType = SOFTPWM ); 
	void Deinit() {}

protected:
	         int        m_pin; // 1..n, 0 =unused
  volatile uint8_t  * m_pPort;
				   uint8_t    m_pinMask;
				   enPwmType  m_pwmType;

           // working set (used by ISR)
           uint8_t    m_channelPwmVal; // this is the current pwm value used for this channel (set and used by ISR only)
};


class UniPWMInpChannel
{
	friend class UniPWM;
  friend void TIMER2_COMPA_vect(void);

public:
    enum enInpType
    { 
      INP_NORMAL   = 0,  // normal input, i.e. for direct receiver input
      INP_INVERTED = 1,  // inverted input, i.e. for use on opto coupler
      INP_DISABLED = 2,  // disable input for this pin
    }; 

public:
  UniPWMInpChannel() : m_pin( 0 ), m_inpVal( 0 ), m_pPort( 0 ), m_pinMask( 0 ), m_cnt( 0 ), m_lastIn( 0 ), m_lastVal( 0 ) {}
  void Init( int pin, enInpType inpType = INP_NORMAL );
  inline uint16_t GetValue()
  {
    cli();
    int val     = m_inpVal;
    int lastVal = m_lastVal;
    sei();
    if( abs( val - lastVal ) > 3 ) // simple filter
      val = 0;
    return (uint16_t)val;
  }

protected:
	inline bool IsFree() { return m_pin == 0; }
	inline bool IsPin( int pin ) { return m_pin == pin; }
  inline void SetFree() { m_pin = 0; }

  int       m_pin;
  enInpType m_inpType;
  uint16_t  m_inpVal;
  volatile  uint8_t  * m_pPort;
				    uint8_t    m_pinMask;

  // working set (used by ISR)
  uint16_t m_cnt;
  uint16_t m_lastIn;
  uint16_t m_lastVal;
  inline void ISR_MeasurePulse();
};


class UniPWM
{
  friend void TIMER2_COMPA_vect(void);

public:
	UniPWM( UniPWMChannel * pChannels, int channelCount, UniPWMInpChannel * pInpChannels = 0, int inpChannelCount = 0 ) : m_pChannels( pChannels ), m_channelCount( channelCount ), m_pInpChannels( pInpChannels ), m_inpChannelCount( inpChannelCount ) {}
	~UniPWM() {}

	void SetOutChannel( int pin, UNIPWMPHASE_PTR * ppPhases, int phaseCount, UniPWMChannel::enPwmType pwmType = UniPWMChannel::SOFTPWM );
  void SetInpChannel( int pin, UniPWMInpChannel::enInpType inpType = UniPWMInpChannel::INP_NORMAL );
	void Start( uint16_t channelMask = 0xFFFF ); // use channel mask with pin numbers to start specific channels only
	void Stop( uint16_t channelMask = 0xFFFF ); // use channel mask with pin numbers to stop specific channels only

  uint16_t GetInpChannelValue( int pin = 0 ); // pin = 0 first pin value

protected:
	UniPWMChannel    * m_pChannels;
	int                m_channelCount;
  
  UniPWMInpChannel * m_pInpChannels;
	int                m_inpChannelCount;
};

#endif // UNIPWM_H

