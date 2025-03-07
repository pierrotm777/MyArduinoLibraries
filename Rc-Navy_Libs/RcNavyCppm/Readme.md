Cppm library
============

**Cppm** is an interrupt-driven **RC CPPM generator** and **RC CPPM reader** library using a 16 bit Timer Output Compare Interrupt and 16 bit Input Capture Timer. As this library uses hardware resources, the timing is very accurate but the CPPM output pin and CPPM input pin are imposed (cf. _Design Considerations_ below).

Both, **CPPM generator** and **CPPM receiver** can support up to 12 RC channels and supports positive and negative CPPM modulation. The CPPM frame period is constant (configurable from 10 to 40ms, default=20 ms) regardless of the channel pulse widths.

Some examples of use cases:
-------------------------
* **Standalone RC CPPM generator**
* **Channel substitution and/or addition in an existing CPPM frame**
* **Digital data transmission over CPPM**

Supported Arduinos:
------------------
* **ATmega328P (UNO)**

API/methods:
-----------
* **CppmGen.begin(uint8_t _PpmModu_, uint8_t _ChNb_, uint16_t _PpmPeriod_us = DEFAULT_PPM_PERIOD_US_, uint16_t _PpmHeader_us = DEFAULT_PPM_HEADER_US_)**
With:
	* **_PpmModu_**: **CPPM_GEN_POS_MOD** or **CPPM_GEN_NEG_MOD** for respectiveley positive and negative PPM modulation
	* **_ChNb_**: The number of RC channel to transport in the CPPM frame (1 to 12)
	* **PpmPeriod_us**: CPPM period in µs (from 10000 to 40000 µs, 20000 µs if 3rd argument absent)
 
* **CppmGen.width_us(uint8_t _Ch_, uint16_t _Width_us_)**
With:
	* **_Ch_**: the RC channel (1 to _ChNb_)
	* **_Width_us_**: the pulse width in µs

* **uint8_t CppmGen.isSynchro()**:
	* CPPM Synchronization indicator: indicates that the pulse values have just been sent for the current CPPM frame generation and gives synchronization for loading next pulse widths. This allows to pass digital information over CPPM (one different pulse width per CPPM frame). This is a "clear on read" function (no need to clear explicitely the indicator).

* **void CppmGen.suspend()**:
	* Suspends the CPPM frame generation.

* **void CppmGen.resume()**:
	* Resumes the CPPM frame generation.

TO DO:
-----
Add doc about CppmReader which is part of the same library.

Design considerations:
---------------------
As this library relies on Timer Output Compare capabilities, the CPPM output pin is imposed by the hardware and is target dependent.

* **ATmega328P** (Arduino UNO):
* CPPM output pin: TIMER(1), CHANNEL(A) -> OC1A -> PB1 -> Pin#9
* CPPM input pin:  TIMER(1) -> ICP1 -> PB0 -> Pin#8

Contact
-------

If you have some ideas of enhancement, please contact me by clicking on: [RC Navy](http://p.loussouarn.free.fr/contact.html).

