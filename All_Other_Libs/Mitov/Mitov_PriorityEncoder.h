////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_PRIORITY_ENCODER_h
#define _MITOV_PRIORITY_ENCODER_h

#include <Mitov.h>

namespace Mitov
{
	class PriorityEncoder : public BasicCommonMultiInput<bool, unsigned long>
	{
		typedef BasicCommonMultiInput<bool, unsigned int> inherited;

	protected:
		virtual unsigned long CalculateOutput()
		{
			for( unsigned long i = inherited::InputPins.size(); i--; )
				if( inherited::InputPins[ i ].Value )
					return i;

			return 0;
		}

	};
//---------------------------------------------------------------------------
}

#endif
