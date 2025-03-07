////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_ROTARY_ENCODER_SENSOR_h
#define _MITOV_ROTARY_ENCODER_SENSOR_h

#include <Mitov.h>

namespace Mitov
{
	class RotaryEncoderSensor : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

	public:
		OpenWire::SinkPin	AInputPin;
		OpenWire::SinkPin	BInputPin;

		OpenWire::SourcePin	UpOutputPin;
		OpenWire::SourcePin	DownOutputPin;

	protected:
		bool	FAValue;
		bool	FBValue;
		bool    FAPopulated;
		bool    FBPopulated;

	protected:
		void DoReceiveA( void *_Data )
		{
			bool AValue = *(bool*)_Data;
			FAPopulated = true;
			if( FAValue == AValue )
				return;

			FAValue = AValue;
			if( AValue && FBPopulated )
			{
				if( FBValue )
					UpOutputPin.Notify( NULL );

				else
					DownOutputPin.Notify( NULL );
			}
		}

		void DoReceiveB( void *_Data )
		{
			bool AValue = *(bool*)_Data;
			FBPopulated = true;
			if( FBValue == AValue )
				return;

			FBValue = AValue;
		}

	public:
		RotaryEncoderSensor() :
			FAPopulated( false ),
			FBPopulated( false ),
			FAValue( false ),
			FBValue( false )
		{
			AInputPin.OnReceiveObject = this;
			AInputPin.OnReceive = (OpenWire::TOnPinReceive)&RotaryEncoderSensor::DoReceiveA;

			BInputPin.OnReceiveObject = this;
			BInputPin.OnReceive = (OpenWire::TOnPinReceive)&RotaryEncoderSensor::DoReceiveB;
		}

	};
//---------------------------------------------------------------------------
}

#endif
