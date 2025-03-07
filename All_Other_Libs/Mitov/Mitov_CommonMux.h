////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_COMMON_MUX_h
#define _MITOV_COMMON_MUX_h

#include <Mitov.h>

namespace Mitov
{
	template<typename T, typename T_OUT> class BasicToggleSwitch : public Mitov::BasicMultiInput<T, T_OUT>
	{
		typedef Mitov::BasicMultiInput<T, T_OUT> inherited;

	public:
		OpenWire::VlaueSinkPin<T>	TrueInputPin;
		OpenWire::VlaueSinkPin<T>	FalseInputPin;

		OpenWire::SinkPin	SelectInputPin;

	public:
		bool	InitialSelectValue;

	protected:
		bool	FSelectValue;

	protected:
		virtual void SystemStart()
		{
			FSelectValue = InitialSelectValue;
			inherited::SystemStart();
		}

	protected:
		virtual void DoReceiveSelect( void *_Data )
		{
			bool AValue = *(bool *)_Data;
			if( FSelectValue == AValue )
				return;

			FSelectValue = AValue;
			inherited::CalculateSendOutput( false );
		}

	public:
		BasicToggleSwitch() :
			FSelectValue( false ),
			InitialSelectValue( false )
		{
			SelectInputPin.OnReceiveObject = this;
			SelectInputPin.OnReceive = (OpenWire::TOnPinReceive)&BasicToggleSwitch::DoReceiveSelect;

			TrueInputPin.OnReceiveObject = this;
			TrueInputPin.OnReceive = (OpenWire::TOnPinReceive)&BasicToggleSwitch::DoReceive;

			FalseInputPin.OnReceiveObject = this;
			FalseInputPin.OnReceive = (OpenWire::TOnPinReceive)&BasicToggleSwitch::DoReceive;
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class ToggleSwitch : public Mitov::BasicToggleSwitch<T, T>
	{
		typedef Mitov::BasicToggleSwitch<T, T> inherited;

	protected:
		virtual T CalculateOutput()
		{
			if( inherited::FSelectValue )
				return inherited::TrueInputPin.Value;

			else
				return inherited::FalseInputPin.Value;
		}
	};
//---------------------------------------------------------------------------
	template<> class ToggleSwitch<char *> : public Mitov::BasicToggleSwitch<char *, char *>
	{
		typedef Mitov::BasicToggleSwitch<char *, char *> inherited;

	protected:
		virtual char * CalculateOutput()
		{
			if( inherited::FSelectValue )
				return (char *)inherited::TrueInputPin.Value.c_str();

			else
				return (char *)inherited::FalseInputPin.Value.c_str();
		}
	};
//---------------------------------------------------------------------------
	template<typename T> class BasicCommonMux : public Mitov::CommonMultiInput<T>
	{
		typedef Mitov::CommonMultiInput<T> inherited;

	protected:
		unsigned int	FChannel;

	public:
		unsigned int	InitialChannel;

	public:
		OpenWire::SinkPin	SelectInputPin;

	protected:
		virtual T CalculateOutput() = 0;

	protected:
		virtual void DoReceiveSelect( void *_Data )
		{
			unsigned long AChannel = *(unsigned long *)_Data;
			if( AChannel >= inherited::InputPins.size() )
				AChannel = inherited::InputPins.size() - 1;

			if( FChannel == AChannel )
				return;

			FChannel = AChannel;
			inherited::CalculateSendOutput( false );
		}

	protected:
		virtual void SystemStart()
		{
			FChannel = InitialChannel;
			inherited::SystemStart();
		}

	public:
		BasicCommonMux() :
			FChannel( 0 ),
			InitialChannel( 0 )
		{
			SelectInputPin.OnReceiveObject = this;
			SelectInputPin.OnReceive = (OpenWire::TOnPinReceive)&BasicCommonMux::DoReceiveSelect;
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class CommonMux : public BasicCommonMux<T>
	{
		typedef BasicCommonMux<T> inherited;

	protected:
		virtual T CalculateOutput()
		{
			return inherited::InputPins[ inherited::FChannel ].Value;
		}

	};
//---------------------------------------------------------------------------
	template<> class CommonMux<char *> : public Mitov::BasicCommonMux<char *>
	{
		typedef BasicCommonMux<char *> inherited;

	protected:
		virtual char * CalculateOutput()
		{
			return (char *)inherited::InputPins[ FChannel ].Value.c_str();
		}

	};
//---------------------------------------------------------------------------
}

#endif
