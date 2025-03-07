////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_PULSE_WIDE_MODULATION_h
#define _MITOV_PULSE_WIDE_MODULATION_h

#include <Mitov.h>

namespace Mitov
{
	class PulseWideModulation : public Mitov::CommonFilter
	{
		typedef Mitov::CommonFilter inherited;

	protected:
		float FValue;

	public:
		float Period;

	protected:
		virtual void DoReceive( OpenWire::Data &_Data )
		{
			FValue = ((OpenWire::ValueData<float>*)&_Data)->Value;
/*
			if(((OpenWire::ValueData<bool>*)&_Data)->Value )
				digitalWrite( PinNumber, HIGH );

			else
				digitalWrite( PinNumber, LOW );
*/
		}

	public:
		PulseWideModulation( float _period = 20 ) :
			FValue( 0 ),
			Period( _period )
		{
		}

	};
}

#endif
