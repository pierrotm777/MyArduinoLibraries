// RCPlaneRx.ino

#include "Servo.h"
#include "EasyTransfer.h"

#define throttlepin 9
#define elevpin 8
#define rudderpin 10
#define laileronpin 11
#define raileronpin 12

Servo throttle, elev, rudder, laileron, raileron;

EasyTransfer ET, ETrange;

struct RECEIVE_DATA_STRUCTURE{
	int throttleval;
	int elevval;
	int rudderval;
	int aileronval;
};

struct RANGE_TEST_STRUCTURE{
	int rangetest;
};

RECEIVE_DATA_STRUCTURE txdata;
RANGE_TEST_STRUCTURE rangetestdata;

//long previousMillis = 0;

//long interval = 1500;

void setup() {

	pinMode(13, OUTPUT);

	throttle.attach(9);
	elev.attach(8);
	rudder.attach(10);
	laileron.attach(11);
	raileron.attach(12);

	/*
	elev.write(90);
	rudder.write(90);
	laileron.write(90);
	raileron.write(90);
	*/

	Serial.begin(38400);

	ET.begin(details(txdata), &Serial);
	//ETrange.begin(details(rangetestdata), &Serial);

}

void loop() {
	
	if(ET.receiveData()){	

		throttle.write(txdata.throttleval);
		elev.write(txdata.elevval);
		rudder.write(txdata.rudderval);
		laileron.write(txdata.aileronval);
		raileron.write(txdata.aileronval);

	}

	//rangetestdata.rangetest = 1;
	
	//ETrange.sendData();

	/*
	unsigned long currentMillis = millis();

	if(currentMillis - previousMillis > interval){

		previousMillis = currentMillis;

		if(digitalRead(13) == HIGH)
			digitalWrite(13, LOW);
		else
			digitalWrite(13, HIGH);

		rangetestdata.rangetest = 1;
		ETrange.sendData();
	    
	}
	*/

	/* 
	else {

		throttle.write(40);
		elev.write(90);
		rudder.write(90);
		laileron.write(110);
		raileron.write(110);
	
	} 
	*/

}
