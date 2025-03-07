////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_TIMING_h
#define _MITOV_TIMING_h

#include <Mitov.h>

namespace Mitov
{
	class ClockGenerator : public Mitov::CommonGenerator
	{
	protected:
		virtual void Clock()
		{
			OutputPin.Notify( NULL );
		}

	};
//---------------------------------------------------------------------------
	class Timer : public Mitov::CommonEnableSource
	{
	public:
		OpenWire::SinkPin	StartInputPin;

	public:
		unsigned long	Interval;
		bool			AutoRepeat;
		bool			CanRestart;

	protected:
		unsigned long	FStartTime;
		bool			FValue;

	protected:
		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( ! AutoRepeat )
				if( ! FValue )
					return;

			if( FValue )
				if( ! Enabled )
				{
					FValue = false;
					OutputPin.Notify( &FValue );
					return;
				}

			currentMicros = micros(); // Use micros(); to make sure the DoReceiveStart is not from the same clock event!
			if( currentMicros - FStartTime < Interval )
				return;

			FValue = !FValue;
			OutputPin.Notify( &FValue );
			if( AutoRepeat )
				FStartTime += Interval;

		}

	protected:
		virtual void DoReceiveStart( void *_Data )
		{
			if( ! Enabled )
				return;

			if( !CanRestart )
				if( FValue )
					return;

			FStartTime = micros();
			if( FValue )
				return;

			FValue = true;
			OutputPin.Notify( &FValue );
		}

	public:
		Timer() :
			Interval( 1000000 ),
			AutoRepeat( false ),
			CanRestart( true )
		{
			StartInputPin.OnReceiveObject = this;
			StartInputPin.OnReceive = (OpenWire::TOnPinReceive)&Timer::DoReceiveStart;
		}
	};
//---------------------------------------------------------------------------
	class DetectEdge : public Mitov::CommonEnableFilter
	{
	protected:
		bool	FLastValue;

	public:
		bool	Rising;

	protected:
		virtual void DoReceive( void *_Data )
		{
			bool AValue = *(bool *)_Data;
			if( AValue == FLastValue )
				return;

			FLastValue = AValue;
			if( Rising == AValue )
				OutputPin.Notify( NULL );

		}

	public:
		DetectEdge() : 
			FLastValue( false ),
			Rising( true )
		{
		}

	};

//---------------------------------------------------------------------------
}

#endif
