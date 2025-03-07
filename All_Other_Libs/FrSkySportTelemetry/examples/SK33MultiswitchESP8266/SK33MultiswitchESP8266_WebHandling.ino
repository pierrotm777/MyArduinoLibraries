/* This are the WiFi access point settings. Update them to your likin */
//const char *ssid = "SK33";
//const char *password = "theodorstorm";

// Define a web server at port 80 for HTTP
//ESP8266WebServer server(80);

void handleRoot() {
  char html[1500];

  // Build an HTML page to display on the web-server root address
  snprintf ( html, 1000,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <title>SK33 Theodor Storm</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }\
      h1 { Color: #AA0000; }\
    </style>\
  </head>\
  <body>\
    <h1>SK33 Theodor Storm</br>\
    mit Nis Puk</h1>\
    </br>\
    <h2>\
      <p>Fahrtbeleuchtung:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/FON\">EIN</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/FOFF\">AUS</a><p>\
      <p>Schleppbeleuchtung:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/SON\">EIN</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/SOFF\">AUS</a><p>\
      <p>Blaulicht:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/BON\">EIN</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/BOFF\">AUS</a><p>\
      <p>Man&ouml;vrierbehindert:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/MON\">EIN</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/MOFF\">AUS</a><p>\
    </h2>\
  </body>\
</html>");
  server.send ( 200, "text/html", html );
}

void handleFahrtOn() {
  uiValue[0] |= B00000001; //Fahrtlicht EIN
  uiValue[0] &= B11111101; //Schlepplicht AUS
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleFahrtOff() {
  uiValue[0] &= B11111110;

  bValuesUpdated = true;
  
  handleRoot();
}

void handleSchleppOn() {
  uiValue[0] |= B00000010; //Fahrtlicht EIN
  uiValue[0] &= B11111110; //Schlepplicht AUS
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleSchleppOff() {
  uiValue[0] &= B11111101;
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleBluelightOn() {
  uiValue[0] |= B00000100; //Blaulicht EIN
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleBluelightOff() {
  uiValue[0] &= B11111011;
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleManeuverOn() {
  uiValue[1] |= B00000001; //Fahrtlicht EIN
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleManeuverOff() {
  uiValue[1] &= B11111110;
  
  bValuesUpdated = true;
  
  handleRoot();
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}
