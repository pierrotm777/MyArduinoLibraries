//https://www.hackster.io/yvesmorele/low-cost-telemetry-and-data-logger-flysky-turnigy-8491d1


#include <iBUSTelemetry.h> //https://github.com/adis1313/iBUSTelemetry-Arduino
#include <Wire.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>

//pin pour carte SD
const int cs_sd=2;

TinyGPSPlus gps;

#define UPDATE_INTERVAL 500

// pin voltage
#define dP A0 

iBUSTelemetry telemetry(9); // arduino pin pour donnes iBUS 

  
  uint32_t prevMillis =0; // neessaire pour updateValues()

  //LED 
  
    const int verte = 3;
    const int rouge = 4; 

    float maxspeed = 0, speed1 = 0;
    int maxhigh = 0, high1 = 0;
    int maxsatelite = 0, satelite1 = 0;

  float meters ; 
  float voltage=0.0;
  
void setup()
{
  Serial.begin(9600);

  
  pinMode(rouge, OUTPUT);
  pinMode(verte, OUTPUT);

  //Condition vrifiant si la carte SD est prsente dans l'appareil

if(!SD.begin(cs_sd))    
  {
   
  digitalWrite(rouge, HIGH);//absence SD
    return;
  }

 digitalWrite(rouge, LOW);// carte SD absente

    File data = SD.open("donnees.txt",FILE_WRITE);              // Ouvre le fichier "donnees.txt"
    data.println(""); data.println("Demarrage acquisition ibus"); // Ecrit dans ce fichier
    data.println(""); data.println("date      heure      latitude       longitude   altitude   vitesse"); 
    data.close(); 
  
  Wire.begin();
  telemetry.begin(); 
  
 // definition capteurs
  
  telemetry.addSensor(IBUS_MEAS_TYPE_ALT); //altitude
  telemetry.addSensor(IBUS_MEAS_TYPE_EXTV); //batterie
  telemetry.addSensor(IBUS_MEAS_TYPE_SPE); //vitesse km/h
  telemetry.addSensor(IBUS_MEAS_TYPE_GPS_LAT); // latitude
  telemetry.addSensor(IBUS_MEAS_TYPE_GPS_LON); // longitde

 
}

void loop()
{
   
    updateValues(); 
    telemetry.run();  
}

void updateValues()
{

// lecture donnes Gps et allumage LED si OK

    if (Serial.available()) {
     digitalWrite(verte, HIGH);//donnes Gps OK
     gps.encode(Serial.read());
}



  
 
  float meters = (gps.altitude.meters());
  voltage=float(analogRead(dP))*(20/1023.00);
  float airSpeed = (gps.speed.kmph());

// definition vitesse max

    speed1 = (gps.speed.kmph());
    if ( speed1 > maxspeed) {
     maxspeed = speed1;
    }


  // recuperation date et heure du GPS pour datalogging
  
      String Temps=String(gps.time.hour()+1)+(":")+(gps.time.minute())+(":")+(gps.time.second());
      String Date=String(gps.date.day())+("/")+(gps.date.month())+("/")+(gps.date.year());

// Temporisation
  
   uint32_t currMillis = millis();

    if (currMillis - prevMillis >= UPDATE_INTERVAL) { // Code in the middle of these brackets will be performed every 500ms.
        prevMillis = currMillis;
     
  
// affichage donnes capteur sur radiocommande

  telemetry.setSensorValue(1, (meters )*100.0  ); //  altitude
  
 telemetry.setSensorValue(2, voltage*100  );

 telemetry.setSensorValue(3, speed1 );

 telemetry.setSensorValue(4, (gps.location.lat()*10000000));

 telemetry.setSensorValue(5, (gps.location.lng()*10000000)) ;
 

  
   // Ecriture des donnes dans le fichier texte

   
    File data=SD.open("donnees.txt",FILE_WRITE);
  
    data.println(Date + " " + Temps + " " + String(gps.location.lat(), 6)+" "+String(gps.location.lng(), 6)+(" ")+String(gps.altitude.meters(),0)+(" ")+String(speed1)+(" ")+String(maxspeed)); 
    data.close();
    }
}