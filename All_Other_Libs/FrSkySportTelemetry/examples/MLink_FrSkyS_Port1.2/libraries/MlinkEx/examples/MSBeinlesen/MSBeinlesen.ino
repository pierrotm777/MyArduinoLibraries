
#include <MLinkEx.h>

MLinkEx mLink(13); //LED 13 blinkt bei jedem Datenpaket


void setup() 
{
  Serial.begin(38400);
  while(!Serial){;}
  if(!mLink.begin(SW_SERIAL))//Rx 10, Tx 11
  {
    Serial.print("Initialisierung fehlgeschlagen!");
    while(1);
  }

}

void loop() 
{
  // put your main code here, to run repeatedly: 
   if(mLink.get())
   {
        if(mLink.Return.einheit_nr > 0)
        {
          Serial.print(mLink.Return.ausgabewert,2);
          Serial.print(" - ");   
          Serial.print(mLink.Return.einheit);
          Serial.print(" - ");   
          Serial.print(mLink.Return.einheit_nr,HEX);
          Serial.print(" - ");   
          Serial.print(mLink.Return.alarm,HEX);
          Serial.print(" - ");   
          Serial.print(mLink.Return.stelle,HEX);
          Serial.print(" - ");   
          Serial.print(mLink.Return.faktor,1);
          Serial.print(" - ");   
          Serial.println(mLink.Return.adresse,HEX); 
        }
        else Serial.println("Keine Daten im Datenstrom");
        
   }
   else Serial.println("Kein Datenstrom");
}