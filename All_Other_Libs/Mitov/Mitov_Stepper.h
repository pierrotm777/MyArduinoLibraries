////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_STEPPER_h
#define _MITOV_STEPPER_h

#include <Mitov.h>

namespace Mitov
{

/*
	enum StepperMotorType =
	{
		smtDirectional,
		smtFull2Wire,
		smtFull3Wire,
		smtFull4Wire,
		smtHalf3Wire,
		smtHalf4Wire
	};
*/
	const unsigned char C_StepperBits4[] =
	{
			0b1100,
			0b0110,
			0b0011,
			0b1001
	};

	const unsigned char C_StepperBits8[] =
	{
			0b1000,
			0b1100,
			0b0100,
			0b0110,
			0b0010,
			0b0011,
			0b0001,
			0b1001
	};
//---------------------------------------------------------------------------
	class BasicStepperMotor : public OpenWire::Component
	{
	public:
		Mitov::SimpleList<OpenWire::SourcePin>	OutputPins;
		OpenWire::SinkPin	StepInputPin;

	protected:
		unsigned char	FStep;
		unsigned long	FLastTime;


	public:
		bool	Enabled;
		bool	Reversed;
		float	StepsPerSecond;

	protected:
		virtual void UpdatePins() = 0;

		void DoStepReceive( void *_Data )
		{
			Step();
		}

		void SetPinsValue( unsigned char AValue )
		{
			for( int i = 0; i < OutputPins.size(); ++i )
			{
				bool APinValue = ( AValue & 1 );
				OutputPins[ i ].Notify( &APinValue );
				AValue >>= 1;
			}
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( !Enabled )
				FLastTime = currentMicros;

			else
			{
				unsigned long APeriod = ( 1000000 / abs( StepsPerSecond )) + 0.5;
				if( ( currentMicros - FLastTime ) >= APeriod )
				{
					FLastTime += APeriod;
					if(( StepsPerSecond > 0 ) ^ Reversed )
						++FStep;

					else
						--FStep;

					UpdatePins();
				}
			}
		}

		public:
			BasicStepperMotor() : 
				FStep( 0 ), 
				FLastTime( 0 ),
				Enabled( true ),
				Reversed( true ),
				StepsPerSecond( 1000 )
			{
				StepInputPin.OnReceiveObject = this;
				StepInputPin.OnReceive = (OpenWire::TOnPinReceive)&BasicStepperMotor::DoStepReceive;
			}

	public:
		void Step()
		{
			if( Reversed ) //^ ( StepsPerSecond > 0 ) )
				--FStep;

			else
				++FStep;

			UpdatePins();
		}
	};
//---------------------------------------------------------------------------
	class StepperMotorDirectional : public BasicStepperMotor
	{
	public:
		StepperMotorDirectional()
		{
			OutputPins.SetCount(2);
		}
	};
//---------------------------------------------------------------------------
	class BasicStepperMotorHalfStep : public BasicStepperMotor
	{
	public:
		bool	HalfStep;

	};
//---------------------------------------------------------------------------
	class StepperMotor2Wire : public BasicStepperMotorHalfStep
	{
	public:
		StepperMotor2Wire()
		{
			OutputPins.SetCount(2);
		}
	};
//---------------------------------------------------------------------------
	class StepperMotor4Wire : public BasicStepperMotorHalfStep
	{
	protected:
		virtual void UpdatePins()
		{
			unsigned char AValue = HalfStep ? C_StepperBits8[ FStep & 0b0111 ] : C_StepperBits4[ FStep & 0b0011 ];
			SetPinsValue( AValue );
		}

	public:
		StepperMotor4Wire()
		{
			OutputPins.SetCount(4);
		}
	};
//---------------------------------------------------------------------------
}

#endif
