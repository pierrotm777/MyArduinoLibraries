  JetiExProtocol - EX protocol implementation
  --------------------------------------------------------------------
  
  Designed for Arduino Mini Pro 328, Nano, Leonardo/Pro Micro, Teensy 3.x and IDE 1.6 and higher

  Version history:

    0.90   11/22/2015  created
 
    0.92   12/14/2015  set baudrate for 8 MHz Mini Pro added
                       15 sensor values, data type 22b added
    0.93   12/14/2015  bug fixed when less sensors than fit in one frame	   

    0.94   12/22/2015  Teensy 3.x support on Serial2
    0.95   12/23/2015  Refactoring
                       Data types added: GPS, date/time, 6b, 30b
    0.96   02/21/2016  comPort number as optional parameter for Teensy in Start(...)
                       sensor device id as optional parameter (SetDeviceId(...))
                       new sample program: JetiExSimple (stripped down to the essential things)
    0.97   02/26/2016  runs w/o EX sensors (Jetibox only), see sample "JetiNoxEX"
    0.98   04/09/2016  slightly improved sensor packet size calculation (avoids "flickering" values)
    0.99   06/05/2016  max number of sensors increased to 32 
                          (set MAX_SENSOR to a higher value in JetiExProtocol.h if you need more)
                       bug with TYPE_6b removed
                       DemoSensor delivers 18 demo values now
    1.00   01/29/2017  Some refactoring:
                       - Bugixes for Jetibox keys and morse alarms (Thanks to Ingmar !)
                       - Optimized half duplex control for AVR CPUs in JetiExHardwareSerialInt class (for improved Jetibox key handling)
                       - Reduced size of serial transmit buffer (128-->64 words) 
                       - Changed bitrates for serial communication for AVR CPUs (9600-->9800 bps)
                       - EX encryption removed, as a consequence: new manufacturer ID: 0xA409
                         *** Telemetry setup in transmitter must be reconfigured (telemetry reset) ***
                       - Delay at startup increased (to give receiver more init time)
                       - New HandleMenu() function in JetiexSensor.ini (including more alarm test)
                       - JETI_DEBUG and BLOCKING_MODE removed (cleanup)
    1.0.1  02/15/2017  Support for ATMega32u4 CPU in Leonardo/Pro Micro
                       GetKey routine optimized 
    1.0.2  03/28/2017  New sensor memory management. Sensor data can be located in PROGMEM
					   *****  You have to adapt the sensor array initialization. See examples ! *****
