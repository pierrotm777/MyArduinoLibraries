////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//     This software is supplied under the terms of a license agreement or    //
//     nondisclosure agreement with Mitov Software and may not be copied      //
//     or disclosed except in accordance with the terms of that agreement.    //
//         Copyright(c) 2002-2015 Mitov Software. All Rights Reserved.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _MITOV_SNAPSHOT_h
#define _MITOV_SNAPSHOT_h

#include <Mitov.h>

namespace Mitov
{
//---------------------------------------------------------------------------
	class CommonSnapshot : public CommonEnableFilter
	{
		typedef CommonFilter inherited;

	public:
		OpenWire::SinkPin	SnapshotInputPin;

	protected:
		virtual void TakeSnapshot() = 0;

	protected:
		void DoReceiveSnapshot( void *_Data )
		{
			if( Enabled )
				TakeSnapshot();
		}

	public:
		CommonSnapshot()
		{
			SnapshotInputPin.OnReceiveObject = this;
			SnapshotInputPin.OnReceive = (OpenWire::TOnPinReceive)&CommonSnapshot::DoReceiveSnapshot;
		}
	};
//---------------------------------------------------------------------------
	template<typename T> class Snapshot : public CommonSnapshot
	{
		typedef CommonSnapshot inherited;

	protected:
		T FData;

	protected:
		virtual void DoReceive( void *_Data )
		{
			FData = *(T*)_Data;
		}

	protected:
		virtual void TakeSnapshot()
		{
			OutputPin.Notify( &FData );
		}

	public:
		Snapshot( T AData ) : FData( AData ) {}
	};
//---------------------------------------------------------------------------
	class TextSnapshot : public CommonSnapshot
	{
		typedef CommonSnapshot inherited;

	protected:
		String FData;

	protected:
		virtual void DoReceive( void *_Data )
		{
			FData = (char*)_Data;
		}

	protected:
		virtual void TakeSnapshot()
		{
			const char *AData = FData.c_str();
			OutputPin.Notify( (void *)AData );
		}
	};
//---------------------------------------------------------------------------
}

#endif
