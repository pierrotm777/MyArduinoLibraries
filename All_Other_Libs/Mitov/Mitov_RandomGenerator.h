////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_RANDOM_GENERATOR_h
#define _MITOV_RANDOM_GENERATOR_h

#include <Mitov.h>
#include "Mitov_BasicGenerator.h"

namespace Mitov
{
	template<typename T> class CommonRandomGenerator : public Mitov::BasicGenerator<T>
	{
		typedef Mitov::BasicGenerator<T> inherited;

	public:
		T	Min;
		T	Max;
		long	Seed;

	protected:
		virtual void SystemStart()
		{
			randomSeed( Seed );

			inherited::SystemStart();
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( ! inherited::ClockInputPin.IsConnected() )
				Generate();

			inherited::SystemLoopBegin( currentMicros );
		}

	protected:
		virtual void GenerateValue() = 0;

		void Generate()
		{
            if( inherited::Enabled )
            {
				if( Min == Max )
					inherited::FValue = Min;

				else
					GenerateValue();
			}

			inherited::SedOutput();
		}

		virtual void DoClockReceive( void *_Data )
		{
			Generate();
		}

	public:
		CommonRandomGenerator( T AMin, T AMax ) :
			Min( AMin ),
			Max( AMax ),
			Seed( 0 )
		{
		}

	};
//---------------------------------------------------------------------------
	class RandomAnalogGenerator : public Mitov::CommonRandomGenerator<float>
	{
		typedef Mitov::CommonRandomGenerator<float> inherited;

	protected:
		virtual void GenerateValue()
		{
			float AMin = min( Min, Max );
			float AMax = max( Min, Max );
//			double ARandom = random( -2147483648, 2147483647 );
			double ARandom = random( -1147483648, 1147483647 );
//			FValue = ARandom;
			ARandom += 1147483648;
			FValue = AMin + ( ARandom / ( (double)1147483647 + (double)1147483648 )) * (AMax - AMin);
		}

	public:
		RandomAnalogGenerator() :
			inherited( 0, 1 )
		{
		}

	};
//---------------------------------------------------------------------------
	class RandomIntegerGenerator : public Mitov::CommonRandomGenerator<long>
	{
		typedef Mitov::CommonRandomGenerator<long> inherited;

	protected:
		virtual void GenerateValue()
		{
			long AMin = min( Min, Max );
			long AMax = max( Min, Max );
			FValue = random( AMin, AMax + 1 );
		}

	public:
		RandomIntegerGenerator() :
			inherited( -1000, 1000 )
		{
		}

	};
//---------------------------------------------------------------------------
	class RandomUnsignedGenerator : public Mitov::CommonRandomGenerator<unsigned long>
	{
		typedef Mitov::CommonRandomGenerator<unsigned long> inherited;

	protected:
		virtual void GenerateValue()
		{
			unsigned long AMin = min( Min, Max );
			unsigned long AMax = max( Min, Max );
			FValue = random( AMin, AMax + 1 );
		}

	public:
		RandomUnsignedGenerator() :
			inherited( 0, 1000 )
		{
		}

	};

}

#endif
