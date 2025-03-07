////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_LOGIC_GATES_h
#define _MITOV_LOGIC_GATES_h

#include <Mitov.h>

namespace Mitov
{
	class BooleanOr : public CommonMultiInput<bool>
	{
		typedef CommonMultiInput<bool> inherited;

	protected:
		virtual bool CalculateOutput()
		{
			bool AValue = false;
			for (SimpleList<OpenWire::VlaueSinkPin<bool> >::iterator Iter = InputPins.begin(); Iter != InputPins.end(); ++Iter)
				AValue |= Iter->Value;

			return AValue;
		}

	};
//---------------------------------------------------------------------------
	class BooleanAnd : public CommonMultiInput<bool>
	{
		typedef CommonMultiInput<bool> inherited;

	protected:
		virtual bool CalculateOutput()
		{
			bool AValue = (InputPins.size() > 0);
			for (SimpleList<OpenWire::VlaueSinkPin<bool> >::iterator Iter = InputPins.begin(); Iter != InputPins.end(); ++Iter)
				AValue &= Iter->Value;

			return AValue;
		}

	};
//---------------------------------------------------------------------------
	class BooleanXor : public CommonMultiInput<bool>
	{
		typedef CommonMultiInput<bool> inherited;

	protected:
		virtual bool CalculateOutput()
		{
			bool AValue = false;
			for (SimpleList<OpenWire::VlaueSinkPin<bool> >::iterator Iter = InputPins.begin(); Iter != InputPins.end(); ++Iter)
				AValue ^= Iter->Value;

			return AValue;
		}

	};
//---------------------------------------------------------------------------
	class BooleanInverter : public CommonTypedFilter<bool>
	{
	protected:
		virtual bool FilterValue(bool AValue)
		{
			return ! AValue;
		}
	};
//---------------------------------------------------------------------------

}

#endif
