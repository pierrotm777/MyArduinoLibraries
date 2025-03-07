////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_ACCELEROMETER_h
#define _MITOV_ACCELEROMETER_h

#include <Mitov.h>

namespace Mitov
{
	class Accelerometer : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

	public:
		OpenWire::SinkPin	XInputPin;
		OpenWire::SinkPin	YInputPin;

		OpenWire::SourcePin	XOutputPin;
		OpenWire::SourcePin	YOutputPin;

	public:
		bool	Enabled;

	protected:
		class DataSet : public Object
		{
		protected:
			OpenWire::SourcePin	*FOutputPin;

			unsigned long	FStartTime;
			bool			FOldValue;
			const Accelerometer	*FOwner;

		public:
			virtual void DoReceive( void *_Data )
			{
				if( ! FOwner->Enabled )
					return;

				bool AValue = *(bool *)_Data;
				if( FOldValue == AValue )
					return;

				unsigned long ANow = micros();
				FOldValue = AValue;
				if( AValue )
				{
					FStartTime = ANow;
					return;
				}

				long APeriod = ANow - FStartTime;

				if( APeriod == 0 )
					APeriod = 1;

				float AAcceleration = (( APeriod / 10 ) - 500) * 8;

				FOutputPin->Notify( &AAcceleration );
			}

		public:
			DataSet( const Accelerometer *AOwner, OpenWire::SinkPin *AInputPin, OpenWire::SourcePin *AOutputPin ) :
				FOwner( AOwner ),
				FOutputPin( AOutputPin ),
				FOldValue( false ),
				FStartTime( 0 )
			{
				AInputPin->OnReceiveObject = this;
				AInputPin->OnReceive = (OpenWire::TOnPinReceive)&DataSet::DoReceive;
			}
		};


	protected:
		DataSet	FXDataSet;
		DataSet	FYDataSet;

	protected:
		virtual void DoReceiveX( void *_Data )
		{
		}

	public:
		Accelerometer() :
			Enabled( true ),
			FXDataSet( this, &XInputPin, &XOutputPin ),
			FYDataSet( this, &YInputPin, &YOutputPin )
		{
		}
	};
}

#endif
