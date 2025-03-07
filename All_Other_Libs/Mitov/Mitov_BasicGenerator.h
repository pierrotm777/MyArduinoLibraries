////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_BASIC_GENERATOR_h
#define _MITOV_BASIC_GENERATOR_h

#include <Mitov.h>

namespace Mitov
{
	template<typename T> class BasicGenerator : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource inherited;

	public:
		OpenWire::SinkPin	ClockInputPin;

	public:
        bool    Enabled;

	public: // Needs to be public due to compiler bug :-(
		T	FValue;

	protected:
		virtual void DoClockReceive( void *_Data ) = 0;

		inline void SedOutput() // Needed to be due to compiler bug :-(
		{
			 OutputPin.Notify( &FValue );
		}

	public:
		BasicGenerator() :
			FValue( 0 ),
            Enabled( true )
		{
			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&BasicGenerator::DoClockReceive;
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class BasicFrequencyGenerator : public Mitov::BasicGenerator<T>
	{
		typedef Mitov::BasicGenerator<T> inherited;

	public:
		float	Frequency;
		T	Amplitude;
		T	Offset;

		// 0 - 1
		float	Phase;

	protected:
		float	FPhase;
		unsigned long FLastTime;
		
	protected:
		virtual void SystemStart()
		{
			FPhase = Phase;

			inherited::SystemStart();
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( ! inherited::ClockInputPin.IsConnected() )
				Generate( currentMicros );

			inherited::SystemLoopBegin( currentMicros );
		}

		void Generate( unsigned long currentMicros )
		{
			if( inherited::Enabled )
			{
				float APeriod = 1000000 / Frequency;

				float ATime = ( currentMicros - FLastTime );
				ATime /= APeriod;
				FPhase += ATime;
				FPhase = fmod( FPhase, 1 );

				CalculateValue();
			}

			FLastTime = currentMicros;
			inherited::SedOutput();
		}

	protected:
		virtual void CalculateValue() = 0;

		virtual void DoClockReceive( void *_Data )
		{
			Generate( micros() );
		}

	public:
		BasicFrequencyGenerator( T AAmplitude, T AOffset ) :
			FPhase( 0 ),
			FLastTime( 0 ),
			Phase( 0 ),
			Frequency( 1 ),
			Amplitude( AAmplitude ),
			Offset( AOffset )
		{
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class AsymmetricGenerator : public Mitov::BasicFrequencyGenerator<T>
	{
		typedef Mitov::BasicFrequencyGenerator<T> inherited;

	public:
		// -1 - 1
		float	Asymmetry;

	public:
		AsymmetricGenerator( T AAmplitude, T AOffset ) :
			inherited( AAmplitude, AOffset ),
			Asymmetry( 0 )
		{
		}

	};
}

#endif
