#include <SD.h>               // need to include the SD library
#define SD_ChipSelectPin 10   //using digital pin 4 on arduino nano 328, can use other pins
#include <TMRpcmSpeed.h>      //  also need to include this library...
#include <SPI.h>

TMRpcmSpeed tmrpcm;           // create an object for use in this sketch

#include <Rcul.h>
#include <SoftRcPulseIn.h>
SoftRcPulseIn PwmRcEngine;
#define AVERAGE_LEVEL       2 /* Choose here the average level among the above listed values */
                              /* Higher is the average level, more the system is stable (jitter suppression), but lesser is the reaction */
/* Macro for average */
#define AVERAGE(ValueToAverage,LastReceivedValue,AverageLevelInPowerOf2)  ValueToAverage=(((ValueToAverage)*((1<<(AverageLevelInPowerOf2))-1)+(LastReceivedValue))/(1<<(AverageLevelInPowerOf2)))

unsigned long time = 0;
int numLoop = 0;
uint16_t prevThrottle = 0;
uint16_t currThrottle = 0;
uint8_t playingSound = 0;

unsigned int sampleSpeed = 8000;      // Loop samplerate
unsigned int sampleSpeed_max = 40000; // Loop samplerate max
  
//RC scale
uint8_t EnginePin = 2;
uint16_t NewEngineUs;
uint16_t prevNewEngineUs;
uint8_t NewVolume, OldVolume;
uint16_t NewVolumeUs;
  
uint8_t mode = 0;//0=avion 1=bateau/voiture

void setup()
{
  PwmRcEngine.attach(EnginePin);
  tmrpcm.speakerPin = 9; //5,6,11 or 46 on Mega, 9 on Uno, Nano, etc
  Serial.begin(115200);
  // while (!Serial) 
  // {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }
  delay(1000);
  if (!SD.begin(SD_ChipSelectPin)) {  // see if the card is present and can be initialized:
    Serial.println("SD fail");  
    while (true);
  }
  else
  {   
    Serial.println("SD ok");   
  }

  if (mode == 0)
    NewEngineUs = 990;
  else if (mode == 1)
    NewEngineUs = 1500;

  //tmrpcm.setVolume(6);//0 to 7. Set volume level
  //PLay init file to tell sound version
  // Serial.println("beginsound.wav");
  // tmrpcm.play("beginsound.wav");
  // while(tmrpcm.isPlaying())
  // {
  //   delay(2000); 
  // }; 
  //Serial.println("setup end");
}
  
void loop()
{
  //Read throttle value
  ++numLoop;
  //Serial.println(numLoop);

  if (numLoop == 30000)
  {
    // noInterrupts();
    // NewEngineUs = pulseIn(EnginePin, HIGH);
    // interrupts();
    if(PwmRcEngine.available())
    {
      AVERAGE(NewEngineUs,PwmRcEngine.width_us(),AVERAGE_LEVEL);//channel
      //Serial.print(NewEngineUs);
    }
   
    numLoop = 1;

    //Set new throttle value
    if (mode == 0)//mode avion
    {
      if(NewEngineUs <= 1010){
        currThrottle = 0;
      }
      if((NewEngineUs > 1010) && (NewEngineUs <= 1050)){
        currThrottle = 1;
      }
      if((NewEngineUs > 1050) && (NewEngineUs < 2010)){
        currThrottle = 2;
      }

      sampleSpeed = map(NewEngineUs, 988, 2010, tmrpcm.orgsamplerate, sampleSpeed_max);
      tmrpcm.speed(sampleSpeed); 
    }//fin avion


    else if (mode == 1)//bateau
    {
      if(NewEngineUs > 1480 && NewEngineUs < 1520)//center
      {
        currThrottle = 0;
      }
      if((NewEngineUs > 1400 && NewEngineUs <= 1490) || (NewEngineUs > 1510 && NewEngineUs <= 1600)) {
        currThrottle = 1;
      }
      if(NewEngineUs < 1400 || NewEngineUs > 1600) {
        currThrottle = 2;
      }

      if (NewEngineUs < 1480)
      {
        sampleSpeed = map(NewEngineUs, 1480, 990, tmrpcm.orgsamplerate, sampleSpeed_max);
        tmrpcm.speed(sampleSpeed);      
      }
      if (NewEngineUs > 1520) 
      {
        sampleSpeed = map(NewEngineUs, 1520, 2010, tmrpcm.orgsamplerate, sampleSpeed_max);
        tmrpcm.speed(sampleSpeed);      
      }  
    }//fin bateau

    //If currThrottle != prevThrottle set start playing new file
    if(currThrottle != prevThrottle)
    {
      //Serial.print(NewEngineUs);Serial.print("\t");
      if((currThrottle == 0) && (prevThrottle > 0)/* && (mode == 1)*/)
      {
        tmrpcm.play("shut.wav");
        tmrpcm.speed(tmrpcm.orgsamplerate);
        //Serial.println("shut.wav");
        tmrpcm.loop(0);
        while(tmrpcm.isPlaying()){}
        tmrpcm.disable();  // disables the timer on output pin and stops the music
        prevThrottle = currThrottle;
      }

      if(currThrottle == 1){
        if(currThrottle > prevThrottle)
        {
          tmrpcm.play("start.wav");
          tmrpcm.speed(tmrpcm.orgsamplerate);
          //Serial.println("start.wav");
          tmrpcm.loop(0);
          while(tmrpcm.isPlaying()){}
        }
        playingSound = 1;
        prevThrottle = currThrottle;
      }
      if(currThrottle == 2){
        tmrpcm.play("1.wav");
        prevThrottle = currThrottle;
        playingSound = 1;
      }
    }

    // Serial.println(tmrpcm.isPlaying());
    if((currThrottle = prevThrottle) && (tmrpcm.isPlaying() == 0))
    {
      if(playingSound == 1){
        tmrpcm.play("1.wav");
        //Serial.println("again 1.wav");
        prevThrottle = currThrottle;
      }
    }
    
  }
}

