////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_BASIC_ETHERNET_h
#define _MITOV_BASIC_ETHERNET_h

#include <Mitov.h>

namespace Mitov
{
	class BasicSocket : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

	public:
//		OpenWire::SinkPin	InputPin;
		OpenWire::SourcePin	OutputPin;

	public:
		bool			Enabled;
		unsigned int	Port;

	protected:
		virtual void StartSocket() = 0;

	public:
		void SetEnabled( bool AValue )
		{
            if( Enabled == AValue )
                return;

            Enabled = AValue;
			if( IsEnabled() )
				StartSocket();

			else
				StopSocket();

		}

	public:
		virtual bool IsEnabled()
		{
			return Enabled;
		}

		virtual void BeginPacket()
		{
		}

		virtual void EndPacket()
		{
		}

		virtual void StopSocket() = 0;

	public:
		virtual void SystemStart()
		{
//			Serial.println( Enabled );
			if( Enabled )
				StartSocket();

			inherited::SystemStart();
		}

	public:
		BasicSocket() :
			Enabled( true )
		{
		}

	};
//---------------------------------------------------------------------------
	template<typename T_ROOT, T_ROOT *T_INSTANCE, typename T> class EthernetSocketInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_INSTANCE->IsEnabled() )
			{
				T_INSTANCE->BeginPacket();
				T_INSTANCE->GetPrint()->println( *(T*)_Data );
				T_INSTANCE->EndPacket();
			}
		}
	};
//---------------------------------------------------------------------------
	template<typename T_ROOT, T_ROOT *T_INSTANCE> class EthernetSocketBinaryInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_INSTANCE->IsEnabled() )
			{
				T_INSTANCE->BeginPacket();
				T_INSTANCE->GetPrint()->write( *(unsigned char*)_Data );
				T_INSTANCE->EndPacket();
			}
		}
	};
//---------------------------------------------------------------------------
	template<typename T_ROOT, T_ROOT *T_INSTANCE> class EthernetSocketStringInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_INSTANCE->IsEnabled() )
			{
				T_INSTANCE->BeginPacket();
				T_INSTANCE->GetPrint()->println( (char*)_Data );
				T_INSTANCE->EndPacket();
			}
		}
	};
//---------------------------------------------------------------------------
	template<typename T_ROOT, T_ROOT *T_INSTANCE> class EthernetSocketColorInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_INSTANCE->IsEnabled() )
			{
				T_INSTANCE->BeginPacket();
				Print *APrint = T_INSTANCE->GetPrint();
				APrint->print( "(" );
				APrint->print( (int)((TColor *)_Data)->Red );
				APrint->print( "," );
				APrint->print( (int)((TColor *)_Data)->Green );
				APrint->print( "," );
				APrint->print( (int)((TColor *)_Data)->Blue );
				APrint->println( ")" );
				T_INSTANCE->EndPacket();
			}
		}
	};
//---------------------------------------------------------------------------
}	
#endif
