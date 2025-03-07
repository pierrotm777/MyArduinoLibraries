////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_ULTRASONIC_RANGE_h
#define _MITOV_ULTRASONIC_RANGE_h

#include <Mitov.h>

namespace Mitov
{
	enum UltrasonicRangerUnits { rdTime, rdCm, rdInch };

	class UltrasonicRanger : public CommonSource
	{
		typedef Mitov::CommonSource inherited;

		enum State { sStartUp, sStartDown, sListeningEdgeUp, sListeningEdgeDown, sEchoDetected, sPause };

	public:
		OpenWire::SinkPin	EchoInputPin;
		OpenWire::SourcePin	PingOutputPin;
		OpenWire::SourcePin	TimeoutOutputPin;

	public:
		bool	Enabled;
		int		Timeout;
		int		PingTime;
		int		PauseTime;
		float   TimeoutValue;
		UltrasonicRangerUnits	Units;

	protected:
		State			FState;
		unsigned long	FStartTime;
		unsigned long	FEchoStartTime;
		unsigned long	FEndTime;


	protected:
		virtual void DoReceive( void *_Data )
		{
			if( ( FState != sListeningEdgeUp ) && ( FState != sListeningEdgeDown ) )
				return;

			bool AValue = *(bool *)_Data;
			if( FState == sListeningEdgeDown )
			{
			  if( AValue )
				  return;

				FEndTime = micros();
				FState = sEchoDetected;
			}

			else
			{
			  if( ! AValue )
				  return;

			  FState = sListeningEdgeDown;
			  FEchoStartTime = micros();
			}

		}

	protected:
		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( !Enabled )
				return;

			switch ( FState )
			{
				case sPause:
					if( ( currentMicros - FStartTime ) < ((unsigned long)PauseTime ) * 1000 )
						return;		

				case sStartUp:
				{
//					Serial.println( "start" );
					bool AValue = true;
					PingOutputPin.Notify( &AValue );
					FStartTime = currentMicros;
					FState = sStartDown;
					break;
				}

				case sStartDown:
				{
					if( ( currentMicros - FStartTime ) < PingTime )
						return;

					bool AValue = false;
					PingOutputPin.Notify( &AValue );
					FStartTime = currentMicros;
					FState = sListeningEdgeUp;
					break;
				}

				case sListeningEdgeUp:
				case sListeningEdgeDown:
				{
					if( ( currentMicros - FStartTime ) < ((unsigned long)Timeout ) * 1000 )
						return;

					OutputPin.Notify( &TimeoutValue );
					bool ABoolValue = true;
					TimeoutOutputPin.Notify( &ABoolValue );
					FState = sPause;
					break;
				}

				case sEchoDetected:
				{
					unsigned long APeriod = FEndTime - FEchoStartTime;
					float AValue;
					switch( Units )
					{
						case rdTime:
							AValue = APeriod;
							break;

						case rdCm:
							AValue = APeriod / 29 / 2; // / 58.2;
							break;

						case rdInch:
							AValue = APeriod / 74 / 2;
							break;
					}

					OutputPin.Notify( &AValue );
					bool ABoolValue = false;
					TimeoutOutputPin.Notify( &ABoolValue );
					FState = sPause;
					break;
				}
			}
		}

	public:
		UltrasonicRanger() :
			FState( sStartUp ),
			Enabled( true ),
			Timeout( 1000 ),
			PingTime( 2 ),
			PauseTime( 1 ),
			TimeoutValue( -1 ),
			Units( rdCm )
		{
			EchoInputPin.OnReceiveObject = this;
			EchoInputPin.OnReceive = (OpenWire::TOnPinReceive)&UltrasonicRanger::DoReceive;
		}

	};
}

#endif
