////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_NEO_PIXEL_h
#define _MITOV_NEO_PIXEL_h

#include <Mitov.h>

#include <MitovEmbedded_Adafruit_NeoPixel/MitovEmbedded_Adafruit_NeoPixel.h>

namespace Mitov
{
	class NeoPixelsRunningColorGroup;
//---------------------------------------------------------------------------
	class NeoPixelsController
	{
	public:
		virtual void SetPixelColor( int AIndex, TColor AColor ) = 0;
		virtual TColor GetPixelColor( int AIndex ) = 0;

	};
//---------------------------------------------------------------------------
	class NeoPixelsCommonGroup : public OpenWire::Component
	{
	public:
		NeoPixelsController	*FOwner;
		int			FStartPixel;

	public:
		virtual void StartPixels( NeoPixelsController *AOwner, int &AStartPixel ) 
		{
			FOwner = AOwner;
			FStartPixel = AStartPixel;
		}

		virtual void PixelsClock( unsigned long currentMicros )
		{
		}

	};
//---------------------------------------------------------------------------
	class NeoPixelsBasicGroup : public NeoPixelsCommonGroup
	{
		typedef NeoPixelsCommonGroup inherited;

	public:
		int		CountPixels;

	public:
		virtual void StartPixels( NeoPixelsController *AOwner, int &AStartPixel )
		{
			inherited::StartPixels( AOwner, AStartPixel );
			AStartPixel += CountPixels;
		}

	public:
		NeoPixelsBasicGroup() :
			CountPixels( 10 )
		{
		}
	};
//---------------------------------------------------------------------------
	class NeoPixels : public OpenWire::Component, public NeoPixelsController
	{
		typedef OpenWire::Component inherited;

	public:
		OpenWire::SinkPin	ClockInputPin;

	public:
		float   Brightness;
		Mitov::SimpleObjectList<NeoPixelsCommonGroup*>	PixelGroups;

	public:
		bool	FModified;

	protected:
		int	FPinNumber;

	public:
		void SetBrightness( float AValue )
		{
			if( Brightness == AValue )
				return;

			IntSetBrightness( AValue );
		}

	public:
		void SetPixelColor( int AIndex, TColor AColor )
		{
			FPixel.setPixelColor( AIndex, AColor.Red, AColor.Green, AColor.Blue );
			FModified = true;
		}

		TColor GetPixelColor( int AIndex )
		{
			return TColor( FPixel.getPixelColor( AIndex ), true );
		}

	protected:
		MitovEmbedded_Adafruit_NeoPixel	&FPixel;

	protected:
		void IntSetBrightness( float AValue )
		{
			Brightness = AValue;
			FPixel.setBrightness( AValue * 255 );
			FModified = true;
		}
/*
		void DoReceiveColor( void *_Data )
		{
			TColor	AColor = *(TColor*)_Data;
			Serial.println( AColor.Blue );
			FPixel.setPixelColor(FIndex, AColor.Red, AColor.Green, AColor.Blue);
			FPixel.show();
		}
*/
		virtual void SystemInit() 
		{

//					Serial.println( 2 );
//					Serial.println( CountPixels );
					//FPixel = new MitovEmbedded_Adafruit_NeoPixel( 1, 2, NEO_KHZ400 + NEO_RGB );
					//FPixel = new MitovEmbedded_Adafruit_NeoPixel( 1, 0, NEO_KHZ400 + NEO_RGB );
//			FPixel.setPin( 2 );
//			FPixel.begin();
//			FPixel.show();
//			FIndex = InitialIndex;
//			DoReceiveColor( &InitialColor );
//			inherited::SystemStart();
//					return;

//					Serial.println( APinNumber );
//					Serial.println( CountPixels );
			FPixel.setPin( FPinNumber );
//					FPixel.setBrightness( 20 );
//					FPixel = new MitovEmbedded_Adafruit_NeoPixel( CountPixels, APinNumber, NEO_KHZ400 + NEO_RGB );
//					FPixel = new MitovEmbedded_Adafruit_NeoPixel( 5, 2, NEO_KHZ400 + NEO_RGB );
			FPixel.begin();
//				IntSetInitialColor( InitialColor );
			IntSetBrightness( Brightness );

			int AStartPixel = 0;
			for( int i = 0; i < PixelGroups.size(); ++i )
				PixelGroups[ i ]->StartPixels( this, AStartPixel );
			

			inherited::SystemInit();
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			for( int i = 0; i < PixelGroups.size(); ++i )
				PixelGroups[ i ]->PixelsClock( currentMicros );

			inherited::SystemLoopBegin( currentMicros );
		}

		virtual void SystemLoopEnd()
		{
			if( FModified )
				if( ! ClockInputPin.IsConnected())
				{
					FPixel.show();
					FModified = false;
				}

			inherited::SystemLoopEnd();
		}

		void DoReceiveClock(void *)
		{
			if( FModified )
			{
				FPixel.show();
				FModified = false;
			}
		}

	public:
		NeoPixels( int APinNumber, MitovEmbedded_Adafruit_NeoPixel &APixel ) :
			FPinNumber( APinNumber ),
		    FPixel( APixel ),
			Brightness( 1 )
		{
			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&NeoPixels::DoReceiveClock;
		}

	};
//---------------------------------------------------------------------------
	class NeoPixelsBasicInitialColorGroup : public NeoPixelsBasicGroup
	{
		typedef NeoPixelsBasicGroup inherited;

	public:
		TColor	InitialColor;

		virtual void StartPixels( NeoPixelsController *AOwner, int &AStartPixel )
		{
			inherited::StartPixels( AOwner, AStartPixel );

			for( int i = 0; i < CountPixels; ++i )
				FOwner->SetPixelColor( FStartPixel + i, InitialColor );
		}

	};
//---------------------------------------------------------------------------
	class NeoPixelsBasicColorGroup : public NeoPixelsBasicInitialColorGroup
	{
		typedef NeoPixelsBasicInitialColorGroup inherited;

	protected:
		TColor	FColor;

	public:
		OpenWire::SinkPin	ColorInputPin;

	public:
		void SetInitialColor( TColor AValue )
		{
			if( InitialColor == AValue )
				return;

			InitialColor = AValue;
			FColor = AValue;
			ApplyColorsAll(); 
//			FOwner->FModified = true;
		}

	protected:
		virtual void ApplyColorsAll()
		{
			for( int i = 0; i < CountPixels; ++i )
				FOwner->SetPixelColor( FStartPixel + i, FColor );
		}

		virtual void ApplyColors() {}

		void IntSetColor( TColor AValue )
		{
			if( FColor == AValue )
				return;

			FColor = AValue;
			ApplyColors(); 
//			FOwner->FModified = true;
		}

		void DoReceiveColor( void *_Data )
		{
			IntSetColor( *(TColor *)_Data );
		}

	protected:
		virtual void StartPixels( NeoPixelsController *AOwner, int &AStartPixel )
		{
			inherited::StartPixels( AOwner, AStartPixel );
//			Serial.println( CountPixels );
			IntSetColor( InitialColor );
//			FOwner->FModified = true;
//			Serial.println( FStartPixel );
		}

	public:
		NeoPixelsBasicColorGroup()
		{
			ColorInputPin.OnReceiveObject = this;
			ColorInputPin.OnReceive = (OpenWire::TOnPinReceive)&NeoPixelsBasicColorGroup::DoReceiveColor;
		}

	};
//---------------------------------------------------------------------------
	class NeoPixelsGroup : public NeoPixelsBasicInitialColorGroup
	{
		typedef NeoPixelsBasicInitialColorGroup inherited;

	protected:
		class PixelVlaueSinkPin : public OpenWire::VlaueSinkPin<TColor>
		{
			typedef OpenWire::VlaueSinkPin<TColor> inherited;

		public:
			NeoPixelsGroup *FOwner;
			int				FIndex;

		public:
			virtual void Receive( void *_Data )
			{
				if( *(TColor *)_Data != Value )
					FOwner->FOwner->SetPixelColor( FIndex, *(TColor *)_Data );

				inherited::Receive( _Data );
			}
		};

	public:
		Mitov::SimpleList<PixelVlaueSinkPin> InputPins;

	protected:
		virtual void StartPixels( NeoPixelsController *AOwner, int &AStartPixel )
		{
			inherited::StartPixels( AOwner, AStartPixel );
			for( int i = 0; i < InputPins.size(); ++i )
			{
				InputPins[ i ].FOwner = this;
				InputPins[ i ].FIndex = FStartPixel + i;
				InputPins[ i ].Value = InitialColor;
//				FOwner->SetPixelColor( FStartPixel + i, InitialColor );
//				Iter->OnReceiveObject = this;
//				Iter->OnReceive = (OpenWire::TOnPinReceive)&NeoPixelsGroup::DoReceive;
//				Iter->Value = T_VALUE;
			}

		}

	};
//---------------------------------------------------------------------------
	class NeoPixelsRepeatGroup : public NeoPixelsBasicInitialColorGroup, public NeoPixelsController
	{
		typedef NeoPixelsBasicInitialColorGroup inherited;

	public:
		Mitov::SimpleObjectList<NeoPixelsCommonGroup*>	PixelGroups;

	protected:
		int FSubPixelCount;
		int FRepeatCount;

	public:
		virtual void SetPixelColor( int AIndex, TColor AColor )
		{
			for( int i = 0; i < FRepeatCount; ++i )
				FOwner->SetPixelColor( FStartPixel + AIndex + i * FSubPixelCount, AColor );
		}

		virtual TColor GetPixelColor( int AIndex )
		{
			return FOwner->GetPixelColor( AIndex );
		}

	public:
		virtual void StartPixels( NeoPixelsController *AOwner, int &AStartPixel ) 
		{
			inherited::StartPixels( AOwner, AStartPixel );

			FSubPixelCount = 0;
			for( int i = 0; i < PixelGroups.size(); ++i )
				PixelGroups[ i ]->StartPixels( this, FSubPixelCount );

			if( FSubPixelCount == 0 )
				FRepeatCount = 0;

			else
				FRepeatCount = ( CountPixels + FSubPixelCount - 1 ) / FSubPixelCount;

		}

		virtual void PixelsClock( unsigned long currentMicros )
		{
			inherited::PixelsClock( currentMicros );

			for( int i = 0; i < PixelGroups.size(); ++i )
				PixelGroups[ i ]->PixelsClock( currentMicros );
		}
	};
//---------------------------------------------------------------------------
	class NeoPixelsSingleColorGroup : public NeoPixelsBasicColorGroup
	{
		typedef NeoPixelsBasicColorGroup inherited;

	protected:
		virtual void ApplyColors()
		{
			ApplyColorsAll();
		}

	};
//---------------------------------------------------------------------------
	class NeoPixelsReversedProperty
	{
	protected:
		NeoPixelsRunningColorGroup &FOwner;

	public:
		bool Reversed;
		bool AllPixels;

	public:
		void SetReversed( bool AValue );

	public:
		NeoPixelsReversedProperty( NeoPixelsRunningColorGroup &AOwner ) :
		  FOwner( AOwner )
		{
		}
	};
//---------------------------------------------------------------------------
	class NeoPixelsRunningColorGroup : public NeoPixelsBasicColorGroup
	{
		typedef NeoPixelsBasicColorGroup inherited;

	public:
		OpenWire::SinkPin	StepInputPin;
		OpenWire::SourcePin	ColorOutputPin;

	public:
		NeoPixelsReversedProperty	Reversed;

	public:
		void ReversePixels()
		{
			for( int i = 0; i < CountPixels / 2; ++i )
			{
				TColor AOldColor1 = FOwner->GetPixelColor( FStartPixel + ( CountPixels - i - 1 ));
				TColor AOldColor2 = FOwner->GetPixelColor( FStartPixel + i );

				FOwner->SetPixelColor( FStartPixel + i, AOldColor1 );
				FOwner->SetPixelColor( FStartPixel + ( CountPixels - i - 1 ), AOldColor2 );
			}
		}

	protected:
		void AnimatePixels()
		{
			if( Reversed.Reversed )
			{
				TColor AOldColor = FOwner->GetPixelColor( FStartPixel );
				ColorOutputPin.Notify( &AOldColor );
				for( int i = 0; i < CountPixels - 1; ++i )
				{
					AOldColor = FOwner->GetPixelColor( FStartPixel + i + 1 );
					FOwner->SetPixelColor( FStartPixel + i, AOldColor );
				}

				FOwner->SetPixelColor( FStartPixel + CountPixels - 1, FColor );
			}

			else
			{
				TColor AOldColor = FOwner->GetPixelColor( FStartPixel + CountPixels - 1 );
				ColorOutputPin.Notify( &AOldColor );
				for( int i = CountPixels - 1; i--; )
				{
					AOldColor = FOwner->GetPixelColor( FStartPixel + i );
					FOwner->SetPixelColor( FStartPixel + i + 1, AOldColor );
				}

				FOwner->SetPixelColor( FStartPixel, FColor );
			}
//			FOwner->FModified = true;
		}

		void DoReceiveStep( void *_Data )
		{
			AnimatePixels();
		}

	protected:
		virtual void PixelsClock( unsigned long currentMicros )
		{
			if( StepInputPin.IsConnected())
				return;

			AnimatePixels();
		}

	public:
		NeoPixelsRunningColorGroup() :
		  Reversed( *this )
		{
			StepInputPin.OnReceiveObject = this;
			StepInputPin.OnReceive = (OpenWire::TOnPinReceive)&NeoPixelsRunningColorGroup::DoReceiveStep;
		}
	};
//---------------------------------------------------------------------------
	inline void NeoPixelsReversedProperty::SetReversed( bool AValue )
	{
		if( Reversed == AValue )
			return;

		Reversed = AValue;
		if( AllPixels )
			FOwner.ReversePixels();

	}
//---------------------------------------------------------------------------
	class NeoPixelsColorPixelGroup : public NeoPixelsBasicColorGroup
	{
	public:
		OpenWire::SinkPin	IndexInputPin;

	public:
		unsigned long InitialIndex;

	protected:
		unsigned long	FIndex;
		bool			FModified;

	protected:
		void DoReceiveIndex( void *_Data )
		{
			unsigned long AIndex = *(unsigned long *)_Data;
			if( AIndex > CountPixels )
				AIndex = CountPixels;

			if( FIndex == AIndex )
				return;

			FIndex = AIndex;
			FModified = true;
		}

		virtual void PixelsClock( unsigned long currentMicros )
		{
			if( FModified )
			{
				FOwner->SetPixelColor( FStartPixel + FIndex, FColor );
//				FOwner->FModified = true;
				FModified = false;
			}
		}

		virtual void ApplyColors() 
		{
			FModified = true;
		}

	public:
		NeoPixelsColorPixelGroup() :
		  InitialIndex( 0 )
		{
			IndexInputPin.OnReceiveObject = this;
			IndexInputPin.OnReceive = (OpenWire::TOnPinReceive)&NeoPixelsColorPixelGroup::DoReceiveIndex;
		}
	};
//---------------------------------------------------------------------------
}

#endif
