#ifndef _SK33WebServer_H_
#define _SK33WebServer_H_

#include "Arduino.h"

#include <ESP8266WebServer.h>

#include "SK33RcvData.h"

class SK33WebServer
{
	public:
		SK33WebServer();
		
		void init();
		void begin(void);
		void handleClient(void);
		
		void handleRoot(void);
		void handleFahrt(void);
		void handleSchlepp(void);
		void handleBluelight(void);
		void handleRadar(void);
		void handleWipers(void);
		void handleManeuver(void);
		void handleNotFound(void);
		
	private:
		ESP8266WebServer _server;
		uint8_t* _pUiValue;
		SK33RcvData SK33data;
};

#endif //_SK33WebServer_H_