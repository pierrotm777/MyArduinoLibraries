
void Write_Data (void){
   if(Status==RECORD){   
      // make a string for assembling the data to log:
  String dataString = "";
  dataFile = SD.open(filename, FILE_WRITE);
  if(dataFile){
    dataString = String(gps.time.hour()+SW);   //hour = utc  +1 Wintertime +2 Summertime
    dataString += ".";
    dataString += String(gps.time.minute());
    dataString += ".";
    dataString += String(gps.time.second());
    dataString += ";";
    dataString += String(gps.date.day());
    dataString += ".";
    dataString += String(gps.date.month());
    dataString += ".";
    dataString += String(gps.date.year());
    dataString += ";";
    dataString += String(gps.satellites.value());
    dataString += ";";
    dataString += String(gps.location.lat(), 6);      // 6 Kommastellen
    dataString += ";";
    dataString += String(gps.location.lng(), 6);
    dataString += ";";
    dataString += String(gps.altitude.meters(),2);    // 2 Kommastellen
    dataString += ";";
    dataString += String(gps.speed.kmph(),2);
    dataFile.println(dataString);                     //new line
    dataFile.close();   
    }
    else{
    Status=SD_ERROR;
    }
  }
}


//***********  for debugging only  ****************
void Write_Serial (void){
  String dataString = "";
    dataString = String(gps.time.hour()+SW);   //hour = utc  +1 Wintertime +2 Summertime
    dataString += ".";
    dataString += String(gps.time.minute());
    dataString += ".";
    dataString += String(gps.time.second());
    dataString += ";";
    dataString += String(gps.date.day());
    dataString += ".";
    dataString += String(gps.date.month());
    dataString += ".";
    dataString += String(gps.date.year());
    dataString += ";";
    dataString += String(gps.satellites.value());
    dataString += ";";
    dataString += String(gps.location.lat(), 6);      // 6 Kommastellen
    dataString += ";";
    dataString += String(gps.location.lng(), 6);
    dataString += ";";
    dataString += String(gps.altitude.meters(),2);    // 2 Kommastellen
    dataString += ";";
    dataString += String(gps.speed.kmph(),2);
    //dataString += ";";
    //dataString += String(gps.satellites.isValid());
              // print to the serial port too:
    //Serial.println("Stunde;Minute;Sekunde;Tag;Monat;Jahr;Sat_Anzahl;Latitute;Longtitute;Hoehe;Geschwindigkeit");
    Serial.println(dataString);
    //Serial.println(dateTime);
  
}
//***************************************************************************
void GPS_Values (void){
  
  unsigned long chars = 0;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      int c = ss.read();
      ++chars;
      if (gps.encode(c)) // new valid sentence?
        newData = true;
        
    }
  }
}

void Check_Update(void){
  byte Update=0;
  if(Status==RECORD||Status==WAIT_UPDATE){
    Update=gps.location.isUpdated();
    Update&=gps.time.isUpdated();
    Update&=gps.speed.isUpdated();
    Update&=gps.altitude.isUpdated();
    if(Update!=0){
      Status=RECORD;
    }else{
      Status=WAIT_UPDATE;
    }
  }
}

void State_Sat (void){
   if(gps.satellites.value()>Satellit_min&&gps.satellites.isValid()&&Status==WAIT_SATELLIT){   // Überprüfen ob genug Satelliten da sind
     Status=CREATE_FILENAME;
   }
    if(gps.time.isUpdated()&&Status==CREATE_FILENAME){                                          // Dateinamen erstellen
    String stringtemp;
    
    stringtemp+=String((gps.time.hour()+SW));       //hour = UTC  +1 Wintertime  +2 Summertime
    stringtemp+=String("_");
    stringtemp+=String(gps.time.minute());
    stringtemp+=String(".txt");
    stringtemp.toCharArray(filename, sizeof(filename));
    Serial.print(F("RECORD"));
    Status=RECORD;
    }
}

// LED Status Ausgabe. Sobald eine Blinkfrequenz abgearbeitet wird "true" zurück gegeben.
byte LED_Flash (byte nr_blink, byte pause){
  static byte cnt=0;
  static boolean toogle=true;    
  if(cnt<(nr_blink*4)&& !(cnt%2)){
      if(toogle==true){
        digitalWrite(LED, HIGH);  // LED zwei Funktionsdurchläufe an
        toogle=false;
     }else{
        digitalWrite(LED, LOW);   // LED zwei Funktionsdurchläufe aus
        toogle=true;
      }
  }

  if((cnt-(nr_blink*4))>=(pause-1)){  // Überprüfen ob ein Status Durchlauf komplett abgeschlossen ist
    cnt=0;
    return true; 
  }else{
    cnt++;
  }
}

// Setzt den Status der LED gleich dem des Datenloggers
void Chance_LED_Status (byte LED_Flash_true, byte Statustmp){ 

  if(LED_Flash_true==true){
    LED_Status=(Status_t)Statustmp;
  }
}

// LED Blinksequenz gemäß dem aktuellen Gerätestatus setzen
void Status_LED (void){

  switch(LED_Status){
      
    case CREATE_FILENAME: 
    Chance_LED_Status(LED_Flash(4,6), Status);
    break;
    
    case WAIT_SATELLIT: 
    Chance_LED_Status(LED_Flash(2,6), Status); 
    break;
    
    case SD_ERROR: 
    Chance_LED_Status(LED_Flash(3,6), Status);
    break;
    
    case RECORD: 
    Chance_LED_Status(LED_Flash(1,6), Status);
    break;

    case WAIT_UPDATE: 
    Chance_LED_Status(LED_Flash(5,6), Status);
    break;    
    
    default: 
    Chance_LED_Status(LED_Flash(6,6), Status);
    break;
  } 
}

// Nötige GPS Einstellungen werden gesetzt
void GPS_Setup (void){
  ss.begin(9600);
  unsigned char Set_Baud[]={0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 
                            0x00, 0xE1, 0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDE, 0xC9};  // 57600 Baud einstellen                            
  ss.write(Set_Baud,sizeof(Set_Baud));
  delay(50);
  ss.begin(57600);                     // Ab jetzt ist die Baud auf 57600 im GPS eingestellt, deshalb auch im Arduino ändern
  unsigned char Set_Update[]={0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 
                              0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A};  // 5Hz Baud einstellen
  ss.write(Set_Update,sizeof(Set_Update)); 
  delay(50);
  unsigned char Set_Message1[]={0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 
                               0xF0, 0x02, 0x00, 0xFC, 0x13};             // GPGSA deaktivieren
  ss.write(Set_Message1,sizeof(Set_Message1)); 
  delay(50);
  unsigned char Set_Message2[]={0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 
                                0xF0, 0x03, 0x00, 0xFD, 0x15};            // GPGSV deaktivieren
  ss.write(Set_Message2,sizeof(Set_Message2)); 
  delay(50);
  unsigned char Set_Message3[]={0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 
                                0xF0, 0x01, 0x00, 0xFB, 0x11};            // GPGLL deaktivieren
  ss.write(Set_Message3,sizeof(Set_Message3)); 
  delay(50);  
}

