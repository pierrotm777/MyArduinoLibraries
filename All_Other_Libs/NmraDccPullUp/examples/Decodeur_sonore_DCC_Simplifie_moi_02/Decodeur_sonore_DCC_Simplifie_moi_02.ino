







//**************************************************//
//                                                  //
//       Programme pour décodeur SONORE Fixe        //
//             avec carte ARDUINO UNO               //
//                                                  //
//   Les librairies NmraDcc, DFPlayer_Mini_Mp3      //
//                                                  //
//         La base du code provient de :            //
//             UAICF Nevers-Vauzelles               //
//          http://modelisme58.free.fr              //
//            Simplifié par DocMarco                //
//                                                  //
//                 Janvier 2022                     //
//                                                  //
//**************************************************//

/*
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Commande pour gestion des fichiers Sons
  Les fichiers doient être nommés 000x.mp3 et placés dans le repertoire /mp3

   mp3_play ();		                // Joue les sons
   mp3_play (5);	                // Joue "mp3/0005.mp3"
   mp3_next ();		                // Joue fichier suivant 
   mp3_prev ();		                // Joue fichier précédent
   mp3_stop ();                         // STOP l'excution du fichier
   mp3_pause ();                        // Mise en PAUSE du fichier
   mp3_single_loop (true);              // Joue fichier en boucle
   mp3_single_loop (false);             // Stop l'excution du fichier en boucle   
   mp3_set_volume (uint16_t volume);	  // Niveau sonore de 1 à 30

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*/ 

// Bibliothèque à installer : http://mrrwa.org ou http://sourceforge.net/projects/mrrwa/files/MRRwA-2011-12-31.zip/download
#include <NmraDcc.h>
// Le signal DCC est relié à la borne 2 (via l'optocoupleur)
#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// Définition des bornes pour la laiaison série
SoftwareSerial mySerial(11, 10); // RX, TX



// Paramétrage pour signal DCC
NmraDcc  Dcc ;
DCC_MSG  Packet ;

// Adresses DCC des accessoires
int AdresseAccessoire[] = {111,112,113,114,115,116,117}; // ici, faut entrer les adresses qu'on veut utiliser. La première adresse correspond à Play et Pause,la seconde est le volume + et -,
//la troisième adresse lit le premier son en boucle.Donc, si vous avez 10 sons à lire, il vous faudra 13 adresses.

// Paramètres pour gestion des fichiers en mode aléatoire
const int PROGMEM Interval_AleatoireB =   30;     // Durée minimale en secondes entre 2 fichiers son en mode aléatoire
const int PROGMEM Interval_AleatoireH =  200;     // Durée maximale en secondes entre 2 fichiers son en mode aléatoire
const int PROGMEM Fichier1 =  1;                 // Numero minimal du fichier son joué en mode aléatoire
const int PROGMEM Fichier2 =  10;                 // Numero maximal du fichier son joué en mode aléatoire

// Autres variables : Ne pas modifier
int ACCESSOIRE = 0;             // Numéro de l'accessoire commandé
byte ETAT = 0;                  // Position de l'accessoire commandé 
int i;
byte Mode_Aleatoire = 0;
int Bt[13];
unsigned long Millis_Ancien = 0;
unsigned long Millis_Aleatoire = 0; 
const long PROGMEM Interval = 500;
long Interval_Aleatoire = 0;
byte Pause = 0;
#define ROW_COUNT(array)    (sizeof(array) / sizeof(*array))

//*********************************************************************************
// --- Fonction en Digital --------------------------------------------------------

void notifyDccAccState( uint16_t Addr, uint16_t BoardAddr, uint8_t OutputAddr, uint8_t State)
{
  // Mise à 0 de l'adresse pour les accessoires ayant une adresse de 1 à 4  
  if (Addr > 65532 && Addr < 65536 )  Addr = 0 ;
  
  ETAT = (8 * BoardAddr + OutputAddr)%2;
  ACCESSOIRE = 1 + (8 * BoardAddr + OutputAddr)/2 ;

  Serial.print("Notication Adresse et Etat accessoire : ") ;
  Serial.print(ACCESSOIRE,DEC) ;
  Serial.print(' -> ');
  Serial.println(ETAT,DEC) ;
  
  // Commande des accessoires
if (millis() - Millis_Ancien > Interval)
{
  if (ACCESSOIRE == AdresseAccessoire[0] && ETAT == 0 ) { mp3_play ();  Serial.println("Joue les sons"); }
  if (ACCESSOIRE == AdresseAccessoire[0] && ETAT == 1 ) { mp3_pause (); Serial.println("Pause"); }
  
  if (ACCESSOIRE == AdresseAccessoire[1] && ETAT == 1 && i > 0  )
      {
        i--; mp3_set_volume (i);
        EEPROM.write(0, i);
        Serial.print("Niveau sonore (0 -> 30) : "); Serial.println(i); 
      }
      
  if (ACCESSOIRE == AdresseAccessoire[1] && ETAT == 0 && i < 30 )
      {
        i++; mp3_set_volume (i);
        EEPROM.write(0, i);
        Serial.print("Niveau sonore (0 -> 30) : "); Serial.println(i); 
      }
      
  if (ACCESSOIRE == AdresseAccessoire[2]  && ETAT == 0 ) { mp3_play (1); delay (100); mp3_single_loop (true); Serial.println("Joue le fichier 0001.mp3 en boucle"); Mode_Aleatoire = 0;}
  if (ACCESSOIRE == AdresseAccessoire[3]  && ETAT == 0 ) { mp3_play (1); Serial.println("Joue le fichier 0001.mp3"); Mode_Aleatoire = 0;}
  if (ACCESSOIRE == AdresseAccessoire[4]  && ETAT == 0 ) { mp3_play (2); Serial.println("Joue le fichier 0002.mp3"); Mode_Aleatoire = 0;}
  if (ACCESSOIRE == AdresseAccessoire[5]  && ETAT == 0 ) { mp3_play (3); Serial.println("Joue le fichier 0003.mp3"); Mode_Aleatoire = 0;}
  if (ACCESSOIRE == AdresseAccessoire[6]  && ETAT == 0 ) { mp3_play (4); Serial.println("Joue le fichier 0004.mp3"); Mode_Aleatoire = 0;}
 
}
} // Fin Fonction DCC


//*********************************************************************************
// --- Démarrage du progamme - SETUP ----------------------------------------------

void setup()
{
 
  
  delay (100);
  Serial.begin(9600);
  mySerial.begin (9600);
  mp3_set_serial (mySerial);
  
  i = EEPROM.read(0);
  if (EEPROM.read(0) == 255 || EEPROM.read(0) == 0) { EEPROM.write(0, 10); i = 10;}
  delay (100);

  
  Serial.println("DocMarco");
  Serial.println("Lecteur SONORE : Commande en Digital ");
  Dcc.init( MAN_ID_DIY, 10, FLAGS_OUTPUT_ADDRESS_MODE | FLAGS_DCC_ACCESSORY_DECODER | FLAGS_ENABLE_INT0_PULL_UP, 0 );
  Serial.println(" ");
  Serial.print("Niveau sonore (ech 0 -> 30) : "); Serial.println(i); 
  
  int Ligne = ROW_COUNT(AdresseAccessoire);
  
  Serial.println("Adresses pour commande en DCC :");     
  for (int k = 0; k < Ligne; k++)
    {
       Serial.print("     Accesoire DDC = ");
       Serial.println(AdresseAccessoire[k]);
    }

  Serial.println(" ");
  Serial.println("Fichier 0001.mp3 en boucle au demarrage...");
 
  delay (10);
  Serial.println(""); 
  Mode_Aleatoire = 0;
  
  // Son en boucle au démarrage
        mp3_set_volume (i);
        delay (10);
	mp3_play (1);
	delay (100);
	mp3_single_loop (true); 
}

//*********************************************************************************
// --- Boucle du programme Principal ----------------------------------------------
void loop()
{
   Dcc.process();

}
