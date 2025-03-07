////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_IQUID_CRYSTAL_DISPLAY_h
#define _MITOV_IQUID_CRYSTAL_DISPLAY_h

#include <Mitov.h>

namespace Mitov
{
//---------------------------------------------------------------------------
	class LiquidCrystalDisplay;
//---------------------------------------------------------------------------
	class LiquidCrystalElementBasic : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

	public: // Public for the print access
		LCD	*FLcd;

	public:
		virtual void DisplayInit() {}
		virtual void DisplayStart() {}

	public:
		LiquidCrystalElementBasic( Mitov::LiquidCrystalDisplay &AOwner );

	};
//---------------------------------------------------------------------------
	class LiquidCrystalElementBasicPositionedField : public LiquidCrystalElementBasic
	{
		typedef Mitov::LiquidCrystalElementBasic inherited;

	public:
		unsigned long Column;
		unsigned long Row;

	public:
		LiquidCrystalElementBasicPositionedField( Mitov::LiquidCrystalDisplay &AOwner ) :
			inherited( AOwner ),
			Column( 0 ),
			Row( 0 )
		{
		}

	};
//---------------------------------------------------------------------------
	class LiquidCrystalElementTextField : public LiquidCrystalElementBasicPositionedField
	{
		typedef Mitov::LiquidCrystalElementBasicPositionedField inherited;

	public:
		unsigned long Width;

		String	InitialValue;

	protected:
		virtual void SystemStart()
		{
			inherited::SystemStart();
			ClearLine();
			int AClearSize = FLcd->print( InitialValue );
			NewLine( AClearSize );
		}

	public:
		void ClearLine()
		{
			FLcd->setCursor( Column, Row );
		}

		void NewLine( int AClearSize )
		{
			for( int i = 0; i < Width - AClearSize; ++ i )
				FLcd->print( ' ' );
		}

	public:
		LiquidCrystalElementTextField( Mitov::LiquidCrystalDisplay &AOwner ) :
			inherited( AOwner ),
			Width( 16 )
		{
		}

	};
//---------------------------------------------------------------------------
	// TODO: Implement setCursor() and createChar()
	class LiquidCrystalDisplay : public OpenWire::Component
	{
		typedef OpenWire::Component inherited;

	public:
		OpenWire::SinkPin	ScrollLeftInputPin;
		OpenWire::SinkPin	ScrollRightInputPin;
		OpenWire::SinkPin	ClearInputPin;
		OpenWire::SinkPin	HomeInputPin;

	public:
		bool Enabled;
		bool AutoScroll;
		bool RightToLeft;
		bool ShowCursor;
		bool Blink;

	protected:
		unsigned int FCols;
		unsigned int FRows;
		unsigned int FCursorLine;

	public: // Public for the print access
		LCD	*FLcd;
		Mitov::SimpleList<LiquidCrystalElementBasic *>	FElements;

	public:
		void ClearLine()
		{
			FLcd->setCursor( 0, FCursorLine );
//			for( int i = 0; i < FCols; ++ i )
//				FLcd->print( ' ' );

//			FLcd->setCursor( 0, FCursorLine );
		}

		void NewLine( int AClearSize )
		{
			for( int i = 0; i < FCols - AClearSize; ++ i )
				FLcd->print( ' ' );

			++FCursorLine;
			if( FCursorLine >= FRows )
				FCursorLine = 0;

//			FLcd->setCursor( 0, FCursorLine );
		}

	public:
		void SetEnabled( bool AValue )
		{
			if( Enabled == AValue )
				return;

			Enabled = AValue;
			UpdateEnabled();
		}

		void SetAutoScroll( bool AValue )
		{
			if( AutoScroll == AValue )
				return;

			AutoScroll = AValue;
			UpdateAutoScroll();
		}

		void SetRightToLeft( bool AValue )
		{
			if( RightToLeft == AValue )
				return;

			RightToLeft = AValue;
			UpdateRightToLeft();
		}

		void SetShowCursor( bool AValue )
		{
			if( ShowCursor == AValue )
				return;

			ShowCursor = AValue;
			UpdateShowCursor();
		}

		void SetBlink( bool AValue )
		{
			if( Blink == AValue )
				return;

			Blink = AValue;
			UpdateBlink();
		}

	protected:
		void UpdateEnabled()
		{
			if( Enabled )
				FLcd->display();

			else
				FLcd->noDisplay();

		}

		void UpdateAutoScroll()
		{
			if( AutoScroll )
				FLcd->autoscroll();

			else
				FLcd->noAutoscroll();

		}

		void UpdateRightToLeft()
		{
			if( RightToLeft )
				FLcd->rightToLeft();

			else
				FLcd->leftToRight();

		}

		void UpdateShowCursor()
		{
			if( ShowCursor )
				FLcd->cursor();

			else
				FLcd->noCursor();

		}

		void UpdateBlink()
		{
			if( Blink )
				FLcd->blink();

			else
				FLcd->noBlink();

		}

		void DoScrollLeft( void * )
		{
			FLcd->scrollDisplayLeft();
		}

		void DoScrollRight( void * )
		{
			FLcd->scrollDisplayRight();
		}

		void DoClear( void * )
		{
			FLcd->clear();
		}

		void DoHome( void * )
		{
			FLcd->home();
		}

	public:
		virtual void SystemInit()
		{
			inherited::SystemInit();

			FLcd->begin( FCols, FRows );
			UpdateEnabled();
			UpdateAutoScroll();
			UpdateRightToLeft();
			UpdateShowCursor();
			UpdateBlink();

			for( Mitov::SimpleList<LiquidCrystalElementBasic *>::iterator Iter = FElements.begin(); Iter != FElements.end(); ++Iter )
				( *Iter)->DisplayInit();

//			FLcd->setCursor(0,0);
		}

		virtual void SystemStart()
		{
			inherited::SystemStart();

			for( Mitov::SimpleList<LiquidCrystalElementBasic *>::iterator Iter = FElements.begin(); Iter != FElements.end(); ++Iter )
				( *Iter)->DisplayStart();
		}

	public:
		LiquidCrystalDisplay( LCD *ALcd, unsigned int ACols, unsigned int ARows ) :
			FLcd( ALcd ),
			FCols( ACols ),
			FRows( ARows ),
			Enabled( true ),
			AutoScroll( false ),
			RightToLeft( false ),
			ShowCursor( false ),
			Blink( false ),
			FCursorLine( 0 )
		{
			ScrollLeftInputPin.OnReceiveObject = this;
			ScrollLeftInputPin.OnReceive = (OpenWire::TOnPinReceive)&LiquidCrystalDisplay::DoScrollLeft;

			ScrollRightInputPin.OnReceiveObject = this;
			ScrollRightInputPin.OnReceive = (OpenWire::TOnPinReceive)&LiquidCrystalDisplay::DoScrollRight;

			ClearInputPin.OnReceiveObject = this;
			ClearInputPin.OnReceive = (OpenWire::TOnPinReceive)&LiquidCrystalDisplay::DoClear;

			HomeInputPin.OnReceiveObject = this;
			HomeInputPin.OnReceive = (OpenWire::TOnPinReceive)&LiquidCrystalDisplay::DoHome;
		}

		~LiquidCrystalDisplay()
		{
			delete FLcd;
		}

	};
//---------------------------------------------------------------------------
	class LiquidCrystalDisplayI2C : public LiquidCrystalDisplay
	{
		typedef Mitov::LiquidCrystalDisplay inherited;

	public:
		bool	Backlight;

	public:
		void SetBacklight( bool AValue )
		{
			if( Backlight == AValue )
				return;

			Backlight = AValue;
			UpdateBacklight();
		}

	public:
		virtual void SystemInit()
		{
			inherited::SystemInit();
			UpdateBacklight();
		}

	public:
		void UpdateBacklight()
		{
			if( Backlight )
				inherited::FLcd->setBacklight( 255 );

			else
				inherited::FLcd->setBacklight( 0 );

		}

	public:
		LiquidCrystalDisplayI2C( LCD *ALcd, unsigned int ACols, unsigned int ARows ) :
			inherited( ALcd, ACols, ARows ),
			Backlight( true )
		{
		}

	};
//---------------------------------------------------------------------------
	class LiquidCrystalElementDefineCustomCharacter : public LiquidCrystalElementBasic
	{
		typedef Mitov::LiquidCrystalElementBasic inherited;

	protected:
		uint8_t FCharMap[ 8 ];

		uint8_t	FIndex;

	public:
		virtual void DisplayInit()
		{
			FLcd->createChar( FIndex, FCharMap );
		}

	public:
		LiquidCrystalElementDefineCustomCharacter( Mitov::LiquidCrystalDisplay &AOwner, uint8_t AIndex, uint8_t Byte0, uint8_t Byte1, uint8_t Byte2, uint8_t Byte3, uint8_t Byte4, uint8_t Byte5, uint8_t Byte6, uint8_t Byte7 ) :
			inherited( AOwner ),
			FIndex( AIndex )
		{
			AOwner.FElements.push_back( this );

			FCharMap[ 0 ] = Byte0;
			FCharMap[ 1 ] = Byte1;
			FCharMap[ 2 ] = Byte2;
			FCharMap[ 3 ] = Byte3;
			FCharMap[ 4 ] = Byte4;
			FCharMap[ 5 ] = Byte5;
			FCharMap[ 6 ] = Byte6;
			FCharMap[ 7 ] = Byte7;
		}

	};
//---------------------------------------------------------------------------
	class LiquidCrystalElementCutomCharacterField : public LiquidCrystalElementBasicPositionedField
	{
		typedef Mitov::LiquidCrystalElementBasicPositionedField inherited;

	public:
		long	Index;

	protected:
		bool	FModified;

	public:
		void SetIndex( long	AValue )
		{
			if( AValue > 7 )
				AValue = 7;

			else if( AValue < 0 )
				AValue = 0;

			if( Index == AValue )
				return;

			Index = AValue;
			FModified = true;
		}

		void SetColumn( unsigned long AValue )
		{
			if( AValue < 0 )
				AValue = 0;

			if( Column == AValue )
				return;

			Column = AValue;
			FModified = true;
		}

		void SetRow( unsigned long AValue )
		{
			if( AValue < 0 )
				AValue = 0;

			if( Row == AValue )
				return;

			Row = AValue;
			FModified = true;
		}

	public:
		virtual void DisplayStart() 
		{
			DisplayCharacter();
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			inherited::SystemLoopBegin( currentMicros );
			if( FModified )
			{
				DisplayCharacter();
				FModified = false;
			}
		}

	protected:
		void DisplayCharacter()
		{
			FLcd->setCursor( Column, Row );
			FLcd->write( (uint8_t) Index );
		}

	public:
		LiquidCrystalElementCutomCharacterField( Mitov::LiquidCrystalDisplay &AOwner ) :
			inherited( AOwner ),
			Index( 0 ),
			FModified( false )
		{
			AOwner.FElements.push_back( this );
		}

	};
//---------------------------------------------------------------------------
	template<typename T_LCD, T_LCD *T_LCD_INSTANCE, typename T> class LiquidCrystalDisplayInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			T_LCD_INSTANCE->ClearLine();
			int AClearSize = T_LCD_INSTANCE->FLcd->print( *(T*)_Data );
			T_LCD_INSTANCE->NewLine( AClearSize );
		}
	};
//---------------------------------------------------------------------------
	template<typename T_LCD, T_LCD *T_LCD_INSTANCE> class LiquidCrystalDisplayStringInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			T_LCD_INSTANCE->ClearLine();
			int AClearSize = T_LCD_INSTANCE->FLcd->print( (char*)_Data );
			T_LCD_INSTANCE->NewLine( AClearSize );
		}
	};
//---------------------------------------------------------------------------
	template<typename T_LCD, T_LCD *T_LCD_INSTANCE> class LiquidCrystalDisplayColorInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			T_LCD_INSTANCE->ClearLine();
			int AClearSize = T_LCD_INSTANCE->FLcd->print( "(" );
			AClearSize += T_LCD_INSTANCE->FLcd->print( (int)((TColor *)_Data)->Red );
			AClearSize += T_LCD_INSTANCE->FLcd->print( "," );
			AClearSize += T_LCD_INSTANCE->FLcd->print( (int)((TColor *)_Data)->Green );
			AClearSize += T_LCD_INSTANCE->FLcd->print( "," );
			AClearSize += T_LCD_INSTANCE->FLcd->print( (int)((TColor *)_Data)->Blue );
			AClearSize += T_LCD_INSTANCE->FLcd->print( ")" );
			T_LCD_INSTANCE->NewLine( AClearSize );
		}
	};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
	LiquidCrystalElementBasic::LiquidCrystalElementBasic( LiquidCrystalDisplay &AOwner ) :
		FLcd( AOwner.FLcd )
	{
	}
//---------------------------------------------------------------------------
}

#endif
