This library was created to work with Arduino Nano V3.0 and Arduino IDE 1.0x.

Version history:

  1.00	06/04/2014	Created
  1.01	06/28/2014	"Convenience" macros added
  1.02  07/09/2014      Optimized ISR. Max. software PWM 10 channels can be serviced now
                        New programming interface for simplified usage
                        Low battery handling
  1.03  01/15/2015 	Sleep mode changes - dont let CPU sleep

                        Analog pin for voltage measurement can be defined in

                        SetLowBatt(...)

                        You can provide the receiver input value in DoLoop(...)

                        now to replace the receiver output by some other measurement

  1.04  03/04/2015 	multiple input channels
, new sample4 to demonstrate usage of 2nd rc input
  1.05  03/07/2015 	storage structure modified to separate workdata from static data

                   	(no separate SEQUENCE() for each pin needed anymore)
