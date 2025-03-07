////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_MATH_h
#define _MITOV_MATH_h

#include <Mitov.h>

namespace Mitov
{
//---------------------------------------------------------------------------
    class CommonValueMathFilter : public CommonTypedFilter<float>
    {
    public:
        float   Value;

    public:
        CommonValueMathFilter() : Value( 0 ) {}

    };
//---------------------------------------------------------------------------
    class AddValue : public CommonValueMathFilter
	{
		typedef CommonValueMathFilter inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return AValue + Value;
        }

	};
//---------------------------------------------------------------------------
	class SubtractValue : public CommonValueMathFilter
	{
		typedef CommonValueMathFilter inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return AValue - Value;
        }

	};
//---------------------------------------------------------------------------
	class SubtractFromValue : public CommonValueMathFilter
	{
		typedef CommonValueMathFilter inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return Value - AValue;
        }

	};
//---------------------------------------------------------------------------
    class MultiplyByValue : public CommonValueMathFilter
	{
		typedef CommonValueMathFilter inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return AValue * Value;
        }

	};
//---------------------------------------------------------------------------
    class DivideByValue : public CommonValueMathFilter
	{
		typedef CommonValueMathFilter inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return AValue / Value;
        }

	public:
		DivideByValue()
		{
			Value = 1.0;
		}
	};
//---------------------------------------------------------------------------
	class DivideValue : public CommonValueMathFilter
	{
		typedef CommonValueMathFilter inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return Value / AValue;
        }

	};
//---------------------------------------------------------------------------
	class Inverse : public CommonTypedFilter<float>
	{
		typedef CommonTypedFilter<float> inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return (Enabled) ? -AValue : AValue;
        }

	};
//---------------------------------------------------------------------------
	class Abs : public CommonTypedFilter<float>
	{
		typedef CommonTypedFilter<float> inherited;

	protected:
        virtual float FilterValue( float AValue )
        {
            return (Enabled) ? abs( AValue ) : AValue;
        }

	};
//---------------------------------------------------------------------------
	class MinLimit : public CommonValueMathFilter
	{
	protected:
        virtual float FilterValue( float AValue )
        {
			return ( Enabled && ( AValue < Value )) ? Value : AValue;
        }
	};
//---------------------------------------------------------------------------
	class MaxLimit : public CommonValueMathFilter
	{
	protected:
        virtual float FilterValue( float AValue )
        {
			return ( Enabled && ( AValue > Value )) ? Value : AValue;
        }

	public:
		MaxLimit()
		{
			Value = 1.0;
		}
	};
//---------------------------------------------------------------------------
	class Limit : public CommonTypedFilter<float>
	{
	public:
		float	Min;
		float	Max;

	protected:
        virtual float FilterValue( float AValue )
        {
			if( !Enabled )
				return AValue;

			return ( AValue > Max ) ? 
					Max : 
					( AValue < Min ) ? Min : AValue;
        }

	public:
		Limit()
		{
			Min = 0.0;
			Max = 1.0;
		}
	};
//---------------------------------------------------------------------------
	enum CompareValueType { ctEqual, ctNotEqual, ctBigger, ctSmaller, ctBiggerOrEqual, ctSmallerOrEqual };
//---------------------------------------------------------------------------
	class CompareValue : public CommonFilter
	{
		typedef CommonFilter inherited;

    public:
        float				Value;
		CompareValueType	CompareType;

	protected:
		virtual void DoReceive( void *_Data )
		{
			float AValue = *(float*)_Data;

			bool ABoolValue;
			switch( CompareType )
			{
			case ctEqual:
				ABoolValue = ( AValue == Value );
				break;

			case ctNotEqual:
				ABoolValue = ( AValue != Value );
				break;

			case ctBigger:
				ABoolValue = ( AValue > Value );
				break;

			case ctSmaller:
				ABoolValue = ( AValue < Value );
				break;

			case ctBiggerOrEqual:
				ABoolValue = ( AValue >= Value );
				break;

			case ctSmallerOrEqual:
				ABoolValue = ( AValue <= Value );
				break;

			}

			OutputPin.Notify( &ABoolValue );
		}

	public:
		CompareValue() : 
			Value( 0 ),
			CompareType( ctEqual )
		{
		}

	};
//---------------------------------------------------------------------------
	class AveragePeriod : public CommonFilter
	{
	public:
		unsigned long	Period;

	protected:
		float FSum;
		unsigned long	FCount;
		unsigned long	FLastTime;

	protected:
		virtual void DoReceive( void *_Data )
		{
			FSum += *(float *)_Data;
			++FCount;
		}

		virtual void SystemLoopBegin( unsigned long currentMicros )
		{
			if( currentMicros - FLastTime < Period )
				return;

			if( ! FCount )
				return;

			float AValue = FSum / FCount;

			FSum = 0.0;
			FCount = 0;
			FLastTime = currentMicros;

			OutputPin.Notify( &AValue );
		}

	public:
		AveragePeriod() : 
			FSum( 0 ),
			FCount( 0 ),
			FLastTime( 0 ),
			Period( 1000 )
		{
		}
	};
//---------------------------------------------------------------------------
	class CommonMathMultiInput : public CommonEnableSource
	{
		typedef CommonEnableSource inherited;

	protected:
		class ValueSinkPin : public OpenWire::SinkPin
		{
			typedef OpenWire::SinkPin inherited;

		public:
			float	Value;

		public:
			virtual void Receive( void *_Data )
			{
				Value = *(float*)_Data;
				inherited::Receive( _Data );
			}
		};

	public:
		Mitov::SimpleList<ValueSinkPin>	InputPins;

	protected:
		virtual void DoReceive( void *_Data ) = 0;

	protected:
		virtual void SystemInit()
		{
			for( int i = 0; i < InputPins.size(); ++i )
			{
				InputPins[ i ].OnReceiveObject = this;
				InputPins[ i ].OnReceive = (OpenWire::TOnPinReceive)&CommonMathMultiInput::DoReceive;
			}

			inherited::SystemInit();
		}
	};
//---------------------------------------------------------------------------
	class Add : public CommonMathMultiInput
	{
	protected:
		virtual void DoReceive( void *_Data )
		{
			float AValue = 0;
			for( int i = 0; i < InputPins.size(); ++i )
				AValue += InputPins[ i ].Value;

			OutputPin.Notify( &AValue );
		}
	};
//---------------------------------------------------------------------------
	class Multiply : public CommonMathMultiInput
	{
	protected:
		virtual void DoReceive( void *_Data )
		{
			float AValue = 0;
			for( int i = 0; i < InputPins.size(); ++i )
				AValue *= InputPins[ i ].Value;

			OutputPin.Notify( &AValue );
		}
	};
//---------------------------------------------------------------------------
}

#endif
