PwmGen library
======================

**PwmGen** is pseudo-asynchronous library designed to generate RC pulse signals. RC pulse signals are intended to command servos, **E**lectronic **S**peed **C**ontrollers (**ESC**), Brushless Controllers and any devices expecting such a command signal.
Static and dynamic object allocation are both supported.
Dynamic deallocation is supported for **PwmGen** instance allocated dynamically.

Some examples of use cases:
-------------------------
* **Servo/ESC/Brushless Controller tester**
* **Servo sequencer** (look at RcSeq library which uses _PwmGen_)
* **Robot wheels using modified Servo to support 360° rotation**
* **RC pulse stretcher** (in conjunction with **SoftRcPulseIn** library)

Supported Arduinos:
------------------
* **RP2040**
* **Teensy**
And more I suppose

Tip and Tricks:
--------------
Develop your project on an arduino UNO or MEGA, and then shrink it by loading the sketch in an ATtiny or Digispark (pro).

API/methods:
-----------
* The **PwmGen** library uses the same API as the regular **SoftwareServo** library:
	* **attach()**: attaches the PwmGen object to a pin
	* **attached()**: returns if attached
	* **detach()**: detaches the PwmGen object from a pin
	* **write()**: write angle in degrees
	* **read()**: read angle in degrees
	* **setMinimumPulse()**: set minimum pulse in µs
	* **setMaximumPulse()**: set maximum pulse in µs
	* **refresh()**: generate the PwmGen pulse(s) when specified

* Two additional methods allow using µs rather than angle in ° :
	* **write_us()**: write the pulse width in µs
	* **read_us()**: returns the pulse width in µs

* Extended methods to manage dynamically the instance creation/destruction
	* **createInstance()**: returns the id of the freshly created instance
	* **createdInstanceNb()**: returns the number of created instances (including static instances)
	* **PwmGenById()**: returns a pointer on the corresponding PwmGen object
	* **getIdByPin()**: returns the Id of the instance attached to a pin
	* **destroyInstance()**: destroys a dynamically created instance (instance statically created cannot be destroyed)

* Constants for version management:
	* **SOFT_RC_PULSE_OUT_VERSION**: returns the library version
	* **SOFT_RC_PULSE_OUT_REVISION**: returns the library revision

* Synchronization:
	* By giving **_1_** or **_true_** as optional argument for the **PwmGen::refresh()** method, the pulses are refreshed immediately (without waiting for the usual 20ms).

	* the **PwmGen::refresh()** method returns **_1_** or **_true_** when the pulses have been refreshed. Testing this return value provides a 20ms timer.

Design considerations:
---------------------
The **PwmGen** library relies on a 8 bit timer. This allows using it even on little MCU (such as ATtiny85) which do not have any 16 bit timer.

Whereas a 8 bit timer is used for pulse generation, the jitter is limited by using the technic of anticipated interrupt masking.

Interrups are only masked during rising and falling edges of the pulse signals.

Statically + dynamically created instances are limited to 15.

CAUTION:
-------
The end user shall also use asynchronous programmation method in the loop() function (not too long blocking functions such as delay(1000): the **PwmGen::refresh()** method shall be called at least every 50ms).

Contact
-------

If you have some ideas of enhancement, please contact me by clicking on: [RC Navy](http://p.loussouarn.free.fr/contact.html).

