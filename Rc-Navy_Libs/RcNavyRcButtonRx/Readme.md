RcButtonRx library
==================

**RcButtonRx** is a library designed to read RC pulse signal to make actions from a keyboard (push-buttons + resistors) connected to a free channel of an RC transmitter. This library manages the mandatory calibration phase (an hardware or software serial interface is needed). The action associated to each push-button can be set in pulse mode (sometime called memory mode). This **RcButtonRx** library is intended to facilitate the design of a decoder placed at RC receiver side. With this library, the exploitation of the commands from the push-buttons are greatly facilitated. In case of lost signal, all the commands are set to 0 after one second (Failsafe).

The **RcButtonRx** library the following RC signals:
---------------------------------------------------
* PWM
* CPPM
* SBUS
* IBUS
* SRXL
* SUMD
* JETI

Some examples of keyboards:
-------------------------
* **Custom keyboard (DIY)** made with push-buttons and resistors
* **Kingpad from Pistenking**
* **Steuerpad from Kraftwerk**

Supported Arduinos:
------------------
* **ATmega328 (UNO)**
* **ATmega2560 (MEGA)**
* **ATtiny84 (Standalone)**
* **ATtiny85 (Standalone or Digispark)**
* **ATtiny167 (Digispark pro)**
* **ATmega32U4 (Leonardo, Micro, Pro Micro)**
* **ESP8266**

Tip and Tricks:
--------------
Develop your project on an arduino UNO or MEGA, and then shrink it by loading the sketch in an ATtiny or Digispark (pro).


API/methods:
-----------
* **begin()**: initializes the RcButtonRx object.

* **getStoredEepromBytes()**: returns the number of bytes used in EEPROM for a given number of push-buttons.

* **setPulseMap()**: defines the mode (Normal or Pulse) for the action associated to each push-button in one shot.

* **setPulseMode()**: defines the mode (Normal or Pulse) for the action associated to one specific push-button.

* **isPulseMode()**: returns if the action associated to one specific push-button is Pulse mode or not.

* **enterInCalibrationMode()**: switches to the calibration mode.

* **process()**: shall be called in the loop and returns, in a 16 bit variable, the current status of the actions associated to all the push-buttons.

* **displayButtonPulseWidth()**: displays in the serial console the EEPROM pulse width values associated to all the push-buttons.

* **Constants for version management**:
	* **RC_BUTTON_RX_VERSION**: returns the library version
	* **RC_BUTTON_RX_REVISION**: returns the library revision

Design considerations:
---------------------
The **RcButtonRx** library relies on the RC libraries implementing the RCUL abstraction interface class developped by the autor. RCUL stands for: RC Universal Link. This abstracton allows using any kind of RC interface for **RcButtonRx**: PWM, CPPM and all the serial protocols (SBUS, IBUS, SRXL, SUMD, JETI, etc...).

List of RC libraries implementing the RCUL abstraction interface class:
-----------
* SoftRcPulseIn: A software library reading a PWM signal
* HwRcPulseIn: A library reading a PWM frame using ICP capabilities
* Cppm: A library reading a CPPM frame using ICP capabilities
* TinyCppmReader: A software library reading a CPPM frame
* SoftSBusRx: A software library supporting SBUS without the need of an hardware inverter
* RcBusRx: A library supporting the following serial protocols (SBUS, SRXL, SUMD, IBUS and JETI)

Contact
-------

If you have some ideas of enhancement, please contact me by clicking on: [RC Navy](http://p.loussouarn.free.fr/contact.html).

