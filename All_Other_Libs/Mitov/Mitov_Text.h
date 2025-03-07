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
	class TextValue : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource inherited;

	public:
		OpenWire::SinkPin	ClockInputPin;

	public:
		String Value;

	protected:
		virtual void SystemStart()
		{
			inherited::SystemStart();
			OutputPin.Notify( (void *)Value.c_str() );
		}

		void DoClockReceive( void *_Data )
		{
			OutputPin.Notify( (void *)Value.c_str() );
		}

	public:
		TextValue( char *AValue ) :
			Value( AValue )
		{
			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&TextValue::DoClockReceive;
		}

	};
//---------------------------------------------------------------------------
	class BindableTextValue : public TextValue
	{
		typedef TextValue inherited;

	protected:
		String OldValue;

	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();
			OldValue = inherited::Value;
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( inherited::Value == OldValue )
				return;

			OldValue = inherited::Value;
			inherited::OutputPin.Notify( (void *)OldValue.c_str() );
		}

	public:
		BindableTextValue( char *AValue ) :
			inherited( AValue ),
			OldValue( AValue )
		{
		}

	};
//---------------------------------------------------------------------------
	class FormattedText;
//---------------------------------------------------------------------------
	class FormattedTextElementBasic : public OpenWire::Object // : public OpenWire::Component
	{
//		typedef OpenWire::Component inherited;
	protected:
		FormattedText &FOwner;

	public:
		virtual String GetText() = 0;
		virtual void SystemStart()
		{
		}

	public:
		FormattedTextElementBasic( FormattedText &AOwner );

	};
//---------------------------------------------------------------------------
	class FormattedText : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource inherited;

	public:
		OpenWire::SinkPin	ClockInputPin;

	public:
		Mitov::SimpleList<FormattedTextElementBasic *>	FElements;

	public:
		String Text;

	protected:
		bool FModified;

	public:
		void SetModified()
		{
			FModified = true;
		}

	protected:
		struct TStringItem
		{
			String	Text;
			FormattedTextElementBasic *Element;

/*
		public:
			TStringItem( TStringItem &AOther ) :
				Text( AOther.Text ),
				Element( AOther.Element )
			{
			}

			TStringItem( String	AText, FormattedTextElementBasic *AElement ) :
				Text( AText ),
				Element( AElement )
			{
			}
*/
		};

	protected:
		Mitov::SimpleList<TStringItem>	FReadyElements;

	protected:
		void AddReadyElement( String ATextItem, int AIndex )
		{
			TStringItem	AItem;
			AItem.Text = ATextItem;
			if( AIndex < FElements.size() )
				AItem.Element = FElements[ AIndex ];

			else
				AItem.Element = NULL;

			FReadyElements.push_back( AItem );
		}

		void InitElements()
		{
			FReadyElements.clear();
			String	ATextItem;
			String	AIndexText;
			bool	AInEscape;
			for( int i = 0; i < Text.length(); ++ i )
			{
				char AChar = Text[ i ];
				if( AInEscape )
				{
					if( AChar >= '0' && AChar <= '9' )
						AIndexText += AChar;

					else
					{
						if( AChar == '%' )
						{
							if( AIndexText.length() == 0 )
								ATextItem += '%';

							else
							{
								AddReadyElement( ATextItem, AIndexText.toInt() );
								ATextItem = "";
							}

						}

						else
						{
							if( AIndexText.length() == 0 )
								ATextItem += '%';

							else
							{
								AddReadyElement( ATextItem, AIndexText.toInt() );
								ATextItem = "";
							}

							ATextItem += AChar;
						}

						AInEscape = false;
					}
				}

				else
				{
					if( AChar == '%' )
					{
						AInEscape = true;
						AIndexText = "";
					}

					else
						ATextItem += AChar;

				}

			}

			if( AInEscape )
				AddReadyElement( ATextItem, AIndexText.toInt() );

			else if( ATextItem.length() )
			{
				TStringItem	AItem;

				AItem.Text = ATextItem;
				AItem.Element = NULL;

				FReadyElements.push_back( AItem );
			}
		}

		void ProcessSendOutput()
		{
			String AText;
			for( Mitov::SimpleList<TStringItem>::iterator Iter = FReadyElements.begin(); Iter != FReadyElements.end(); ++Iter )
			{
				AText += Iter->Text;
				if( Iter->Element )
					AText += Iter->Element->GetText();
			}

			OutputPin.Notify( (void *)AText.c_str() );
			FModified = false;
		}

	protected:
		virtual void SystemStart()
		{
			for( Mitov::SimpleList<FormattedTextElementBasic *>::iterator Iter = FElements.begin(); Iter != FElements.end(); ++Iter )
				(*Iter )->SystemStart();
			
			InitElements();
			inherited::SystemStart();
			ProcessSendOutput();
		}

		virtual void SystemLoopEnd()
		{
			if( FModified )
				if( ! ClockInputPin.IsConnected() )
					ProcessSendOutput();

			inherited::SystemLoopEnd();
		}

	protected:
		void DoClockReceive( void *_Data )
		{
			ProcessSendOutput();
		}

	public:
		FormattedText() :
			FModified( false )
		{
			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&FormattedText::DoClockReceive;
		}
	};
//---------------------------------------------------------------------------
	template<typename T_LCD, T_LCD *T_LCD_INSTANCE, typename T> class TextFormatElementInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			T_LCD_INSTANCE->SetValue( String( *(T*)_Data ));
		}
	};
//---------------------------------------------------------------------------
	template<typename T_LCD, T_LCD *T_LCD_INSTANCE> class TextFormatElementStringInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			T_LCD_INSTANCE->SetValue( (char*)_Data );
		}
	};
//---------------------------------------------------------------------------
	template<typename T_LCD, T_LCD *T_LCD_INSTANCE> class TextFormatElementColorInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			T_LCD_INSTANCE->SetValue( 
					String( "(" ) + 
					String((int)((TColor *)_Data)->Red ) + "," +
					String((int)((TColor *)_Data)->Green ) + "," +
					String((int)((TColor *)_Data)->Blue ) + ")" );
		}
	};
//---------------------------------------------------------------------------
	class FormattedTextElementText : public FormattedTextElementBasic
	{
		typedef Mitov::FormattedTextElementBasic inherited;

	public:
		String	InitialValue;
		String	FValue;

	public:
		void SetValue( String AValue )
		{
			FOwner.SetModified();
			FValue = AValue;
		}

	public:
		virtual String GetText()
		{
			return FValue;
		}

	public:
		virtual void SystemStart()
		{
//			inherited::SystemStart();
			FValue = InitialValue;
		}

	public:
		FormattedTextElementText( FormattedText &AOwner ) :
			inherited( AOwner )
		{
		}

	};
//---------------------------------------------------------------------------
	class FormattedTextInputElement : public FormattedTextElementBasic
	{
		typedef Mitov::FormattedTextElementBasic inherited;

	public:
		OpenWire::SinkPin	InputPin;

	protected:
		virtual void DoReceive( void *_Data ) = 0;

	public:
		FormattedTextInputElement( FormattedText &AOwner ) :
			inherited( AOwner )
		{
			InputPin.OnReceiveObject = this;
			InputPin.OnReceive = (OpenWire::TOnPinReceive)&FormattedTextInputElement::DoReceive;
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class FormattedTextElementTyped : public FormattedTextInputElement
	{
		typedef Mitov::FormattedTextInputElement inherited;

	public:
		T	InitialValue;
		T	FValue;

	public:
		virtual void SystemStart()
		{
//			inherited::SystemStart();
			FValue = InitialValue;
		}

	protected:
		virtual void DoReceive( void *_Data )
		{
			FOwner.SetModified();
			FValue = *(T *)_Data;
		}

	public:
		FormattedTextElementTyped( FormattedText &AOwner ) :
			inherited( AOwner )
		{
		}

	};
//---------------------------------------------------------------------------
	class FormattedTextElementInteger : public Mitov::FormattedTextElementTyped<long>
	{
		typedef Mitov::FormattedTextElementTyped<long> inherited;

	public:
		int	Base;

	public:
		virtual String GetText()
		{
			char AText[ 50 ];
			itoa( FValue, AText, Base );

			return AText;
		}

	public:
		FormattedTextElementInteger( FormattedText &AOwner ) :
			inherited( AOwner ),
			Base( 10 )			
		{
		}

	};
//---------------------------------------------------------------------------
	class FormattedTextElementAnalog : public Mitov::FormattedTextElementTyped<float>
	{
		typedef Mitov::FormattedTextElementTyped<float> inherited;

	public:
		int	MinWidth;
		int	Precision;

	public:
		virtual String GetText()
		{
			char AText[ 50 ];
			dtostrf( FValue,  MinWidth, Precision, AText );

			return AText;
		}

	public:
		FormattedTextElementAnalog( FormattedText &AOwner ) :
			inherited( AOwner ),
			MinWidth( 1 ),
			Precision( 3 )
		{
		}

	};
//---------------------------------------------------------------------------
	class FormattedTextElementDigital : public Mitov::FormattedTextElementTyped<bool>
	{
		typedef Mitov::FormattedTextElementTyped<bool> inherited;

	public:
		String	TrueValue;
		String	FalseValue;

	public:
		virtual String GetText()
		{
			if( FValue )
				return TrueValue;

			return FalseValue;
		}

	public:
		FormattedTextElementDigital( FormattedText &AOwner ) :
			inherited( AOwner ),
			TrueValue( "true" ),
			FalseValue( "false" )
		{
		}

	};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
	FormattedTextElementBasic::FormattedTextElementBasic( FormattedText &AOwner ) :
		FOwner( AOwner )
	{
		AOwner.FElements.push_back( this );
	}
//---------------------------------------------------------------------------
}

#endif
