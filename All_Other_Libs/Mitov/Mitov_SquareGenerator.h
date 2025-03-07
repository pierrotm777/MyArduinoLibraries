////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_SQUARE_GENERATOR_h
#define _MITOV_SQUARE_GENERATOR_h

#include <Mitov.h>
#include "Mitov_BasicGenerator.h"

namespace Mitov
{
	template<typename T> class BasicSquareGenerator : public Mitov::AsymmetricGenerator<T>
	{
		typedef Mitov::AsymmetricGenerator<T> inherited;

	protected:
		virtual void CalculateValue()
		{
			if( inherited::FPhase < 0.5 + inherited::Asymmetry / 2 )
				inherited::FValue = inherited::Offset - inherited::Amplitude;

			else
				inherited::FValue = inherited::Offset + inherited::Amplitude;

		}

	public:
		BasicSquareGenerator( T AAmplitude, T AOffset ) :
			inherited( AAmplitude, AOffset )
		{
		}

	};
//---------------------------------------------------------------------------
	class SquareAnalogGenerator : public Mitov::BasicSquareGenerator<float>
	{
		typedef Mitov::BasicSquareGenerator<float> inherited;

	public:
		SquareAnalogGenerator() :
			inherited( 0.5, 0.5 )
		{
		}

	};
//---------------------------------------------------------------------------
	class SquareIntegerGenerator : public Mitov::BasicSquareGenerator<long>
	{
		typedef Mitov::BasicSquareGenerator<long> inherited;

	public:
		SquareIntegerGenerator() :
			inherited( 1000, 0 )
		{
		}

	};
//---------------------------------------------------------------------------
	class SquareUnsignedGenerator : public Mitov::BasicSquareGenerator<unsigned long>
	{
		typedef Mitov::BasicSquareGenerator<unsigned long> inherited;

	public:
		SquareUnsignedGenerator() :
			inherited( 1000, 1000 )
		{
		}

	};
//---------------------------------------------------------------------------
}

#endif
