/* Created 08.11.2015 A. Broenner
 *  Based on Quelle: http://www.rc-network.de/forum/showthread.php/535749-Arduino-GPS-Datenlogger-DIY-Projekt
 * ********** SD card attached to SPI bus |   ******** GPS ***************************
 ** MOSI - pin 11                         |   ** TX, RX, VCC and Gnd
 ** MISO - pin 12                         |   ** modul  TX to Pin D3 on the Arduino, 
 ** CLK - pin 13                          |   ** modul  RX to Pin D4 
 ** CS - pin 10                           |   ** VCC 5v and Gnd                      */

#include <TimerOne.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>

#define RxPin 3           // connected to Tx pin of the GPS
#define TxPin 4           // connected to Rx pin of the GPS
#define Satellit_min 3    // minimum Satellites
#define LED A4            // Connected to LED

SoftwareSerial ss(RxPin, TxPin);      //UART für GPS einrichten
TinyGPSPlus gps;                      //create GPS Object

typedef enum {WAIT_SATELLIT,CREATE_FILENAME,SD_ERROR,RECORD,WAIT_UPDATE} Status_t;
Status_t Status=WAIT_SATELLIT;
Status_t LED_Status=WAIT_SATELLIT;
File dataFile;

char filename[10];
const int chipSelect = 10;
bool newData = false;
//  needs to be adjusted based on GPS date to switch automaticly to Summer/Winter time
byte SW = B00001;          // B00001 Wintertime, B00010 Summertime

// ***************** create date and time stamp for file
void dateTime(uint16_t* date, uint16_t* time)
{
  *date = FAT_DATE(gps.date.year(), gps.date.month(), gps.date.day());
  *time = FAT_TIME(gps.time.hour()+SW, gps.time.minute(), gps.time.second());
}
// *****************************************

void setup(){
  //GPS_Setup();
  Timer1.initialize(100000);              // 10Hz
  Timer1.attachInterrupt(INTERRUPT);      // interrupt
  pinMode(8, OUTPUT);                     //LED Red - Card failed
  pinMode(6, OUTPUT);                     //LED Green - Card Ok
  pinMode(LED, OUTPUT);                   //Staus LED
  Serial.begin(115200);
  ss.begin(9600);                         //GPS Sensor com speed
  pinMode(chipSelect, OUTPUT);            // Pin für Chip Select als Output einstellen
  digitalWrite(chipSelect, HIGH);         // Chip Select für SD Karten Leser immer aktiv
  
  //startMillis = millis();
  
  SdFile::dateTimeCallback(dateTime);         // Set/Write date and time stamp for file
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    digitalWrite(8, HIGH); // set the LED on - Card failed
    Serial.println("Card failed, or not present");
    Status=SD_ERROR;                      // Wenn nicht, SD Error
    // don't do anything more:
    return;
    }
    digitalWrite(6, HIGH); // set the LED on - Card initialized
    Serial.println("card initialized.");
}

void loop()
{
     GPS_Values();                           // GPS Informationen einlesen
    
if (newData){

    // call functions
    State_Sat();                            // Aktuellen Programmstatus überprüfen
   
    Check_Update();                         // GPS Informationen auf Updates überprüfen
    Write_Data();                           // Daten auf SD schreiben
    Write_Serial();                         // Daten Serial ausgeben

  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening gps_log.txt");
  }
}

void INTERRUPT() {
  Status_LED();                           // LED Status
}
