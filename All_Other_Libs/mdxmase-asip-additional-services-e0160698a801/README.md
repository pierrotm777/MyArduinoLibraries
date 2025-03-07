# Asip Additional Services #

This library adds support for additional services for the Asip protocol: sonar distance sensor, servo motors, tones.

### What is this repository for? ###

* This repository includes code that allows to extend the basic I/O functionality of Asip. It is intended to collect services that do not require additional libraries (apart from Asip core).
* Version: 0.1.0

### Quick installation instructions ###

* Open the Arduino IDE (please use version 1.5 or above).
* Make sure that Asip core is installed, see [https://bitbucket.org/mdxmase/asip](https://bitbucket.org/mdxmase/asip)
* Select Sketch -> Include Library -> Manage Libraries...
* Search for asip-additional-services and install it
* Select one of the examples available: click on File -> Examples -> asip-services  and choose one.
* Connect a board, select the appropriate board and port from the Tools menu and upload the selected sketch.
* You are now ready to go. You can either send messages directly through the serial port (remember to set the baud rate to 57600) or you can download a client for a programming language such as Java, Python and Racket.


### Additional help ###

* Don't be afraid of contacting us. We'll try to reply as soon as possible. Feel free to open issues.
* Do you want to contribute? Please get in touch :-)! We need help for a number of things, starting from documentation to the development of other clients, additional services, examples, etc.