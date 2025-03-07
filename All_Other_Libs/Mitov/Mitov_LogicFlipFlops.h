////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_LOGIC_FLIP_FLOPS_h
#define _MITOV_LOGIC_FLIP_FLOPS_h

#include <Mitov.h>

namespace Mitov
{
	class BasicFlipFlop : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource inherited;

	protected:
		bool	FValue;

	public:
		OpenWire::SourcePin	InvertedOutputPin;

	protected:
		void SetValue( bool AValue )
		{
			FValue = AValue;

			OutputPin.Notify( &AValue );
			AValue = !AValue;
			InvertedOutputPin.Notify( &AValue );
		}

	public:
		BasicFlipFlop() : 
			FValue( false )
		{
		}
	};
//---------------------------------------------------------------------------
	class SRFlipFlop : public BasicFlipFlop
	{
		typedef Mitov::BasicFlipFlop inherited;

	public:
		OpenWire::SinkPin	SetInputPin;
		OpenWire::SinkPin	ResetInputPin;

	protected:
		void DoReceiveSet( void *_Data )
		{
			bool AValue = *(bool *)_Data;
			if( AValue )
				SetValue( true );
		}

		void DoReceiveReset( void *_Data )
		{
			bool AValue = *(bool *)_Data;
			if( AValue )
				SetValue( false );
		}

	public:
		SRFlipFlop()
		{
			SetInputPin.OnReceiveObject = this;
			SetInputPin.OnReceive = (OpenWire::TOnPinReceive)&SRFlipFlop::DoReceiveSet;

			ResetInputPin.OnReceiveObject = this;
			ResetInputPin.OnReceive = (OpenWire::TOnPinReceive)&SRFlipFlop::DoReceiveReset;
		}
	};
//---------------------------------------------------------------------------
	class ClockableFlipFlop : public Mitov::SRFlipFlop
	{
		typedef Mitov::SRFlipFlop inherited;

	protected:
		virtual void DoClock() = 0;

	protected:
		void DoReceiveClock( void *_Data )
		{
			DoClock();
		}

	public:
		OpenWire::SinkPin	ClockInputPin;

	public:
		ClockableFlipFlop()
		{
			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&ClockableFlipFlop::DoReceiveClock;
		}

	};
//---------------------------------------------------------------------------
	class TFlipFlop : public Mitov::ClockableFlipFlop
	{
		typedef Mitov::ClockableFlipFlop inherited;

	public:
		OpenWire::SinkPin	ToggleInputPin;

	protected:
		bool FToggleValue;

	protected:
		void DoReceiveToggle( void *_Data )
		{
			FToggleValue = *(bool *)_Data;
		}

	protected:
		virtual void DoClock()
		{
			if( FToggleValue )
				SetValue( !FValue );
		}

	public:
		TFlipFlop() :
			FToggleValue( false )
		{
			ToggleInputPin.OnReceiveObject = this;
			ToggleInputPin.OnReceive = (OpenWire::TOnPinReceive)&TFlipFlop::DoReceiveToggle;
		}

	};
//---------------------------------------------------------------------------
	class DFlipFlop : public Mitov::ClockableFlipFlop
	{
		typedef Mitov::ClockableFlipFlop inherited;

	public:
		OpenWire::SinkPin	DataInputPin;

	protected:
		bool FData;

	protected:
		virtual void DoClock()
		{
			SetValue( FData );
		}

	protected:
		void DoReceiveData( void *_Data )
		{
			FData = *(bool *)_Data;
		}

	public:
		DFlipFlop() :
			FData( false )
		{
			DataInputPin.OnReceiveObject = this;
			DataInputPin.OnReceive = (OpenWire::TOnPinReceive)&DFlipFlop::DoReceiveData;
		}

	};
//---------------------------------------------------------------------------
	class JKFlipFlop : public Mitov::BasicFlipFlop
	{
	public:
		OpenWire::SinkPin	JInputPin;
		OpenWire::SinkPin	KInputPin;
		OpenWire::SinkPin	ClockInputPin;

	protected:
		bool FJ;
		bool FK;

	protected:
		void DoReceiveJ( void *_Data )
		{
			FJ = *(bool *)_Data;
		}

		void DoReceiveK( void *_Data )
		{
			FK = *(bool *)_Data;
		}

		void DoReceiveClock( void *_Data )
		{
			if( FJ && FK )
				SetValue( !FValue );

			else if( FJ )
				SetValue( true );

			else if( FK )
				SetValue( false );

		}

	public:
		JKFlipFlop() :
			FJ( false ),
			FK( false )
		{
			JInputPin.OnReceiveObject = this;
			JInputPin.OnReceive = (OpenWire::TOnPinReceive)&JKFlipFlop::DoReceiveJ;

			KInputPin.OnReceiveObject = this;
			KInputPin.OnReceive = (OpenWire::TOnPinReceive)&JKFlipFlop::DoReceiveK;

			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&JKFlipFlop::DoReceiveClock;
		}

	};
//---------------------------------------------------------------------------
}

#endif
