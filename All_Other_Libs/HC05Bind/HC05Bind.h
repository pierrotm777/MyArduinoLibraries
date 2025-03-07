

#ifndef HC05Bind_h
#define Hc05Bind_h

/*-----( Import needed libraries )-----*/
#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include "SoftwareSerial.h"

/*-----( Declare Constants and Pin Numbers )-----*/

#define HC05PowerPin 12 //power to HC05
#define HC05AtPin 11 //puts HC05 into command mode
#define R 6371000.00 //radius of the earth
#define PI 3.14159265
#define TORADS PI/180.0
#define TODEGS 180.0/PI

class HC05Bind
{
/*-----( Declare Variables and functions )-----*/
  public:
    HC05Bind();
	bool search();
	bool begin(String pw);
	void comms();
	void gps();
	bool available;
	String NMEAphrase;
	bool fix = false;
	int latDeg;
	float latMin;
	int longDeg;
	float longMin;
	String latSector;
	String longSector;
	String UTC;
	float altitude;
	int tracked;
	String PRNs;
	bool paired = false;
	float DOP;
	float latDegFloat;
	float longDegFloat;
	distance();
	float distance(float lat1,float long1,float lat2,float long2);
	float lat1;
	float lat2;
	float long1;
	float long2;
	bearing();
	float bearing(float lat1,float long1,float lat2,float long2);
	

	
    
  private:
	String readHC05();
	void sendAtCommand(String command);
	void setHC05ToCommandMode();
	void writeString(String stringData);
	bool clearPairings();
	bool setToMaster();
	bool setToAnyBT();
	bool setToEnquiry();
	void SppProfile();
	bool setPassword(String pw);
	bool find();
	bool locate();
	String reply;
	bool started = false;
	bool ended = true;
	char c;
	int countChars = 0;
	String phrase;
	String getChecksum(String sentence);
	bool isValid(String sentence);
	int findComma(int nthComa);
	float theta1;
	float theta2;
	float deltaTheta;
	float deltaLanda;
	float a;
	float c1;
	float d;
	float landa1;
	float landa2;
	float x;
	float y;
	float brng;
	
};

#endif
