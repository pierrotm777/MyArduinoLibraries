// Sketch to read a Rx receiver and to apply first channels to some servos
// V003
// Frome Sev

#define NO_PORTB_PINCHANGES //to go faster
#define NO_PORTC_PINCHANGES //to go faster

#include "PinChangeInt.h"

#include <Servo.h>

// LED pins
#define LEDPINO 13 //local Arduino PIN
#define LEDPINR 12 //RED LED for visible failsafe

// Parameters (total NBOFCH+NBOFSERVO can only be 10 MAX)
#define NBOFCH 6 // Put here the total Rx channel number and plug the FIRST channel on the PIN 2
#define NBOFSERVO 4 // Put here the total servo number and plug the FIRST one directely after the last Rx channel
#define FSCH 1 // This is the channel that will be survey for failsafe

// ServoParam
Servo outServ[NBOFSERVO];
int servPos[NBOFSERVO];


// Loop TimeStamp variables
unsigned long startLpTS, endLpTS, loopTime;
int i;

// Blink LED variables
int ledState[15] = {
  LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};  //TODO : Clean this dirty memory usage
unsigned long previousTS[15] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};                //TODO : Clean this dirty memory usage

// Blink LED function
void BLINK_LED(const int bLed,int bTimeUp,int bTimeDo)
{
  unsigned long currentTS = millis();
  int nowDelay;
  if (ledState[bLed]==HIGH) nowDelay=bTimeUp;
  else nowDelay=bTimeDo;
  if(currentTS - previousTS[bLed] > nowDelay) {
    previousTS[bLed] = currentTS;   
    if (ledState[bLed] == LOW) ledState[bLed] = HIGH;
    else       ledState[bLed] = LOW;
    digitalWrite(bLed, ledState[bLed]);
  }
}

// ReadReciever variables
boolean failsafe;
boolean endTrame;
unsigned long vchUp[NBOFCH+1];
int preCh; //TODO : Make this varaible cleared when entering in main loop (just use in setup)

unsigned long vchOld;

unsigned long vchLenght;

struct channel {
  int pos; // Why don't we just use an int ?
  int nxtCh;
  boolean autoP;
  int fPos;
};

struct channel ch[NBOFCH];

void DO_CHA()  // Interrupt use only to detect channel order
{
  if (PCintPort::arduinoPin==2){
    preCh=0;
    i=0;
  }
  else {
    ch[preCh].nxtCh = PCintPort::arduinoPin-2;
    preCh=PCintPort::arduinoPin-2;
    i++;
    if (i==NBOFCH-1) {
      ch[PCintPort::arduinoPin-2].nxtCh=NBOFCH;
      endTrame=1;
      preCh=PCintPort::arduinoPin-2;
    }
  }
}  

void setup()
{
  // DEBUG serial OUTPUT
  Serial.begin(57600);
  Serial.println(";");
  Serial.print("Receiver with ");
  Serial.print(NBOFCH);
  Serial.print(" channels and ");
  Serial.print(NBOFSERVO);
  Serial.println(" servos");

  Serial.print("Channel order : ");

  pinMode(LEDPINO, OUTPUT);
  pinMode(LEDPINR, OUTPUT);

  for (i=0; i < NBOFCH; i++){
    PCintPort::attachInterrupt(i+2, DO_CHA,RISING);
  }

  while(!endTrame){
    BLINK_LED(LEDPINO,50,50);
    BLINK_LED(LEDPINR,200,50);
  }

  for (i=0; i < NBOFCH; i++){
    PCintPort::detachInterrupt(i+2);
  }

  endTrame=0;

  for (int i=0; i < NBOFCH;i++) {
    Serial.print(ch[i].nxtCh);
    Serial.print(";");
  }
  Serial.println(";");

  // FailSafe Servos position
  ch[0].fPos = 1000;
  ch[1].fPos = 1500;
  ch[2].fPos = 1500;
  ch[3].fPos = 1500;

  FAILSAFE(); //Enabling FailSafe for a safe start



  // Enable final Interruption on channel's PIN
  for (int i=0; i < NBOFCH; i++){
    PCintPort::attachInterrupt(i+2, TS_REDG,RISING);
  }
  PCintPort::detachInterrupt(preCh+2);    // Detach last channel normal interrupt
  PCintPort::attachInterrupt(preCh+2, TS_CEDG,CHANGE); // Attach last channel special interrupt

  // Servo attachement
  for (int i=0; i < NBOFSERVO; i++){
    outServ[i].attach(i+NBOFCH+2);
  }

  Serial.print("SREG :");
  Serial.print(SREG,BIN);
  Serial.println(";");
  Serial.print("PCICR :");
  Serial.print(PCICR,BIN);
  Serial.println(";");
  Serial.print("PCMSK2 :");
  Serial.print(PCMSK2,BIN);
  Serial.println(";");
  Serial.print("PCMSK1 :");
  Serial.print(PCMSK1,BIN);
  Serial.println(";");
  Serial.print("PCMSK0 :");
  Serial.print(PCMSK0,BIN);
  Serial.println(";");

}




void loop()
{
  //TimeStamp Loop begining
  startLpTS = millis();

  if ((startLpTS*1000)>(vchUp[FSCH-1]+100000)){
    FAILSAFE();
  } //Detect failsafe on the (FSCH) Channel
  else{
    if (failsafe) {     //failsafe Exit  
      failsafe=0;
      digitalWrite(LEDPINR, LOW);
      endTrame=0;
    }
    else{
      if  (endTrame) { //End of trame detection
        noInterrupts(); //Stop IRQ & copy channels values
        for (int i=0; i < NBOFCH; i++){
          ch[i].pos=vchUp[ch[i].nxtCh]-vchUp[i];
        }
        vchLenght=vchUp[0]-vchOld;
        vchOld=vchUp[0];
        interrupts();  //Restart IRQ & Reset endTrameflag
        endTrame=0;
        // Set servos as Channel
        Serial.print("UPD");
        for (int i=0; i < NBOFSERVO; i++){
          outServ[i].writeMicroseconds(ch[i].pos);
        }
      }
    }
  }

  // DEBUG : Serial output the channels
  for (int i=0; i < NBOFCH; i++){
    Serial.print(ch[i].pos);
    Serial.print(";");
  }

  //Blink "normaly" the Orange LED
  BLINK_LED(LEDPINO,500,500);



  // DEBUG : Serial output the Loop Duration (ms)
  Serial.print(vchLenght); 
  Serial.print(";");
  Serial.print(micros()-vchUp[0]); 
  Serial.print(";");

  //EndLoop TimeStamp
  endLpTS = millis();
  loopTime = endLpTS - startLpTS;

  Serial.print(loopTime); 
  Serial.println(";");
}

void FAILSAFE()  // In case of lost signal
{
  Serial.print("FailSafe;");
  if (!failsafe) {
    digitalWrite(LEDPINR, HIGH);
    // Set channels on failsafe positions
    for (int i=0; i < NBOFCH; i++){
      ch[i].pos=0;
    }
    // Set servos on failsafe positions
    for (int i=1; i < NBOFSERVO; i++){
      outServ[i].writeMicroseconds(ch[i].fPos);

    }
    failsafe=1;
  }
}  


void TS_REDG()  // Same Interrupt code on all channels except the last one
{
  vchUp[PCintPort::arduinoPin-2] = micros();
}  


void TS_CEDG()  // Special interrupt on last channel
{

  // If this is a rising edge, record it
  if(PCintPort::pinState == HIGH)
  { 
    vchUp[PCintPort::arduinoPin-2] = micros();
  }
  else
  {
    //If this is a falling edge, record it as last value
    vchUp[NBOFCH] = micros();
    endTrame=1; // End of trame is set when the last falling edge of the last channel has gone !
  }
}