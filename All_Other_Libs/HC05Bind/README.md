# HC05Bind
Use Arduino to pair a HC-05 Bluetooth module with a Bluetooth device. Allows communication between devices using the Arduino GUI serial monitor. If the BT device is a GPS module that sends NMEA sentences the library will parse GGA and GSA sentences and display NMEA sentence, latitude, longitude, altitude, UTC, number of satellites tracked, satellite PRN numbers and dilution of position. Latitude, longitude and altitude are only stable if 4 or more satellites are tracked. Given a final destination, the library will calculate distance and bearing from your present location to the final destination.

Basic Bluetooth sketch connects (at 38400) the HC-05 to a BT device
and communicate using the serial monitor at 9600

Connect HC-05 VCC to Arduino pin 12

Connect H-C05 EN to Arduino pin 11 through a voltage divider.

Connect the HC-05 GND to GND

Connect the HC-05 TX to Arduino pin 2 RX.

Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.

See example for methods.
