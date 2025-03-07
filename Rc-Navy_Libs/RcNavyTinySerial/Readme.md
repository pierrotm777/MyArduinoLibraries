TinySerial library
==================

The **TinySerial** library is exactly the same as the **SoftwareSerial** library but is used with the **TinyPinChange** library which allows to share the "Pin Change Interrupt" Vector. 

However, **TinySerial** supports only a **single** instance of serial port, but it supports all the combinations of the following data rate, data bit, parity and stop bit:

- **data rate** from 9600 to 57600 bauds
- **data bit** from 5 to 8
- **parity** (None, Odd, Even)
- **stop bit** from 1 to 2

**TinySerial** also supports:

- signal inversion
- single-wire (half-duplex with a single pin)

The size of the libray is around **400** bytes.

Some limitations:

- lowest data rate is 9600 (to avoid using 16 bit integer for bit delay))
- highest data rate is 57600 (the highest achievable data rate)

To justify these limitations, even if it can work on any arduino, **TinySerial** is designed for very constrained target (eg: ATtiny85).

Additionally, for small devices such as **ATtiny85** (Digispark), it's possible to declare **the same pin for TX and RX**.
Data direction is set by using the new **txMode()** and **rxMode()** methods.

Some examples of use cases:
-------------------------
* **half-duplex bi-directional serial port on a single wire for debuging purpose**
* **half-duplex serial port to interface with Bluetooth module**
* **half-duplex serial port to interconnect an arduino with another one**

Supported Arduinos:
------------------
* **ATmega368 (UNO)**
* **ATmega2560 (MEGA)**
* **ATtiny84 (Standalone)**
* **ATtiny85 (Standalone or Digispark)**
* **ATtiny167 (Digispark pro)**
* **ATmega32U4 (Leonardo or Micro and Pro Micro)**

Tip and Tricks:
--------------
Develop your project on an arduino **UNO** or **MEGA**, and then shrink it by loading the sketch in an **ATtiny** or **Digispark** (pro).

API/methods:
-----------
* The **TinySerial** library uses the same API as the regular **SoftwareSerial** library:
	* begin()
	* end()
	* available()
	* read()
	* overflow()
	* flush()

* Two additional methods are used to manage the serial port on a single pin:
	* txMode()
	* rxMode()

Examples of usage:
----------------
**Object declaration:**

- An usual serial port attached to pin 2 and pin 3: TinySerial MySerial(2, 3);

- An usual serial port attached to a single pin 2: TinySerial MySerial(2, 2);

**Data rate, data bit, parity, stop bit, inversion:**

- 9600 bauds, 8 data bit, None parity, 1 stop bit: MySerial.begin(9600); (default is SERIAL_8N1)

- 9600 bauds, 8 data bit, None parity, 1 stop bit, inverted signal: MySerial.begin(9600, SERIAL_8N1, true);

- 19200 bauds, 8 data bit, Even parity, 2 stop bit: MySerial.begin(19200, SERIAL_8E2);

- 19200 bauds, 7 data bit, Even parity, 1 stop bit, inverted signal: MySerial.begin(19200, SERIAL_7E1, true);

Design considerations:
---------------------
The **TinySerial** library relies the **TinyPinChange** library **for the RX pin**. This one shall be included in the sketch as well.

On the arduino MEGA, as all the pins do not support "pin change interrupt", only the following pins are supported **for the RX pin**:

* 10 -> 15
* 50 -> 53
* A8 -> A15

On the arduino Lenardo, Micro and Pro Micro (ATmega32U4), as all the pins do not support "pin change interrupt", only the following pins are supported:

* 0 -> 3 (external INT0, INT1, INT2 and INT3 are used as emulated Pin Change Interrupt)
* 8 -> 11 (pin 11 is not available on the connector)
* 14 -> 17 (pin 17 is not available on the connector)


On other devices (ATmega328, ATtiny84, ATtiny85 and ATtiny167), all the pins are usable.

Contact
-------

If you have some ideas of enhancement, please contact me by clicking on: [RC Navy](http://p.loussouarn.free.fr/contact.html).

