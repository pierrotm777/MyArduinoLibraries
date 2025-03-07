// RCPlaneTx.ino

#include "EasyTransfer.h"

#define throttlepin 0
#define elevpin 1
#define rudderpin 2
#define aileronpin 3

#define throttletrimpin 4
#define elevtrimpin 5
#define ruddertrimpin 6
#define ailerontrimpin 7

EasyTransfer ET;

int throttletrimval;
int elevtrimval;
int ruddertrimval;
int ailerontrimval;

struct SEND_DATA_STRUCTURE{	
	int throttleval;
	int elevval;
	int rudderval;
	int aileronval;
};

SEND_DATA_STRUCTURE txdata;

void setup() {

	Serial.begin(38400);

	ET.begin(details(txdata), &Serial);

}

void loop() {

	txdata.throttleval = analogRead(0);
	txdata.throttleval = map(txdata.throttleval, 0, 1023, 0, 179);
	throttletrimval = analogRead(4);
	throttletrimval = map(throttletrimval, 0, 1023, -25, 25);
	txdata.throttleval = txdata.throttleval + throttletrimval;
	constrain(txdata.throttleval, 0, 179);

	txdata.elevval = analogRead(1);
	txdata.elevval = map(txdata.elevval, 0, 1023, 0, 179);
	elevtrimval = analogRead(5);
	elevtrimval = map(elevtrimval, 0, 1023, -25, 25);
	txdata.elevval = txdata.elevval+ elevtrimval;
	constrain(txdata.elevval, 0, 179);

	txdata.rudderval = analogRead(2);
	txdata.rudderval = map(txdata.rudderval, 0, 1023, 0, 179);
	ruddertrimval = analogRead(6);
	ruddertrimval = map(ruddertrimval, 0, 1023, -25, 25);
	txdata.rudderval = txdata.rudderval+ ruddertrimval;
	constrain(txdata.rudderval, 0, 179);

	txdata.aileronval = analogRead(3);
	txdata.aileronval = map(txdata.aileronval, 0, 1023, 0, 179);
	ailerontrimval = analogRead(7);
	ailerontrimval = map(ailerontrimval, 0, 1023, -25, 25);
	txdata.aileronval = txdata.aileronval+ ailerontrimval;
	constrain(txdata.aileronval, 0, 179);
	
	ET.sendData();

}