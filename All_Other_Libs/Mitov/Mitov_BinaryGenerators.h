////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_BINARY_GENERATORS_h
#define _MITOV_BINARY_GENERATORS_h

#include <Mitov.h>

namespace Mitov
{
	class PulseGenerator : public Mitov::CommonGenerator
	{
		typedef Mitov::CommonGenerator inherited;

	public:
		bool	StartValue;

	protected:
		virtual void Clock()
		{
			StartValue = !StartValue;
			OutputPin.Notify( &StartValue );
		}
		
		virtual void SystemInit()
		{
			inherited::SystemInit();

			OutputPin.Notify( &StartValue );
		}

	public:
		PulseGenerator() :
			StartValue( false )
		{
		}

	};
//---------------------------------------------------------------------------
}

#endif
