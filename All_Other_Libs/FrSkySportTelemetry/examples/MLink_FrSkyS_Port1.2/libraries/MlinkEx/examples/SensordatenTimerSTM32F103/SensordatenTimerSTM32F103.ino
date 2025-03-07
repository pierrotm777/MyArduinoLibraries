/*
Methoden:
Konstruktor: MLink(LED_Pin)
Destruktor: ~MLink()
Initialisierung: begin(Schnittsetlle,Aufrufschritte,&<name>)
Callback-Funktionn: void <name>(unsigend char Index)


Die Callback-Funktion hat als Parameter einen Index ( callback(unsigend char Index) ), der den aktuellen Sensorwert-Schritt angibt.
Der Indes wird von 0...Aufrufschritte-1 hochgezählt und kann miitels switch(Index) ausgewertet werden.
Jeder Schritt muss in 2ms abgearbeitet sein (z.B. 2 AD-Wandlungen)
Es sind alle verfügbaren seriellen Hardware- u. Software-Schnittstellen als Übergabe möglich. 

   Einheit 1=V, 2=A, 3=m/s Steigen, 4=km/h, 5=1/min, 6=*C,
            7=*, 8=m, 9=%Sprit, 10=%Signal, 11=mAh, 12=ml, 13=km, 14=tbd, 15=tbd

*/


#include <MLinkEx.h>
//#include <TimerOne.h>


MLinkEx mLink(PA1);

//---------------------------------------------------------------------------------------
void timerIsr()
{
//  float ad0=(float)analogRead(PA0)/100.0;
  //Serial.print(ad0,DEC);
//  switch(mLink.adresse())
//  {
//    case  2: mLink.werte(ad0,    ML_M,     2);break;
//    case  3: mLink.werte(ad0,    ML_MpS,   3);break;
//    case  4: mLink.werte(ad0,    ML_GRADC, 4);break;
//    case  5: mLink.werte(ad0,    ML_GRAD,  5);break;
//    case  6: mLink.werte(ad0,    ML_KMpH,  6);break;
//    case  7: mLink.werte(ad0*1E4,ML_UpMIN, 7);break;
//    case  8: mLink.werte(ad0,    ML_MAH,   8);break;
//    case  9: mLink.werte(ad0,    ML_ML,    9);break;
//    case 10: mLink.werte(ad0,    ML_KM,   10);break;
//    case 11: mLink.werte(ad0,    ML_A,    11);break;
//    case 12: mLink.werte(ad0,    ML_VHKAP,12);break;
//    case 13: mLink.werte(ad0,    ML_HPA,  13);break;
//    case 14: mLink.werte(ad0,    ML_V,    14);break;
//    case 15: mLink.werte(ad0,    ML_TBD15,15);break;    
//  }       
  mLink.senden();

}
//---------------------------------------------------------------------------------------
void setup() 
{
    Serial.begin(115200);
    while(!Serial){;}

  if(!mLink.begin(HW_SERIAL1)) // Rx 10, Tx 11
  {

    Serial.print("Initialisierung fehlgeschlagen!");
  }
  else
  {

    Serial.println("Start...");    
  }
  //Da die vom Slave gesendeten Daten auch wieder se´lbst empfamgen werden müsen bis zu 5 bytes
  // gelesen werden (3 Sendebtes, ein Abschussbyte und das Adressbyte vom Master) bis die Adresse vom
  //Maser gefunden werden kann 0> 5x abtasten je 6ms-Datenframe = 1.2ms
  Timer2.setChannel1Mode(TIMER_OUTPUTCOMPARE);
  Timer2.setPeriod(1200); // in microseconds
  Timer2.setCompare1(1);      // overflow might be small
  Timer2.attachCompare1Interrupt(timerIsr);

 }
//---------------------------------------------------------------------------------------
void loop() 
{
float ad0=(float)analogRead(PA0)/100.0;
mLink.werte(ad0,    ML_M,     2);
mLink.werte(ad0,    ML_MpS,   3);
mLink.werte(ad0,    ML_GRADC, 4);
mLink.werte(ad0,    ML_GRAD,  5);
mLink.werte(ad0,    ML_KMpH,  6);
mLink.werte(ad0*1E4,ML_UpMIN, 7);
mLink.werte(ad0,    ML_MAH,   8);
mLink.werte(ad0,    ML_ML,    9);
mLink.werte(ad0,    ML_KM,   10);
mLink.werte(ad0,    ML_A,    11);
mLink.werte(ad0,    ML_VHKAP,12);
mLink.werte(ad0,    ML_HPA,  13);
mLink.werte(ad0,    ML_V,    14);
mLink.werte(ad0,    ML_TBD15,15);    
      
}
