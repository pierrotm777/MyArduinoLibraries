////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_MAXIM_LED_CONTROL_h
#define _MITOV_MAXIM_LED_CONTROL_h

#include <Mitov.h>
//#include <LedControl.h>

namespace Mitov
{
	class PrimaryMaximLedController : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

/*
	public:
		OpenWire::SourcePin	DataOutputPin;
		OpenWire::SourcePin	ClockOutputPin;
		OpenWire::SourcePin	SelectOutputPin;
*/
	protected:
		LedControl	FLedControl;

	public:
		PrimaryMaximLedController( int ADataPin, int AClockPin, int ASelectPin, int ANumDevices ) :
			FLedControl( ADataPin, AClockPin, ASelectPin, ANumDevices )
		{
		}
	};
}

#endif
