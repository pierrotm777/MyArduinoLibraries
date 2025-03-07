#ifndef _SK33RcvData_H_
#define _SK33RcvData_H_

#include "Arduino.h"

class SK33RcvData
{
	public:
		SK33RcvData();
		void Changed(void);
		bool IsChanged(void);
		void Reset(void);
		void Print(void);
		void SetValue0(uint8_t);
		uint8_t GetValue0(void);
		void SetValue1(uint8_t);
		uint8_t GetValue1(void);
		void SetValue2(uint8_t);
		uint8_t GetValue2(void);
		void SetValue3(uint8_t);
		uint8_t GetValue3(void);
		
		
	private:
		static uint8_t _uiValueB0;
		static uint8_t _uiValueB1;
		static uint8_t _uiValueB2;
		static uint8_t _uiValueB3;
		static bool _bValuesUpdated;
};

#endif //_SK33RcvData_H_