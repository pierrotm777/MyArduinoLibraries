# FS_EspSoftwareSerial
 
## This library is an adaptation  of the Arduino software serial library for the ESP8266 / ESP32 family


The standard [EspSoftwareSerial library](https://github.com/plerup/espsoftwareserial) doesnot work well on an ESP32C3, at least with ESP package 2.0.5 Sept 2022(or previous ...)  
The main problem is when some delay(xxx) occur in the application when the user code is running. Even if it is possible to track these delays in the user code, it is impossible to control what is done in the libraries. For example the wifi libraries include some "delay()" and so EspSoftwareSerial can't be used  in this context.   
  
The ESP32C3 only have 2 UARTs and it may be usefull to use EspSoftwareSerial.  
  
This version is bases on [EspSoftwareSerial library](https://github.com/plerup/espsoftwareserial) version 6.16.1  April 2022  

These patches only apply for ESP32C3 and all others paltforms use the same code as the original library.  

Reading is pretty good at "low" speed  9600/19200  
Writing is still bad ...
