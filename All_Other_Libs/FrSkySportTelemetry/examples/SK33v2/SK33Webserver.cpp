#include "SK33WebServer.h"

#include <ESP8266WebServer.h>

//constructor
SK33WebServer::SK33WebServer() :  _server(80) { }

void SK33WebServer::init() {
	_server.on ( "/", std::bind(&SK33WebServer::handleRoot, this) );
	_server.on ( "/F", std::bind(&SK33WebServer::handleFahrt, this) );
	_server.on ( "/S", std::bind(&SK33WebServer::handleSchlepp, this) );
	_server.on ( "/B", std::bind(&SK33WebServer::handleBluelight, this) );
	_server.on ( "/M", std::bind(&SK33WebServer::handleManeuver, this) );
	_server.on ( "/R", std::bind(&SK33WebServer::handleRadar, this) );
	_server.on ( "/W", std::bind(&SK33WebServer::handleWipers, this) );
  
	_server.onNotFound ( std::bind(&SK33WebServer::handleNotFound, this) );
}

void SK33WebServer::begin(void) { _server.begin(); }

void SK33WebServer::handleClient(void) { _server.handleClient(); }

void SK33WebServer::handleRoot() {
	char html[2500];

	const char *fahrt="/F";
	const char *schlepp="/S";
	const char *blaulicht="/B";
	const char *manoever="/M";
	const char *radar="/R";
	const char *wipers="/W";

	//Serial.println("Webserver::handleRoot");

	// Build an HTML page to display on the web-server root address
	snprintf ( html, 2500,

"<html><head><title>SK33 Theodor Storm</title><style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }</style></head>\
<body><svg usemap='#triggerpoints' width='1024' height='600'><text x='0' y='40' fill='darkred' font-size='40'>SK 33 Theodor Storm</text><g stroke='black' stroke-width='1'><g fill='white'><rect x='690' y='90' width='30' height='5'/><polygon points='30,285 230,300 311,300 336,260 360,258 340,215 380,212 480,212 480,255 650,255 690,160 645,160 604,255 599,255 645,145 695,145 730,70 685,70 685,65 720,65 720,45 726,45 726,65 736,65 736,10 741,10 741,65 760,65 760,70 740,70 660,255 665,255 685,310 685,370 950,370 950,425 95,425 10,320'/></g><polygon points='330,215 332,213 420,202 480,200 520,202 522,210 531,220 547,230 570,240 590,245 620,250 665,255 480,255 480,212 380,212 340,215' fill='red'/><polygon points='15,310 195,338 230,327 265,330 320,356 685,360 950,360 950,370 920,370 920,398 885,398 885,370 650,370 320,365 10,320' fill='orangered'/><g fill='gray'><polygon points='350,224 358,224 367,250 360,250'/><polygon points='365,224 375,224 385,250 373,250'/><polygon points='385,224 425,224 425,250 395,250'/><polygon points='435,224 460,224 460,250 435,250'/></g>\
<!--leuchten--><!--fahrt--><g><circle cx='738' cy='11' r='10' fill='white'/><circle cx='675' cy='150' r='10' fill='green'/><circle cx='685' cy='150' r='10' fill='red'/><circle cx='670' cy='280' r='10' fill='white'/></g>\
<a href='%s'><text x='755' y='25'>Fahrtbe-</text><text x='755' y='50'>leuchtung</text></a>\
<!--Schlepp--><g><circle cx='738' cy='55' r='10' fill='white'/><circle cx='670' cy='250' r='10' fill='yellow'/></g>\
<a href='%s'><text x='690' y='257'>Schlepplicht</text></a>\
<!--blaulicht--><circle cx='723' cy='35' r='10' fill='blue'/>\
<a href='%s'><text x='610' y='42'>Blaulicht</text></a>\
<!--Manövrier--><g><circle cx='725' cy='80' r='10' fill='white'/><circle cx='725' cy='105' r='10' fill='red'/><circle cx='725' cy='130' r='10' fill='white'/></g>\
<a href='%s'><text x='740' y='97'>Man&ouml;vrier-</text><text x='740' y='128'>behindert</text></a>\
<!--Radar--><rect width='100' height='15' x='600' y='120' fill='white'/>\
<a href='%s'><text x='510' y='135'>Radar</text></a>\
<!--Wipers-->\
<a href='%s'><text x='110' y='235'>Scheibenwischer</text></a>\
</svg>\
</body></html>", fahrt, schlepp, blaulicht, manoever, radar, wipers);
	_server.send ( 200, "text/html", html );
  
	//SK33data.Print();
}

void SK33WebServer:: handleFahrt() {
	//Serial.println("Webserver::handleFahrt");
	if ((SK33data.GetValue0() & B00000001) != 0) {
		//ausschalten
		SK33data.SetValue0(SK33data.GetValue0() & B11111110);
	} else {
		//einschalten
		SK33data.SetValue0(SK33data.GetValue0() | B00000001); //Fahrtlicht EIN
		SK33data.SetValue0(SK33data.GetValue0() & B11111101); //Schlepplicht AUS
	}
	SK33data.Changed();
	handleRoot();
}

void SK33WebServer:: handleSchlepp() {
	//Serial.println("Webserver::handleSchlepp");
	if ((SK33data.GetValue0() & B00000010) != 0) { 
		SK33data.SetValue0(SK33data.GetValue0() & B11111101);
	} else {
		SK33data.SetValue0(SK33data.GetValue0() | B00000010); //Fahrtlicht EIN
		SK33data.SetValue0(SK33data.GetValue0() & B11111110); //Schlepplicht AUS
	}
	SK33data.Changed();
	handleRoot();
}

void SK33WebServer:: handleBluelight() {
	//Serial.println("Webserver::handleBluelight");
	if ((SK33data.GetValue0() & B00000100) != 0) {
		SK33data.SetValue0(SK33data.GetValue0() & B11111011);
	} else 	{
		SK33data.SetValue0(SK33data.GetValue0() | B00000100); //Blaulicht EIN
	}
	SK33data.Changed();
	handleRoot();
}

void SK33WebServer:: handleRadar() {
	//Serial.println("Webserver::handleRadar");
	if ((SK33data.GetValue0() & B00010000) != 0) {
		SK33data.SetValue0(SK33data.GetValue0() & B11101111);
	} else {
		SK33data.SetValue0(SK33data.GetValue0() | B00010000); //Radar EIN
	}
	SK33data.Changed();
	handleRoot();
}

void SK33WebServer:: handleWipers() {
	//Serial.println("Webserver::handleWipers");
	if ((SK33data.GetValue0() & B01000000) != 0) {
		SK33data.SetValue0(SK33data.GetValue0() & B10111111);
	} else {
		SK33data.SetValue0(SK33data.GetValue0() | B01000000); //Radar EIN
	}
	SK33data.Changed();
	handleRoot();
}

void SK33WebServer:: handleManeuver() {
	//Serial.println("Webserver::handleManeuver");
	if ((SK33data.GetValue1() & B00000001) != 0) {
		SK33data.SetValue1(SK33data.GetValue1() & B11111110);
	} else {
		SK33data.SetValue1(SK33data.GetValue1() | B00000001); //Fahrtlicht EIN
	}
	SK33data.Changed();
	handleRoot();
}

void SK33WebServer:: handleNotFound() {
	//Serial.println("Webserver::handleNotFound");

	String message = "File Not Found\n\n";
	message += "URI: ";
	message += _server.uri();
	message += "\nMethod: ";
	message += ( _server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += _server.args();
	message += "\n";

	for ( uint8_t i = 0; i < _server.args(); i++ ) {
		message += " " + _server.argName ( i ) + ": " + _server.arg ( i ) + "\n";
	}

	_server.send ( 404, "text/plain", message );
}
