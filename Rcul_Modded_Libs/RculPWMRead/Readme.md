PwmRead library
======================
/*SoftRcPulseIn Fork for RP2040*/
**PwmRead** is an asynchronous library designed to read RC pulse signals. It is a non-blocking version of arduino **pulseIn()** function.

Some examples of use cases:
-------------------------
* **RC Servo/ESC/Brushless Controller**
* **Multi-switch (RC Channel to digital outputs converter)** (look at **RcSeq** library)
* **Servo sequencer** (look at **RcSeq** library which uses **SoftRcPulseOut** library)
* **RC Robot using wheels with modified Servo to support 360° rotation**
* **RC pulse stretcher** (in conjunction with **SoftRcPulseOut** library)

Supported:
------------------
* **RP2040 **

Tip and Tricks:
--------------
Develop your project on an arduino UNO or MEGA, and then shrink it by loading the sketch in an ATtiny or Digispark (pro).


Object constructor:
------------------
3 ways to declare a PwmRead object:

* 1) PwmRead MyRcSignal; //Positive RC pulse expected (default behaviour)
* 2) PwmRead MyRcSignal(false); //Positive RC pulse expected (equivalent as the above)
* 3) PwmRead MyRcSignal(true); //Negative RC pulse expected

API/methods:
-----------
* attach()
* available()
* width_us()
* timeout()

* Constants for version management:
	* **SOFT_RC_PULSE_IN_VERSION**: returns the library version
	* **SOFT_RC_PULSE_IN_REVISION**: returns the library revision

Design considerations:
---------------------
The **PwmRead** library isn't relied to **TinyPinChange** library.

