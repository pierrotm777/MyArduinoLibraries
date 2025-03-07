#include "SK33AccessPoint.h"

#include <ESP8266WiFi.h>

SK33AccessPoint::SK33AccessPoint() { }

void SK33AccessPoint::startWiFi() {
	Serial.println("Configuring access point...");
	
	// set-up the custom IP address
	WiFi.mode(WIFI_AP_STA);
  
	// You can remove the password parameter if you want the AP to be open.
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
}