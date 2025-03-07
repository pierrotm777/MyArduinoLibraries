////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _OPENWIRE_h
#define _OPENWIRE_h

#include <Mitov_SimpleList.h>

namespace OpenWire
{
	class Object
	{
	};
//---------------------------------------------------------------------------
	class VirtualObject : public Object
	{
	public:
		virtual ~VirtualObject() {}
	};
//---------------------------------------------------------------------------
	class Component : public VirtualObject
	{
	public:
		static void _SystemInit();
		static void _SystemLoop();

	protected:
		virtual void CalculateFields() {}
		virtual void SystemInit()
		{
			CalculateFields();
		}

		virtual void SystemStart() {}
		virtual void SystemLoopBegin( unsigned long currentMicros ) {}
		virtual void SystemLoopEnd() {}

	public:
		Component();
		virtual ~Component();

	};
//---------------------------------------------------------------------------
	typedef void (Object::*TOnPinReceive) ( void *_Data );
//---------------------------------------------------------------------------
	class Pin : public Object
	{
	public:
		TOnPinReceive OnReceive;
		Object       *OnReceiveObject;

	public:
		virtual void Connect( Pin &_other ) = 0;
		virtual bool IsConnected() = 0;

	public:
		virtual void Receive( void *_Data )
		{
			if( OnReceive )
				( OnReceiveObject->*OnReceive )( _Data );

		}

	public:
		virtual void Notify( void *_data ) = 0;

	public:
		Pin() : OnReceive( NULL ), OnReceiveObject( NULL ) {}

	};
//---------------------------------------------------------------------------
	class SourcePin : public Pin
	{
	public:
		Mitov::SimpleList<OpenWire::Pin*> Sinks;

	public:
		virtual void Connect( Pin &_other )
		{
			Sinks.push_back( &_other );
			_other.Connect( *this );
		}

		virtual bool IsConnected()
		{
			return ( Sinks.size() > 0 );
		}

	public:
		virtual void Notify( void *_data )
		{
			for( Mitov::SimpleList<OpenWire::Pin*>::iterator Iter = Sinks.begin(); Iter != Sinks.end(); ++Iter )
				( *Iter )->Receive( _data );

		}

	};
//---------------------------------------------------------------------------
	class SinkPin : public Pin
	{
	public:
		OpenWire::Pin *Source;

	public:
		virtual void Connect( Pin &_other )
		{
			Source = &_other;
		}

		virtual bool IsConnected()
		{
			return ( Source );
		}

	public:
		virtual void Notify( void *_data )
		{
			if( Source )
				Source->Receive( _data );

		}

	public:
		SinkPin() :
		  Source( NULL )
		{
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class VlaueSinkPin : public SinkPin
	{
		typedef SinkPin inherited;

	public:
		T Value;

	public:
		virtual void Receive( void *_Data )
		{
			Value = *(T*)_Data;
			inherited::Receive( _Data );
		}

	public:
		VlaueSinkPin() :
			Value( 0 )
		{
		}
	};
//---------------------------------------------------------------------------
	template<> class VlaueSinkPin<char *> : public SinkPin
	{
		typedef SinkPin inherited;

	public:
		String Value;

	public:
		virtual void Receive( void *_Data )
		{
			Value = (char*)_Data;
			inherited::Receive( _Data );
		}

	public:
		VlaueSinkPin() :
			Value( "" )
		{
		}
	};
//---------------------------------------------------------------------------
	template<typename T> class LiveBindingSink : public OpenWire::Object
	{
	protected:
		void (*FFunc)( T AData );

	public:
		OpenWire::SinkPin	InputPin;

	protected:
		virtual void DoReceive( void *_Data )
		{
			FFunc(*(T*)_Data );
		}

	public:
		LiveBindingSink( void (*AFunc)( T AData ) ) :
			FFunc( AFunc )
		{
			InputPin.OnReceiveObject = this;
			InputPin.OnReceive = (OpenWire::TOnPinReceive)&LiveBindingSink::DoReceive;
		}
	};
//---------------------------------------------------------------------------
	static Mitov::SimpleList<OpenWire::Component*> _Components;
//---------------------------------------------------------------------------
	Component::Component()
	{
		_Components.push_back(this);
	}
//---------------------------------------------------------------------------
	Component::~Component()
	{
		for( Mitov::SimpleList<OpenWire::Component *>::iterator Iter = OpenWire::_Components.begin(); Iter != OpenWire::_Components.end(); ++Iter )
			if( *Iter == this )
			{
				_Components.erase(Iter);
				break;
			}

	}
//---------------------------------------------------------------------------
	void Component::_SystemInit()
	{
		for( Mitov::SimpleList<OpenWire::Component *>::iterator Iter = OpenWire::_Components.begin(); Iter != OpenWire::_Components.end(); ++Iter )
			( *Iter )->SystemInit();

		for( Mitov::SimpleList<OpenWire::Component *>::iterator Iter = OpenWire::_Components.begin(); Iter != OpenWire::_Components.end(); ++Iter )
			( *Iter )->SystemStart();

	}
//---------------------------------------------------------------------------
	void Component::_SystemLoop()
	{
		unsigned long currentMicros = micros();
		for( Mitov::SimpleList<OpenWire::Component *>::iterator Iter = OpenWire::_Components.begin(); Iter != OpenWire::_Components.end(); ++Iter )
			( *Iter )->SystemLoopBegin( currentMicros );

		for( Mitov::SimpleList<OpenWire::Component *>::iterator Iter = OpenWire::_Components.begin(); Iter != OpenWire::_Components.end(); ++Iter )
			( *Iter )->SystemLoopEnd();

	}
//---------------------------------------------------------------------------
}

#endif
