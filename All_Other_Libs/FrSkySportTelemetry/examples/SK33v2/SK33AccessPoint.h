#ifndef _SK33AccessPoint_H_
#define _SK33AccessPoint_H_

#include "Arduino.h"

#include <ESP8266WiFi.h>

class SK33AccessPoint
{
	
	
	public:
		SK33AccessPoint();
		void startWiFi();
	private:
		// /* This are the WiFi access point settings. Update them to your likin */
		const char *ssid = "SK33";
		const char *password = "theodorstorm";
};


#endif //_SK33AccessPoint_H_