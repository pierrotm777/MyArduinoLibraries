////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_PID_CONTROLLER_h
#define _MITOV_PID_CONTROLLER_h

#include <Mitov.h>

namespace Mitov
{
	class PIDController : public Mitov::CommonEnableFilter
	{
		typedef Mitov::CommonEnableFilter inherited;

	public:
		OpenWire::SinkPin	ManualControlInputPin;
		OpenWire::SinkPin	ClockInputPin;

	public:
		float ProportionalGain;
		float IntegralGain;
		float DerivativeGain;

		float SetPoint;

		float InitialValue;

	protected:
		unsigned long	FLastTime;
		float	FOutput;
		double	FInput;
		double	FLastInput;
		double	FITerm;

	public:
		void SetEnabled( bool AValue )
		{
            if( Enabled == AValue )
                return;

            Enabled = AValue;
			if( Enabled )
				Initialize();

		}

	protected:
		virtual void DoReceive( void *_Data )
		{
			FInput = *(float *)_Data;
		}

		void DoManualControlReceive( void *_Data )
		{
			if( Enabled )
				return;

			FOutput = *(float *)_Data;
		}

		void DoClockReceive( void *_Data )
		{
			OutputPin.Notify( &FOutput );
		}

	protected:
		void Initialize()
		{
			FITerm = FOutput;
			FLastInput = FInput;
			FLastTime = micros();
		}

	protected:
		virtual void SystemStart()
		{
			FInput = InitialValue;
			Initialize();
			inherited::SystemStart();
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			inherited::SystemLoopBegin( currentMicros );
			if( ! Enabled ) 
				return;

			unsigned long timeChange = ( currentMicros - FLastTime );
			float ANormalizedTime = timeChange / 1000000;

			// Compute all the working error variables
			double error = SetPoint - FInput;
//			ITerm += ( ki * error ) * ANormalizedTime;
			FITerm = constrain( FITerm + ( IntegralGain * error ) * ANormalizedTime, 0, 1 );

			double dInput = ( FInput - FLastInput ) * ANormalizedTime;
 
			// Compute PID Output
			float AOutput = constrain( ProportionalGain * error + FITerm - DerivativeGain * dInput, 0, 1 );
	  
			// Remember some variables for next time
			FLastInput = FInput;
			FLastTime = currentMicros;

			if( AOutput == FOutput )
				return;

			if( ClockInputPin.IsConnected() )
				return;

			OutputPin.Notify( &FOutput );
		}

	public:
		PIDController() :
			ProportionalGain( 0.1 ),
			IntegralGain( 0.1 ),
			DerivativeGain( 0.1 ),
			SetPoint( 0 )
		{
			ManualControlInputPin.OnReceiveObject = this;
			ManualControlInputPin.OnReceive = (OpenWire::TOnPinReceive)&PIDController::DoManualControlReceive;

			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&PIDController::DoClockReceive;
		}

	};
//---------------------------------------------------------------------------
}

#endif
