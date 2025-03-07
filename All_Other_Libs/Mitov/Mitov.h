////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_h
#define _MITOV_h

#include <OpenWire.h>

namespace Mitov
{
	const bool GBooleanConst[] = { false, true };
//---------------------------------------------------------------------------
	#pragma pack(push, 1)
	struct TColor
	{
	public:
		unsigned char Red;
		unsigned char Green;
		unsigned char Blue;
		unsigned char Alpha;

	public:
		void SetValue( const long AValue )
		{
			*((long *)this) = AValue & 0xFFFFFF;
//			Blue = ( AValue >> 16 ) & 0xFF;
//			Green = ( AValue >> 8 ) & 0xFF;
//			Red = AValue & 0xFF;
//			Alpha = 0;
		}

		void operator =( const long AValue )
		{
			SetValue( AValue );
		}

		bool operator ==( const TColor other )
		{
			return *((long *)this) == *((long *)&other);
		}

		operator long()
		{
			return *((long *)this);
		}

	public:
		TColor( long AValue = 0 )
		{
			SetValue( AValue );
		}

		TColor( unsigned char ARed, unsigned char AGreen, unsigned char ABlue ) :
			Red( ARed ),
			Green( AGreen ),
			Blue( ABlue ),
			Alpha( 0 )
		{
		}

		TColor( long AValue, bool ) // For Windows BGR support
		{
			Red = ( AValue >> 16 ) & 0xFF;
			Green = ( AValue >> 8 ) & 0xFF;
			Blue = AValue & 0xFF;
			Alpha = 0;
		}
	};
	#pragma pack(pop)
//---------------------------------------------------------------------------
	template <class T> void swap ( T& a, T& b )
	{
	  T c(a); a=b; b=c;
	}
//---------------------------------------------------------------------------
	class Bytes
	{
	public:
		unsigned char *_Bytes;
		unsigned int  _BytesSize;

	public:
		Bytes() :
			_BytesSize( 0 )
		{
		}
	};
//---------------------------------------------------------------------------
	class CommonSource : public OpenWire::Component
	{
	public:
		OpenWire::SourcePin	OutputPin;

	};
//---------------------------------------------------------------------------
	class CommonEnableSource : public OpenWire::Component
	{
	public:
		OpenWire::SourcePin	OutputPin;

    public:
        bool Enabled;

    public:
        CommonEnableSource() : Enabled( true ) {}

	};
//---------------------------------------------------------------------------
	class CommonSink : public OpenWire::Component
	{
	public:
		OpenWire::SinkPin	InputPin;

	protected:
		virtual void DoReceive( void *_Data ) = 0;

	public:
		CommonSink()
		{
			InputPin.OnReceiveObject = this;
			InputPin.OnReceive = (OpenWire::TOnPinReceive)&CommonSink::DoReceive;
		}
	};
//---------------------------------------------------------------------------
	class CommonFilter : public CommonSource
	{
	public:
		OpenWire::SinkPin	InputPin;

	protected:
		virtual void DoReceive( void *_Data ) = 0;

	public:
		CommonFilter()
		{
			InputPin.OnReceiveObject = this;
			InputPin.OnReceive = (OpenWire::TOnPinReceive)&CommonFilter::DoReceive;
		}
	};
//---------------------------------------------------------------------------
	class CommonEnableFilter : public CommonFilter
	{
    public:
        bool Enabled;

    public:
        CommonEnableFilter() : Enabled( true ) {}

	};
//---------------------------------------------------------------------------
	template<typename T> class CommonTypedFilter : public CommonEnableFilter
    {
	protected:
        virtual T FilterValue( T AValue ) = 0;

	protected:
		virtual void DoReceive( void *_Data )
        {
            if( ! Enabled )
            {
			    OutputPin.Notify( _Data );
                return;
            }

			T AValue = *(T*)_Data;

            AValue = FilterValue( AValue );

			OutputPin.Notify( &AValue );
        }

    };
//---------------------------------------------------------------------------
	class CommonGenerator : public CommonSource
	{
	protected:
		unsigned long FLastTime;
		unsigned long FPeriod;

	public:
		bool	Enabled;
		float	Frequency;

	protected:
		virtual void Clock() = 0;

	protected:
		virtual void CalculateFields()
		{
			FPeriod = ( (( 1 / Frequency ) * 1000000 ) + 0.5 );
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
/*

		if( currentMicros - FLastTime >= FPeriod )
			{
				Clock();
				FLastTime = currentMicros - ( FPeriod - (currentMicros - FLastTime) );
			}
*/
			while( currentMicros - FLastTime >= FPeriod )
			{
				if( Enabled )
					Clock();

				FLastTime += FPeriod;
//				Serial.println( FPeriod );
			}
		}

	public:
		CommonGenerator() :
			Enabled( true ),
			Frequency( 1.0 ),
			FLastTime( 0 ),
			FPeriod( 0 )
		{
		}
	};
//---------------------------------------------------------------------------
	template<typename T> class ValueSource : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource inherited;

	public:
		OpenWire::SinkPin	ClockInputPin;

	public:
		T Value;

	protected:
		virtual void SystemStart()
		{
			inherited::SystemStart();
			OutputPin.Notify( &Value );
		}

		void DoClockReceive( void *_Data )
		{
			OutputPin.Notify( &Value );
		}

	public:
		ValueSource( T AValue ) :
			Value( AValue )
		{
			ClockInputPin.OnReceiveObject = this;
			ClockInputPin.OnReceive = (OpenWire::TOnPinReceive)&ValueSource::DoClockReceive;
		}

	};
//---------------------------------------------------------------------------
	template<typename T> class BindableValueSource : public Mitov::ValueSource<T>
	{
		typedef Mitov::ValueSource<T> inherited;

	protected:
		T OldValue;

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
			inherited::OutputPin.Notify( &OldValue );
		}

	public:
		BindableValueSource( T AValue ) :
			inherited( AValue ),
			OldValue( AValue )
		{
		}

	};
//---------------------------------------------------------------------------
	template<typename T, typename T_OUT> class BasicMultiInput : public Mitov::CommonSource
	{
		typedef Mitov::CommonSource	inherited;

	protected:
		bool	FModified;
		T_OUT	FLastOutput;

	protected:
		void DoReceive( void *_Data )
		{
			FModified = true;
		}

	protected:
		virtual T_OUT CalculateOutput() = 0;

		virtual void CalculateSendOutput( bool AFromStart )
		{
			T_OUT AValue = CalculateOutput();
			if( ! AFromStart )
				if( FLastOutput == AValue )
					return;

			OutputPin.Notify( &AValue );
			FLastOutput = AValue;

			FModified = false;
		}

		virtual void SystemStart()
		{
			CalculateSendOutput( true );
		}

		virtual void SystemLoopEnd()
		{
			if( FModified )
				CalculateSendOutput( false );

		}

	public:
		BasicMultiInput() : FModified( false ) {}

	};
//---------------------------------------------------------------------------
	template<typename T, typename T_OUT> class BasicCommonMultiInput : public Mitov::BasicMultiInput<T, T_OUT>
	{
		typedef Mitov::BasicMultiInput<T, T_OUT>	inherited;

	public:
		Mitov::SimpleList<OpenWire::VlaueSinkPin<T> > InputPins;

	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();

			for( OpenWire::VlaueSinkPin<T> *Iter = InputPins.begin(); Iter != InputPins.end(); ++Iter )
			{
				Iter->OnReceiveObject = this;
				Iter->OnReceive = (OpenWire::TOnPinReceive)&BasicCommonMultiInput::DoReceive;
			}
		}
	};
//---------------------------------------------------------------------------
	template<typename T> class CommonMultiInput : public Mitov::BasicCommonMultiInput<T,T>
	{
		typedef Mitov::BasicCommonMultiInput<T,T>	inherited;

	};
//---------------------------------------------------------------------------
	class DigitalInput : public CommonSource
	{
		typedef Mitov::CommonSource	inherited;

	public:
		int PinNumber;


	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();
			pinMode(PinNumber, INPUT);
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
//			Serial.println( "Test2" );
			bool AValue = digitalRead( PinNumber );
			OutputPin.Notify( &AValue );
		}

	public:
		DigitalInput( int APinNumber = 2 ) :
			PinNumber( APinNumber )
		{
		}
	};
//---------------------------------------------------------------------------
	class DigitalOutput : public CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
//			Serial.println( "Test1" );
			if( *(bool*)_Data )
				digitalWrite( PinNumber, HIGH );

			else
				digitalWrite( PinNumber, LOW );

		}

	public:
		int PinNumber;

	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();
			pinMode(PinNumber, OUTPUT);
		}

	public:
		DigitalOutput( int APinNumber = 13 ) :
			PinNumber( APinNumber )
		{
		}
	};
//---------------------------------------------------------------------------
	class ArduinoAnalogInputChannel : public CommonSource
	{
		typedef Mitov::CommonSource	inherited;

	protected:
		int		FPinNumber;
		float	FLastOutput;

	protected:
		virtual void SystemStart()
		{
			FLastOutput = analogRead( FPinNumber ) / 1023.0;
	    	OutputPin.Notify( &FLastOutput );
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			float AValue = analogRead( FPinNumber ) / 1023.0;
			if( AValue == FLastOutput )
				return;

			FLastOutput = AValue;
			OutputPin.Notify( &AValue );
		}

	public:
		ArduinoAnalogInputChannel( int APinNumber = 2 ) :
			FPinNumber( APinNumber )
		{
		}
	};
//---------------------------------------------------------------------------
	class AnalogOutput : public CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			float AValue = *(float*)_Data;
			analogWrite( PinNumber, ( AValue * 255 ) + 0.5 );
		}

	public:
		int PinNumber;

	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();

			pinMode(PinNumber, OUTPUT);
		}

	public:
		AnalogOutput( int APinNumber = 13 ) :
			PinNumber( APinNumber )
		{
		}
	};
//---------------------------------------------------------------------------
    class ArduinoDigitalChannel : public CommonSource
    {
		typedef Mitov::CommonSource	inherited;

	protected:
		int  FPinNumber;
        bool FIsOutput;
        bool FIsPullUp;
        bool FIsAnalog;
		bool FCombinedInOut;
		bool FLastOutput;
		bool FRawInput;

	public:
		OpenWire::SinkPin	DigitalInputPin;
		OpenWire::SinkPin	AnalogInputPin;

	protected:
        void PinDirectionsInit()
        {
			if( FRawInput )
				return;

            if( FIsOutput )
                pinMode( FPinNumber, OUTPUT );

            else
            {
                if( FIsPullUp )
                    pinMode( FPinNumber, INPUT_PULLUP );

                else
                    pinMode( FPinNumber, INPUT );
            }
        }

	protected:
		virtual void SystemInit()
		{
			inherited::SystemInit();

            PinDirectionsInit();
		}

		virtual void SystemStart()
		{
			if( FRawInput )
				return;

			FLastOutput = ( digitalRead( FPinNumber ) == HIGH );
	    	OutputPin.Notify( &FLastOutput );
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
        {
            if( !FIsOutput )
            {
    			bool AValue = ( digitalRead( FPinNumber ) == HIGH );
				if( AValue == FLastOutput )
					return;

				FLastOutput = AValue;
//	Serial.println( AData.Value );
	    		OutputPin.Notify( &AValue );
            }
        }

	protected:
		void DoAnalogReceive( void *_Data )
        {
            if( FIsOutput && FIsAnalog )
            {
                float AValue = *(float*)_Data;
                analogWrite( FPinNumber, ( AValue * 255 ) + 0.5 );
            }
        }

/*
		void DoDigitalOutputReceive( void *_Data )
		{
			*((int*)_Data) = FPinNumber;
		}
*/
		void DoDigitalReceive( void *_Data )
		{
			if( FRawInput )
			{
				*((int*)_Data) = FPinNumber;
				return;
			}

            if( ( FCombinedInOut || FIsOutput ) && (! FIsAnalog ))
            {
				bool AValue = *(bool *)_Data;

				if( AValue )
				  if( FCombinedInOut )
					  pinMode( FPinNumber, OUTPUT );

                if( AValue )
                    digitalWrite( FPinNumber, HIGH );

                else
                    digitalWrite( FPinNumber, LOW );

				if( ! AValue )
					if( FCombinedInOut )
						if( FIsPullUp )
							pinMode( FPinNumber, INPUT_PULLUP );

						else
							pinMode( FPinNumber, INPUT );

            }
		}

	public:
        void SetIsOutput( bool AValue )
        {
            if( FIsOutput == AValue )
                return;

            FIsOutput = AValue;
            PinDirectionsInit();
        }

        void SetIsPullUp( bool AValue )
        {
            if( FIsPullUp == AValue )
                return;

            FIsPullUp = AValue;
            PinDirectionsInit();
        }

        void SetIsAnalog( bool AValue )
        {
            if( FIsAnalog == AValue )
                return;

            FIsAnalog = AValue;
            PinDirectionsInit();
        }

	public:
		ArduinoDigitalChannel( int APinNumber, bool AIsOutput, bool AIsPullUp, bool AIsAnalog, bool ACombinedInOut, bool ARawInput ) :
			FPinNumber( APinNumber ),
            FIsOutput( AIsOutput ),
            FIsPullUp( AIsPullUp ),
            FIsAnalog( AIsAnalog ),
			FRawInput( ARawInput ),
			FCombinedInOut( ACombinedInOut )

		{
			DigitalInputPin.OnReceiveObject = this;
			DigitalInputPin.OnReceive = (OpenWire::TOnPinReceive)&ArduinoDigitalChannel::DoDigitalReceive;

			AnalogInputPin.OnReceiveObject = this;
			AnalogInputPin.OnReceive = (OpenWire::TOnPinReceive)&ArduinoDigitalChannel::DoAnalogReceive;

//			OutputPin.OnReceiveObject = this;
//			OutputPin.OnReceive = (OpenWire::TOnPinReceive)&ArduinoDigitalChannel::DoDigitalOutputReceive;
		}
    };
//---------------------------------------------------------------------------
    class ArduinoAnalogOutputChannel : public CommonSink
    {
		typedef Mitov::CommonSink	inherited;

	protected:
		int  FPinNumber;

	protected:
		virtual void DoReceive( void *_Data )
		{
            float AValue = *(float*)_Data;
            analogWrite( FPinNumber, ( AValue * 255 ) + 0.5 );
		}

	public:
		ArduinoAnalogOutputChannel( int APinNumber ) : 
			FPinNumber( APinNumber )
		{
		}

	};
//---------------------------------------------------------------------------
	const	long CSerialInits[] = 
	{
		SERIAL_5N1,
		SERIAL_6N1,
		SERIAL_7N1,
		SERIAL_8N1,
		SERIAL_5N2,
		SERIAL_6N2,
		SERIAL_7N2,
		SERIAL_8N2,
		SERIAL_5E1,
		SERIAL_6E1,
		SERIAL_7E1,
		SERIAL_8E1,
		SERIAL_5E2,
		SERIAL_6E2,
		SERIAL_7E2,
		SERIAL_8E2,
		SERIAL_5O1,
		SERIAL_6O1,
		SERIAL_7O1,
		SERIAL_8O1,
		SERIAL_5O2,
		SERIAL_6O2,
		SERIAL_7O2,
		SERIAL_8O2
	};
//---------------------------------------------------------------------------
	enum TArduinoSerialParity { spNone, spEven, slOdd };
//---------------------------------------------------------------------------
#ifdef HAVE_CDCSERIAL
	#define T_SERIAL_TYPE Serial_
#else
  #ifdef VISUINO_ARDUINO_DUE
	#define T_SERIAL_TYPE UARTClass
  #else
	#define T_SERIAL_TYPE HardwareSerial
  #endif
#endif
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL> class SerialPort : public OpenWire::Component
	{
		typedef OpenWire::Component	inherited;

	public:
		bool					Enabled;
		TArduinoSerialParity	Parity;
		unsigned long			Speed;
		unsigned int			StopBits;
		unsigned int			DataBits;

	public:
		Stream &GetStream() { return *T_SERIAL; }

	protected:
		virtual void SystemInit()
		{
			if( Enabled )
				StartPort();

			inherited::SystemInit();
		}

	protected:
		void StartPort()
		{
#ifdef VISUINO_ARDUINO_DUE
			T_SERIAL->begin( Speed );
#else
			int AIndex = ((int)Parity) * 2 * 4 + ( StopBits - 1 ) + ( DataBits - 5);
			T_SERIAL->begin( Speed, CSerialInits[ AIndex ] );
#endif
		}

        void RestartPort()
		{
			if( ! Enabled )
				return;

			T_SERIAL->end();
			StartPort();
		}

	public:
		void SetEnabled( bool AValue )
		{
            if( Enabled == AValue )
                return;

            Enabled = AValue;
			if( Enabled )
				StartPort();

			else
				T_SERIAL->end();
		}

		void SetParity( TArduinoSerialParity AValue )
		{
            if( Parity == AValue )
                return;

            Parity = AValue;
            RestartPort();
		}

		void SetSpeed( unsigned int AValue )
		{
            if( Speed == AValue )
                return;

            Speed = AValue;
            RestartPort();
		}

		void SetStopBits( unsigned int AValue )
		{
            if( StopBits == AValue )
                return;

            StopBits = AValue;
            RestartPort();
		}

		void SetDataBits( unsigned int AValue )
		{
            if( DataBits == AValue )
                return;

            DataBits = AValue;
            RestartPort();
		}

	public:
		SerialPort() :
			Enabled( true ),
			Parity( spNone ),
			Speed( 9600 ),
			StopBits( 1 ),
			DataBits( 8 )
		{
		}
	};
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL, SerialPort<T_SERIAL> *T_SERIAL_INSTANCE, typename T> class ArduinoSerialInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_SERIAL_INSTANCE->Enabled )
				T_SERIAL->println( *(T*)_Data );
		}
	};
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL, SerialPort<T_SERIAL> *T_SERIAL_INSTANCE> class ArduinoSerialBinaryInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_SERIAL_INSTANCE->Enabled )
				T_SERIAL->write( *(unsigned char*)_Data );
		}
	};
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL, SerialPort<T_SERIAL> *T_SERIAL_INSTANCE> class ArduinoSerialStringInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_SERIAL_INSTANCE->Enabled )
				T_SERIAL->println( (char*)_Data );
		}
	};
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL, SerialPort<T_SERIAL> *T_SERIAL_INSTANCE> class ArduinoSerialColorInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			if( T_SERIAL_INSTANCE->Enabled )
			{
				T_SERIAL->print( "(" );
				T_SERIAL->print( (int)((TColor *)_Data)->Red );
				T_SERIAL->print( "," );
				T_SERIAL->print( (int)((TColor *)_Data)->Green );
				T_SERIAL->print( "," );
				T_SERIAL->print( (int)((TColor *)_Data)->Blue );
				T_SERIAL->println( ")" );
			}
		}
	};
//---------------------------------------------------------------------------
/*
	class DirectSerialAccess
	{
	public:
		virtual Stream &GetStream() = 0;
	};
*/
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL, SerialPort<T_SERIAL> *T_SERIAL_INSTANCE> class ArduinoDirectSerialInput : public Mitov::CommonSink //, public DirectSerialAccess
	{
		typedef Mitov::CommonSink	inherited;

	public:
		virtual Stream &GetStream()
		{
			return T_SERIAL_INSTANCE->GetStream();
		}

	protected:
		virtual void DoReceive( void *_Data )
		{
//			if( T_SERIAL_INSTANCE->Enabled )
//				T_SERIAL->write( *(unsigned char*)_Data );
		}
	};
//---------------------------------------------------------------------------
	template<T_SERIAL_TYPE *T_SERIAL, SerialPort<T_SERIAL> *T_SERIAL_INSTANCE> class ArduinoSerialOutput : public Mitov::CommonSource
	{
	protected:
		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( ! T_SERIAL_INSTANCE->Enabled )
				return;

//			if( Serial.available() )
			{
				int AData = T_SERIAL->read();
				if( AData >= 0 )
				{
					unsigned char AByte = AData;
					OutputPin.Notify( &AByte );
				}

			}
		}

	};
//---------------------------------------------------------------------------
	class ArduinoI2C_Command
	{
	public:
		virtual void Execute() = 0;

	};
//---------------------------------------------------------------------------
	class ArduinoI2CInput : public Mitov::CommonSink
	{
		typedef Mitov::CommonSink	inherited;

	protected:
		virtual void DoReceive( void *_Data )
		{
			((ArduinoI2C_Command*)_Data )->Execute();
//			Wire.println( *(ArduinoI2C_Command*)_Data );
		}
	};

} // Mitov

#endif

