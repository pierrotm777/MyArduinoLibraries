#include "SK33RcvData.h"

uint8_t SK33RcvData::_uiValueB0 = 0;
uint8_t SK33RcvData::_uiValueB1 = 0;
uint8_t SK33RcvData::_uiValueB2 = 0;
uint8_t SK33RcvData::_uiValueB3 = 0;
bool SK33RcvData::_bValuesUpdated = false;

//constructor
SK33RcvData::SK33RcvData() { }

void SK33RcvData::Changed(void) { _bValuesUpdated = true; }
bool SK33RcvData::IsChanged(void) { return _bValuesUpdated; }
void SK33RcvData::Reset(void) { _bValuesUpdated = false; }

void SK33RcvData::Print(void) {
	Serial.println("Latest Values");
	Serial.print("_bValuesUpdated: ");
	Serial.println(_bValuesUpdated);
	Serial.print("_uiValueB0: ");
	Serial.println(_uiValueB0);
	Serial.print("_uiValueB1: ");
	Serial.println(_uiValueB1);
	Serial.print("_uiValueB2: ");
	Serial.println(_uiValueB2);
	Serial.print("_uiValueB3: ");
	Serial.println(_uiValueB3);
}

void SK33RcvData::SetValue0(uint8_t x) { _uiValueB0 = x; }
uint8_t SK33RcvData::GetValue0(void) { return _uiValueB0; }
void SK33RcvData::SetValue1(uint8_t x) { _uiValueB1 = x; }
uint8_t SK33RcvData::GetValue1(void) { return _uiValueB1; }
void SK33RcvData::SetValue2(uint8_t x) { _uiValueB2 = x; }
uint8_t SK33RcvData::GetValue2(void) { return _uiValueB2; }
void SK33RcvData::SetValue3(uint8_t x) { _uiValueB3 = x; }
uint8_t SK33RcvData::GetValue3(void) { return _uiValueB3; }