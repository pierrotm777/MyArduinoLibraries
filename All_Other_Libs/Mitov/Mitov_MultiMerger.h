////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_MULTI_MERGER_h
#define _MITOV_MULTI_MERGER_h

#include <Mitov.h>

namespace Mitov
{
//---------------------------------------------------------------------------
	class MultiMerger : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource inherited;

	public:
		Mitov::SimpleList<OpenWire::SinkPin> InputPins;

	protected:
		void DoReceive( void *_Data )
		{
			OutputPin.Notify( _Data );
		}

	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();

			for( OpenWire::SinkPin *Iter = InputPins.begin(); Iter != InputPins.end(); ++Iter )
			{
				Iter->OnReceiveObject = this;
				Iter->OnReceive = (OpenWire::TOnPinReceive)&MultiMerger::DoReceive;
			}
		}

	};
}

#endif
