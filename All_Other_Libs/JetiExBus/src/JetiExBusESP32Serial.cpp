/*
Jeti Sensor EX Bus Telemetry C++ Library

JetiExBusESP32Serial - serial implementation for
                       half duplex operation with ESP32
---------------------------------------------------------------------

Copyright (C) 2018 Bernd Wokoeck

Version history:
0.90   02/13/2018  created
0.93   02/16/2018  ESP32 uart initialization changed

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

**************************************************************/

#if defined (ESP32)

#include "JetiExBusESP32Serial.h"

ESP32HardwareSerial Esp32Serial0(0);
ESP32HardwareSerial Esp32Serial1(1);
ESP32HardwareSerial Esp32Serial2(2); // default port number 2

static ESP32HardwareSerial * Esp32SerialActive = &Esp32Serial2;

hw_timer_t *  _timer = NULL;
portMUX_TYPE  _timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool _bTimerRunning = false;

void IRAM_ATTR _onTimer() {
	portENTER_CRITICAL_ISR(&_timerMux);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
	// ESP32 Arduino core 3.x: timerAlarmDisable() was removed.
	// Stop the one-shot timer after the TX window.
	timerStop(_timer);
#else
	timerAlarmDisable(_timer);
#endif

	_bTimerRunning = false;
	if (Esp32SerialActive) Esp32SerialActive->uartDetachTx();
	// digitalWrite(15, false);
	portEXIT_CRITICAL_ISR(&_timerMux);
}

JetiExBusSerial * JetiExBusSerial::CreatePort(int comPort)
{
	// pinMode(15, OUTPUT);
	// digitalWrite(15, false);

	return new JetiExBusESP32Serial(comPort);
}

JetiExBusESP32Serial::JetiExBusESP32Serial(int comPort) : m_pSerial(0)
{
	if (comPort == 0)      m_pSerial = &Esp32Serial0;
	else if (comPort == 1) m_pSerial = &Esp32Serial1;
	else                  m_pSerial = &Esp32Serial2;

	Esp32SerialActive = m_pSerial;
}

size_t JetiExBusESP32Serial::write(const uint8_t *buffer, size_t size)
{
	if( !_bTimerRunning )
	{
		portENTER_CRITICAL(&_timerMux);
		Esp32SerialActive = m_pSerial;
		// https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
		/*
		 * ESP32 Arduino core 3.x changed the hardware timer API:
		 *   core 2.x: timerBegin(timer_id, divider, countUp)
		 *   core 3.x: timerBegin(frequency)
		 *
		 * Original timing: prescaler 160 on 80 MHz => 500 kHz timer tick = 2 us.
		 * Alarm value 2000 => 2000 * 2 us = 4 ms TX window.
		 */
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
		if (_timer == NULL)
		{
			_timer = timerBegin(500000);              // 500 kHz timer tick = 2 us
			timerAttachInterrupt(_timer, &_onTimer);
		}
		timerWrite(_timer, 0);
		timerAlarm(_timer, 2000L, false, 0);        // 2000 * 2 us = 4 ms, one-shot
#else
		_timer = timerBegin(0, 160, true);          // timer 0, prescaler 160 --> 80MHz/160 = 0.5 MHz = 2us, count up
		timerAttachInterrupt(_timer, &_onTimer, true); // attach handler, trigger on edge
		timerAlarmWrite(_timer, 2000L, true);       // 2000 * 2us = 4ms
		timerAlarmEnable(_timer);                   // start
#endif
		_bTimerRunning = true;
		m_pSerial->uartAttachTx();
		// digitalWrite(15, true);
		portEXIT_CRITICAL(&_timerMux);
	}
	return m_pSerial->write(buffer, size);
}


#endif // ESP32