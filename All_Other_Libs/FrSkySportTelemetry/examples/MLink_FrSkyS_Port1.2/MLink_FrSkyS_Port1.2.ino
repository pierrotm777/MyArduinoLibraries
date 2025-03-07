/*
  MLink-FrSky translator from T.Pfaff 29.10.2016 V0.2 based on FrSky S-Port Telemetry library
  14.11.2017 W. Zehnder optimized for Sailplanes with RSSI, RxBt, Alt, Dist, Vspeed
  (c) Pawelsky 20160916
  Not for commercial use
  
  Only für ATmega 328P (Arduino Uno o. ProMini 5V)
*/
#include <MLinkEx.h>
#include <avr/io.h>
#include <avr/wdt.h>
MLinkEx mLink(13); //LED 13 blinks on every MLink data set

//Only if MPX Tx-Modul is used uncomment this, comment it for ACT-Tx
//#define _MPX_TX

#include "FrSkySportSensor.h"
#include "FrSkySportSensorAss.h"
#include "FrSkySportSensorFuel.h"
#include "FrSkySportSensorRaw.h"
#include "FrSkySportSensorFcs.h"
#include "FrSkySportSensorFlvss.h"
#include "FrSkySportSensorHdg.h"
#include "FrSkySportSensorT12.h"
#include "FrSkySportSensorRpm.h"
#include "FrSkySportSensorRxBt.h"
#include "FrSkySportSensorSp2uart.h"
#include "FrSkySportSensorDistance.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportSensorA12.h"
#include "FrSkySportTelemetry.h"
#include "SoftwareSerial.h"


FrSkySportSensorAss       ass(    FrSkySportSensor:: ID1);  // Create ASS sensor with default ID
FrSkySportSensorFuel      fuel(   FrSkySportSensor:: ID2);  // Create ASS sensor with default ID
FrSkySportSensorHdg       hdg(    FrSkySportSensor:: ID3);  
FrSkySportSensorRaw       raw(    FrSkySportSensor:: ID4);  // Create a raw sensor for quick processing of vario-value
FrSkySportSensorFcs       fcs1(   FrSkySportSensor:: ID5);  // Create FCS-40A sensor with default ID (use ID8 for FCS-150A)
FrSkySportSensorFcs       fcs2(   FrSkySportSensor:: ID6);  // Create FCS-40A sensor with default ID (use ID8 for FCS-150A)
FrSkySportSensorFlvss     flvss1( FrSkySportSensor:: ID7);  // Create FLVSS sensor with default ID
FrSkySportSensorFlvss     flvss2( FrSkySportSensor:: ID8);  // Create FLVSS sensor with given ID
FrSkySportSensorRpm       rpm(    FrSkySportSensor:: ID9);  // Create RPM sensor with default ID
FrSkySportSensorT12       t12(    FrSkySportSensor:: ID10);  // Create Temperature sensor with default ID
FrSkySportSensorSp2uart   a34(    FrSkySportSensor:: ID11);  // Create SP2UART Type B sensor with default ID
FrSkySportSensorA12       a12(    FrSkySportSensor:: ID12);  // Create A12 sensor
FrSkySportSensorDistance  distance(  FrSkySportSensor:: ID13);  // Create Variometer sensor with default ID but only used for height and distance
FrSkySportSensorRxBt      rxbt(   FrSkySportSensor:: ID14);  // Create RxBt sensor with default ID
FrSkySportTelemetry     telemetry(true);                 // Create telemetry object with polling

void setup()
{
/*
  Serial.begin(38400,SERIAL_8N1);
  while(!Serial){;}
  Serial.print("Testausgabe, sonst nichts");
  delay(1000);    
  // while(1){;}
*/



//Initialisation für MPX-HF by D. Kammerer 26.01.2016 (not yet tested - waiting für feetback) - not necessary for ACT-HF
#ifdef _MPX_TX

  // Nach dem Start 5Sekunden warten und nichts machen, damit das 
  // Sendemodul seinen normalen Betrieb aufnehmen kann.
  delay(5000);  

  // ********************************************************************
  // ** Hier auf den naechste Frame warten und dann aufsynchronisieren **
  // ********************************************************************
  pinMode(0, INPUT);
  // Warten, bis die RX-Leitung M-Link auf Low geht. Denn dann koennte ein Frame anfangen.

 while (digitalRead(0) == 1){};
  // Nun 3ms warten, dann sind die 23Bytes des COM-Ports 
  // 115kBaud auf jedenfall sicher Uebertragen. 
  // 23Bytes dauern ca. 2    ms
  delay(4); // Nach dieser Zeit muessten nun alle Bytes uebertragen sein.
#else
  delay(1000);    
#endif    


  telemetry.begin(FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_2,&ass, &fuel, &hdg, &raw, &fcs1, &fcs2, &flvss1, &flvss2, &t12, &rpm, &a12,&a34, &distance, &rxbt); // festlegen des seriellen Tx-pins 2 .. 12
  
   wdt_enable(WDTO_8S);
  if(!mLink.begin(HW_SERIAL0))
  {
    Serial.begin(9600);
    while(!Serial){;}
    Serial.print("Initialisierung fehlgeschlagen!");
    while(1){;}
  }
   wdt_reset();

   
}
//--------------------------------------------------------------------------------------------
void loop()
{
   wdt_reset();
   static float v_hor         = -1;
   static float dist        = -500,dist_senden;
   static float strom1        = 0;
   static float strom2        = 0;
   static float u_empf_min    = 100000;
   static float u_min         = 100000;   
   static float u_max         = 0;   
   static float u_akku[16]    = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
   static float u_akku_aus[16]= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
   static float temp1         = -273;
   static float temp2         = -273;
   static float hoch          = -5000,hoch_senden;
   static float variowert     = -1;
   static float kap           = -1;
   static float druck         = -1;
   static uint32_t rssi=1,rssi_aus=50; // wenn mit 0 initialisiert und fehlt in MLink - wird kein FrSky update gefahren!
   
   //static uint32_t rssi=1,rssi_aus=1,rssi_mittel[11]={100,100,100,100,100,100,100,100,100,100,100}; // wenn mit 0 initialisiert und fehlt in MLink - wird kein FrSky update gefahren
   static float    vol_rel    = -1;
   static float    kurs       = -1;
   static float    a1         = -1;
   static float    a2         = -1;
   static float    a3         = -1;
   static float    a4         = -1;

   static char strom_adresse1     = -1;
   static char strom_adresse2     = -1;
   static char temp1_adresse      = -1;
   static char temp2_adresse      = -1;
   static char v_hor_adresse      = -1;
   static char dist_adresse     = -1;
   static char hoch_adresse       = -1;
   static char kap_adresse        = -1;
   static char druck_adresse      = -1;
   static char rssi_adresse       = -1;
   static char viol_rel_adresse   = -1;
   static char kurs_adresse       = -1;
   static char upm_adresse        = -1;
   static char a1_adresse         = -1;
   static char a2_adresse         = -1;
   static char a3_adresse         = -1;
   static char a4_adresse         = -1;
   static char raw_adresse        = -1;

   static unsigned int durchlauf_i= 0;

   byte filter=3; // zur Dämpfung des RSSI-Wertes ###
   byte einheit=0;
   byte adresse=0;
   float wert=0;

   wdt_reset();
   if(mLink.get())//reading Mlink stream
   {

     if(mLink.Return.einheit_nr > 0)
     {
         wert= mLink.Return.ausgabewert;
         einheit= mLink.Return.einheit_nr;  
         adresse= mLink.Return.adresse;     
     
    
    bool  ui1_b=false,ui2_b=false,u_rssi_b=false, steighoch_b=false, upmt1t2_b=false, u_b=false, a12_b=false, a34_b=false;
    byte iaus=0;
    switch(einheit)
    {      
      case 0x01: //Spannung [V]
        u_akku[adresse]=wert;
        if(adresse==0)rxbt.setData(19.5*u_akku[0]); // Empfängerspannung   
        else 
        {
          u_b=true; 
          iaus=0;
          for(byte i = 1 ; i < 16 ; i++)
          {
           if(u_akku[i] > -1 && iaus < 12)
            u_akku_aus[iaus++] = u_akku[i];  
          } 
          u_min =  100000;
          u_max = -100000;
          for(byte i = 0 ; i < iaus ; i++)
          {
            if(u_akku_aus[i] > 0 && u_akku_aus[i] <= u_min)
            {
              u_min = u_akku_aus[i];
              ui1_b=true;
            }
            if(u_akku_aus[i] > 0 && u_akku_aus[i] >= u_max )
            {
              u_max = u_akku_aus[i];
              ui2_b=true;
            }
          }    
        }    
       break;
       case 0x02: //Strom [A]
          if(strom_adresse1 == -1 || strom_adresse1 == adresse)
          {
            ui1_b=true;
            strom_adresse1 = adresse;
            strom1 = wert;
          }  
          else if(strom_adresse2 == -1 || strom_adresse2 == adresse)
          {
            ui2_b=true;
            strom_adresse2 = adresse;
            strom2 = wert;
          }  
      break;
      case 0x03: //Steigen [m/s]
            if(raw_adresse == -1 || raw_adresse == adresse)
            {
              raw_adresse = adresse;
              variowert = wert;
/*
Serial.print("wert:");
Serial.print(wert,BIN);
Serial.print(" ");
Serial.print(wert,DEC);
*/
              if(variowert < 0) variowert = -(1638.3+variowert);
/*
Serial.print(" vw:");
Serial.println(variowert,2);
*/
              raw.setData(variowert*10); // um die erste Nachkommastelle zu erhalten
            }
      break;
      case 0x04: //hor. Geschw. [km/h]
          if(v_hor_adresse == -1 || v_hor_adresse == adresse)
          {
            v_hor_adresse = adresse;
            ass.setData(wert*5.5);//correction because OTX value interpreted in unit mph
          }
      break;
      case 0x05: //upm [1/min]
          if(upm_adresse == -1 || upm_adresse == adresse)
          {
            upm_adresse = adresse;
            rpm.setData((uint32_t)wert);
          }  
      break;
      case 0x06: //T [°C]
          if(temp1_adresse == -1 || temp1_adresse == adresse)
          {
            temp1_adresse = adresse;
            temp1=wert;
          }
          else temp1=-273.0;
          if(temp2_adresse == -1 || temp2_adresse == adresse)
          {
            temp2_adresse = adresse;
            temp2=wert;
          }
           else temp2=-273.0;
          t12.setData(temp1, temp2);          
      break;
      case 0x07: //Kurs [°]
          if(kurs_adresse == -1 || kurs_adresse == adresse)
          {
            kurs_adresse = adresse;
            hdg.setData(wert);
          }  
      break;
      case 0x08: //Höhe oder kurze Entfernung [m] 
          if (adresse == 6) // kurze Entfernung muss auf ID 6 liegen ###
          {
            if(dist_adresse == -1 || dist_adresse == adresse)
            {
              steighoch_b=true;
              dist_adresse = adresse;
              dist = wert;
            }  
          }
          else
          {
            if(hoch_adresse == -1 || hoch_adresse == adresse)
            {
              steighoch_b=true;    
              hoch_adresse = adresse;
              hoch = wert;
            }
          }  
          
      break;
      case 0x09: //Kapazität [%]
          if(kap_adresse == -1 || kap_adresse == adresse)
          {
            kap_adresse = adresse;
            fuel.setData(wert);//correction because OTX value interpreted in unit mph
          }
      break;
      case 0x0A:  //RSSI [%]
          if(rssi_adresse == -1 || rssi_adresse == adresse)
          {
            rssi_adresse = adresse;
            rssi = (uint32_t)wert;
            rssi_aus = (rssi_aus*filter+rssi)/(filter+1); // einfacher digitaler Dämpfer für RSSI
          }  
      break;
      case 0x0B: //Kapazität [mAh] not possible in FrSky yet - send over A3
          if(a3_adresse == -1 || a3_adresse == adresse)
          {
            a3_adresse = adresse;
            a3=wert;
            a34_b=true;
          }
      break;
      case 0x0C: //Kapazität [ml] not possible in FrSky yet - send over A4
          if(a4_adresse == -1 || a4_adresse == adresse)
          {
            a4_adresse = adresse;
            a4 = wert;
            a34_b=true;
          }
      break;
      case 0x0D: //Entfernung [km] not possible in FrSky yet - send over A1
          if(a1_adresse == -1 || a1_adresse == adresse)
          {
            a1_adresse = adresse;
            a1 =wert;
            a12_b=true;
          }
      break;
      case 0x0E: //TBD/Druck[TBDF/HPa] not possible in FrSky yet - send over A2
          if(a2_adresse == -1 || a2_adresse == adresse)
          {
            a2_adresse = adresse;
            a2 =wert;
            a12_b=true;
          }
      break;
      case 0x0F: //Volumen [TBDF] not possible in FrSky yet

      break;

    }
 
      if(a12_b)
      {
        a12.setData(a1, a2);
        a12_b=false;
      }
      if(a34_b)
      {
        a34.setData(a3, a4);
        a34_b=false;
      }

      if(ui1_b) 
      {
        fcs1.setData(strom1, u_min);  //current 1 as lowest of all voltages
        ui1_b=false;
      }
      if(ui2_b) 
      {
        fcs2.setData(strom2, u_max);  //current 2 as heighes of all voltages
        ui2_b=false;
      }
                   
      switch(iaus)
      {
       case 1:
          flvss1.setData(u_akku_aus[0]);    
       break;
       case 2:
          flvss1.setData(u_akku_aus[0],u_akku_aus[1]);    
       break;
       case 3:
         flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2]);    
       break;
       case 4:
         flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3]);    
       break;
       case 5:
          flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4]);
       break;
       case 6:
         flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
       break;    
       case 7:
          flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
          flvss2.setData(u_akku_aus[6]);
       break;
       case 8:
        flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
        flvss2.setData(u_akku_aus[6],u_akku_aus[7]);
       break;
       case 9:
        flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
        flvss2.setData(u_akku_aus[6],u_akku_aus[7],u_akku_aus[8]);
       break;
       case 10:
        flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
        flvss2.setData(u_akku_aus[6],u_akku_aus[7],u_akku_aus[8],u_akku_aus[9]);
       break;
       case 11:
        flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
        flvss2.setData(u_akku_aus[6],u_akku_aus[7],u_akku_aus[8],u_akku_aus[9],u_akku_aus[10]);
       break;
       case 12:
        flvss1.setData(u_akku_aus[0],u_akku_aus[1],u_akku_aus[2],u_akku_aus[3],u_akku_aus[4],u_akku_aus[5]);
        flvss2.setData(u_akku_aus[6],u_akku_aus[7],u_akku_aus[8],u_akku_aus[9],u_akku_aus[10],u_akku_aus[11]);
       break;
     }


#define _KORREKTUR_HOEHE_
      if(steighoch_b)
      {
#ifdef _KORREKTUR_HOEHE_ // evtl. Fehler bei der Typwandlung in MLinkEx: mLink.Return.ausgabewert ?

        if(hoch < 0)
          hoch_senden = -(16384.0+hoch);
        else
          hoch_senden =  hoch;

        distance.setData(hoch_senden, dist); 
#else
        distance.setData(hoch, dist); 
#endif
        steighoch_b=false;
      }
      durchlauf_i=0;
    }
    
  }  
  durchlauf_i++;

  if(durchlauf_i < 20)// damit beim Start keine "Telemetrie verloren" Meldung kommt und ggf. kurze Telemetrie-Aussetzer überbrückt werden
    telemetry.send(rssi_aus); 
}
