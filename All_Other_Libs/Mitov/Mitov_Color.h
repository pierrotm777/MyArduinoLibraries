////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_COLOR_h
#define _MITOV_COLOR_h

#include <Mitov.h>

namespace Mitov
{
//---------------------------------------------------------------------------
    class RandomColor : public CommonSource
    {
		typedef CommonSource inherited;

    public:
		bool	Enabled;
		TColor	Min;
		TColor	Max;
		long	Seed;

	public:
		virtual void SystemInit()
		{
			randomSeed( Seed );

			int AMin = min( Min.Red, Max.Red );
			int AMax = max( Min.Red, Max.Red );

			Min.Red = AMin;
			Max.Red = AMax;

			AMin = min( Min.Green, Max.Green );
			AMax = max( Min.Green, Max.Green );

			Min.Green = AMin;
			Max.Green = AMax;

			AMin = min( Min.Blue, Max.Blue );
			AMax = max( Min.Blue, Max.Blue );

			Min.Blue = AMin;
			Max.Blue = AMax;

			inherited::SystemInit();
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
            if( Enabled )
            {
				TColor AColor;

				AColor.Red = random( Min.Red, Max.Red + 1 );
				AColor.Green = random( Min.Green, Max.Green + 1 );
				AColor.Blue = random( Min.Blue, Max.Blue + 1 );

				OutputPin.Notify( &AColor );
			}
		}

	public:
		RandomColor() :
            Enabled( true ),
			Max( 0xFFFFFF ),
			Seed( 0 )
		{
		}

    };
//---------------------------------------------------------------------------
    class BasicColorSource : public CommonSource
    {
    public:
        TColor	InitialValue;

    };
//---------------------------------------------------------------------------
	class AnalogToColor : public BasicColorSource
	{
	public:
		OpenWire::SinkPin	RedInputPin;
		OpenWire::SinkPin	GreenInputPin;
		OpenWire::SinkPin	BlueInputPin;

	protected:
		TColor	FValue;

	protected:
		void DoReceiveRed( void *_Data )
		{
			unsigned char AValue = (*(float *)_Data ) * 255;
			if( FValue.Red == AValue )
				return;

			FValue.Red = AValue;
			OutputPin.Notify( &FValue );
		}

		void DoReceiveGreen( void *_Data )
		{
			unsigned char AValue = (*(float *)_Data ) * 255;
			if( FValue.Green == AValue )
				return;

			FValue.Green = AValue;
			OutputPin.Notify( &FValue );
		}

		void DoReceiveBlue( void *_Data )
		{
			unsigned char AValue = (*(float *)_Data ) * 255;
			if( FValue.Blue == AValue )
				return;

			FValue.Blue = AValue;
			OutputPin.Notify( &FValue );
		}

		virtual void SystemStart() 
		{
			FValue = InitialValue;
			OutputPin.Notify( &FValue );			
		}

	public:
		AnalogToColor()		
		{
			RedInputPin.OnReceiveObject = this;
			RedInputPin.OnReceive = (OpenWire::TOnPinReceive)&AnalogToColor::DoReceiveRed;

			GreenInputPin.OnReceiveObject = this;
			GreenInputPin.OnReceive = (OpenWire::TOnPinReceive)&AnalogToColor::DoReceiveGreen;

			BlueInputPin.OnReceiveObject = this;
			BlueInputPin.OnReceive = (OpenWire::TOnPinReceive)&AnalogToColor::DoReceiveBlue;
		}

	};
//---------------------------------------------------------------------------
	class ColorToAnalog : public CommonSink
	{
	public:
		OpenWire::SourcePin	RedOutputPin;
		OpenWire::SourcePin	GreenOutputPin;
		OpenWire::SourcePin	BlueOutputPin;

	protected:
		virtual void DoReceive( void *_Data )
		{
			TColor &AColor = *(TColor *)_Data;
			RedOutputPin.Notify( &AColor.Red );
			GreenOutputPin.Notify( &AColor.Green );
			BlueOutputPin.Notify( &AColor.Blue );
		}

	};	
//---------------------------------------------------------------------------
}

#endif
