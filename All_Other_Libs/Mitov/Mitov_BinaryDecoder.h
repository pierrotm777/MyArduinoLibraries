////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_BINARY_DECODER_h
#define _MITOV_BINARY_DECODER_h

#include <Mitov.h>

namespace Mitov
{
	class BinaryDecoder : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink inherited;

	public:
		Mitov::SimpleList<OpenWire::SourcePin> OutputPins;

	public:
		unsigned long	InitialValue;

	protected:
		unsigned long	FOldValue;

	protected:
		void SetValue( unsigned long AValue )
		{
			FOldValue = AValue;
			OutputPins[ FOldValue ].Notify( (void *)&GBooleanConst[ 1 ] );
		}

	protected:
		virtual void DoReceive( void *_Data )
		{
			unsigned long AValue = *(unsigned long *)_Data;
			if( FOldValue == AValue )
				return;

			OutputPins[ FOldValue ].Notify( (void *)&GBooleanConst[ 0 ] );

			SetValue( AValue );
		}

		virtual void SystemInit()
		{
			inherited::SystemInit();
			SetValue( InitialValue );
		}

	public:
		BinaryDecoder() :
			InitialValue( 0 ),
			FOldValue( 0 )
		{
		}

/*
	protected:
		virtual unsigned long CalculateOutput()
		{
			for( unsigned int i = inherited::InputPins.size(); i--; )
				if( inherited::InputPins[ i ].Value )
					return i;

			return 0;
		}
*/
	};
//---------------------------------------------------------------------------
}

#endif
