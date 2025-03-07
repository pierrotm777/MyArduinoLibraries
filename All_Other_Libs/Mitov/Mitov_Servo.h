////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_SERVO_h
#define _MITOV_SERVO_h

#include <Mitov.h>

#include <ServoEmbed.h>

namespace Mitov
{
	class MitovServo : public Mitov::CommonFilter
	{
		typedef Mitov::CommonFilter inherited;

	protected:
		Servo	FServo;
		bool	FLastValue;

	public:
		void WriteValue( bool AValue )
		{
			if( FLastValue == AValue )
				return;

			OutputPin.Notify( &AValue );
			FLastValue = AValue;
		}

	protected:
		virtual void DoReceive( void *_Data )
		{
			float AValue = *(float*)_Data;

			if( AValue < 0 )
				AValue = 0;

			if( AValue > 1 )
				AValue = 1;

			AValue *= 180;

			FServo.write( AValue );
		}

		virtual void SystemStart() 
		{
			FServo.attach( this );
			inherited::SystemStart();
		}

	public:
		MitovServo() :
			FLastValue( false )
		{
		}

	};

	void MitovServoWrite( void * pin, int value )
	{
		((MitovServo*)pin)->WriteValue( value != LOW );
	}
}

#endif
