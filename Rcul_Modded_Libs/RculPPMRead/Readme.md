RculPPMReader library
=====================

**RculPPMReader** is an interrupt-driven **RC CPPM reader** library relying on **TinyPinChange** library: this means the CPPM frame input pin shall support pin change interrupt.

This CPPM reader can extract up to 9 RC channels and supports positive and negative CPPM modulation.

Some examples of use cases:
-------------------------
* **Standalone RC CPPM reader**
* **Channel substitution in an existing CPPM frame** (in conjunction with  **TinyCppmGen** library)
* **Channels addition to an existing CPPM frame**: eg. read 4 channels and generate 6 channels (in conjunction with  **TinyCppmGen** library)
* **Digital data transmission over CPPM** (in conjunction with  **TinyCppmGen** library)

Supported Arduinos:
------------------
* **RP2040**
* **Teensy 4.0, LC**
* **ESP32**

Tip and Tricks:
--------------
Develop your project on an arduino UNO, and then shrink it by loading the sketch in an ATtiny or Digispark (pro).

API/methods:
-----------
* **RculPPMReader**: The object constructor

* **uint8_t attach(uint8_t _CppmInputPin_)**: attach the RculPPMReader object to a pin.
With:
	* **_CppmInputPin_**: The CPPM input pin. The modulation can be _Positive_ or _Negative_: it doesn't matter, since sampling on rising edges or on falling edges is equivalent.
	* Returns 1 in case of success, and 0 if the _CppmInputPin_ doesn't support pin change interrupt (unusable pin).

* **uint8_t detectedChannelNb()**: returns the number of detected RC channels in the CPPM frame.
* **uint16_t width_us(uint8_t _Ch_)**:
With:
	* **_Ch_**: The Channel (from 1 to Detected Channel Number).
	* Returns the requested channel pulse width in µs

* **uint16_t cppmPeriod_us()**:
	* Returns the measured CPPM period in µs

* **uint8_t isSynchro()**:
	* CPPM Synchronization indicator: indicates that the largest pulse value (Synchro) has just been received. This is a "clear on read" fonction (no need to clear explicitely the indicator).

* **suspend()**: supends the CPPM acquisition. This can be useful whilst displaying results through a software serial port which disables interrupts during character transmission.

* **resume()**: resumes the CPPM acquisition.



