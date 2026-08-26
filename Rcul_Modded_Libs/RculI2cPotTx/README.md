# RculI2cPotTx

Bibliothèque Arduino permettant à `RcTxSerial` d’émettre un flux **RCUL / X-Any** à travers une entrée analogique de radiocommande, en remplaçant le potentiomètre de la voie par un potentiomètre numérique I²C.

Version documentée : **1.10.6**




## Nouveautés de la version 1.10.6

- Le profil `RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A` applique maintenant automatiquement un offset de **2750 us**.
- Aucun `setCallbackSyncOffsetUs(2750)` n'est nécessaire dans un sketch PTR-6A normal.
- La valeur reste modifiable par `setCallbackSyncOffsetUs()` pour un autre émetteur ou un autre point de fonctionnement.
- Le test `RculI2cPotTx_CALLBACK_PTR_6A_Test` affiche la phase réellement active.
- L'exemple `RculI2cPotTx_Nano_PTR6A_PhaseSweep_SW8` est conservé : il écrase volontairement l'offset par défaut et permet de balayer la phase pour caractériser d'autres émetteurs.
- Résultat de référence PTR-6A : **2750 us / RepeatNb=2 / filtre RX=0 / 100 %** lors des essais de validation.

## Nouveautés de la version 1.10.5

- Ajout d'un offset de phase CALLBACK **non bloquant**.
- Nouvelle API :
  ```cpp
  RculI2cPotTx.setCallbackSyncOffsetUs(1500);
  uint16_t Offset = RculI2cPotTx.getCallbackSyncOffsetUs();
  ```
- `0 us` conserve le comportement V1.10.4.
- Exemple Nano PTR-6A + PCF8574A SW8 avec balayage `0..3500 us` par pas de `500 us`, puis réglage fin par pas de `100 us`.
- `printInfo()` affiche maintenant `Sync offset`.

## Nouveautés de la version 1.10.4

- Tous les fichiers source `.h` et `.cpp` de la bibliothèque sont désormais rangés dans le dossier `src/`. Les exemples, images, `README.md`, `keywords.txt` et `library.properties` restent à leur emplacement précédent.
- Profil PTR-6A : le top RCUL reste généré à la fin (front descendant) du paquet court GDO0.
- Le diagnostic `getCallbackCycleUs()` mesure maintenant la période entre les **fronts montants** de deux paquets courts valides, afin que les variations de largeur observées (~804..816 us ou ~964..984 us) ne créent plus artificiellement des mesures autour de ~19.8 / ~20.17 ms.
- Un commentaire détaillé expliquant ce choix est ajouté dans `src/CallbackSynchro.cpp`.

## Nouveautés de la version 1.10.3

- Ajout de la structure de profils **CALLBACK_SYNCHRO**.
- Premier profil : **PTR-6A**, basé sur le signal `GDO0` du CC2500 observé avec FlyDream V3.
- Nouveau marqueur de constructeur : `RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A`.
- Le troisième argument du constructeur devient simplement la broche de synchronisation, par exemple `GDO0_PIN`.
- Détection portable par `digitalRead()` + `micros()` : aucun Timer1, aucune ISR spécifique AVR, donc même logique sur Nano, ESP32 et Teensy.
- Le mode `RCUL_I2C_POT_SYNCRO_BY_CALLBACK` historique reste inchangé et continue d'utiliser `syncPulse()` fourni par le sketch.
- Profil PTR-6A actuel : impulsion active HIGH `600..1200 us`, garde minimale `15 ms`, top au front descendant du paquet court.

Exemple d'initialisation PTR-6A :

```cpp
static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A,
    GDO0_PIN
);
```

Pour revenir au PPM IN, seule la source de synchronisation change :

```cpp
static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    TinyCppmReader
);
```


## Nouveautés de la version 1.10.2

- Ajout du **MCP4661** double potentiomètre numérique.
- Deux canaux indépendants : `RCUL_I2C_POT_TX_MCP4661_CHANNEL_0` et `RCUL_I2C_POT_TX_MCP4661_CHANNEL_1`.
- 257 positions par canal (`0..256`).
- Adresse I²C par défaut `0x28` (`A2=A1=A0=0`), adresse explicite toujours possible.
- Écriture uniquement des wipers **volatils** du MCP4661 : les transmissions RCUL répétitives n'écrivent pas dans son EEPROM interne.
- Conservation intégrale des fonctions V1.10.1 : MCP4561, DS3502, AD5282, INTERNAL, PPMIN, CALLBACK, calibration et stockage des tables.
- Ajout d'exemples MCP4661 double canal pour **Nano/ATmega328P** et **ESP32**.

## Nouveautés de la version 1.10.1

- Support du **MCP4561**, **DS3502** et **AD5282**.
- Modes de synchronisation **INTERNAL**, **PPMIN** et **CALLBACK**.
- Calibration automatique de la table RCUL via `RculPWMRead`.
- Sauvegarde automatique de la table calibrée (EEPROM AVR / Preferences ESP32).
- Rechargement optionnel avec `useStoredRculTable()`.
- Support de l'AD5282 double RDAC (canaux 0 et 1).


## 1. Objectif

`RcTxSerial` sait transformer un message binaire en une suite de 18 niveaux RC :

- `0` à `F` pour les 16 valeurs d’un nibble ;
- `R` pour répéter le symbole précédent ;
- `I` pour marquer l’état Idle et la fin de message.

Sur une radio classique, ces niveaux sont normalement produits par le firmware de l’émetteur. `RculI2cPotTx` permet de les produire depuis un microcontrôleur externe en pilotant un potentiomètre numérique raccordé à l’entrée analogique d’une voie de la radio.

Chaîne complète validée :

```text
MCP23017 / capteurs / commandes
              |
              v
       microcontrôleur
              |
              v
       RculI2cPotTx
              |
              v
MCP4561 / DS3502 / AD5282 / MCP4661
              |
              v
 entrée analogique de la radio
              |
              v
    liaison RF de la radio
              |
              v
 sortie PWM du récepteur
              |
              v
        RcRxSerial
```

## 2. Composants pris en charge

### MCP4561

- potentiomètre numérique I²C ;
- 257 positions, de `0` à `256` ;
- adresse par défaut utilisée par la bibliothèque : `0x2C` ;
- plage de curseur par défaut : `4..252` ;
- conversion linéaire par défaut : `880..2160 µs`.

### DS3502

- potentiomètre numérique I²C ;
- 128 positions, de `0` à `127` ;
- adresse par défaut : `0x28` ;
- plage de curseur par défaut : `0..127` ;
- mode volatile activé automatiquement ;
- protection de l’EEPROM contre les écritures répétitives ;
- table de correction RCUL à 18 points intégrée.

La bibliothèque écrit `0x80` dans le registre de contrôle `0x02` du DS3502. Les changements rapides du registre de curseur restent ainsi volatils et n’usent pas l’EEPROM.

### AD5282

- double potentiomètre numérique I²C ;
- 256 positions par canal, de `0` à `255` ;
- deux RDAC indépendants à la même adresse I²C ;
- adresse par défaut utilisée par la bibliothèque : `0x2C` ;
- sélection par `RCUL_I2C_POT_TX_AD5282_CHANNEL_0` ou `RCUL_I2C_POT_TX_AD5282_CHANNEL_1`.

### MCP4661

- double potentiomètre numérique I²C ;
- 257 positions par canal, de `0` à `256` ;
- deux wipers indépendants à la même adresse I²C ;
- adresse par défaut utilisée par la bibliothèque : `0x28` ;
- plage de curseur par défaut utilisée par `RculI2cPotTx` : `4..252` ;
- sélection par `RCUL_I2C_POT_TX_MCP4661_CHANNEL_0` ou `RCUL_I2C_POT_TX_MCP4661_CHANNEL_1` ;
- la bibliothèque écrit uniquement les registres de wiper volatils `0x00` et `0x01`, sans solliciter l'EEPROM interne pendant l'émission RCUL.

## 3. Microcontrôleurs

La bibliothèque sélectionne automatiquement le pilote I²C :

| Plateforme | Pilote |
|---|---|
| ATtiny45 / ATtiny85 | `TinyWireM` |
| Uno, Nano et autres AVR | `Wire` |
| ESP32 / ESP32-C3 / ESP32-S3 | `Wire` |
| Autres plateformes Arduino compatibles | `Wire` |

Sur ATtiny85, les arguments SDA et SCL de `begin()` sont conservés pour maintenir une API commune, mais `TinyWireM` utilise les broches matérielles imposées par le cœur et la bibliothèque.

## 4. Installation

Copier le dossier `RculI2cPotTx` dans le dossier des bibliothèques Arduino :

```text
Documents/Arduino/libraries/RculI2cPotTx
```

Dépendances :

- `Rcul`;
- `SoftRcPulseIn`;
- `RculPWMRead`;
- `RcTxSerial` pour l’émission des messages ;
- `Wire`, fourni avec Arduino ;
- `TinyWireM` uniquement pour ATtiny45/85.

Redémarrer l’IDE Arduino après installation ou remplacement de la bibliothèque.

## 5. Création de l’objet

### Compatibilité avec le code historique

L’instance globale par défaut reste un MCP4561 :

```cpp
#include <RculI2cPotTx.h>

RcTxSerial MyRcTxSerial(
    &RculI2cPotTx,
    5,
    8,
    5
);
```

Le code historique utilisant directement `RculI2cPotTx` reste donc compatible.

### Choix explicite du DS3502

La sélection doit être faite avant l’inclusion du fichier d’en-tête :

```cpp
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

static RculI2cPotTxClass RculI2cPotTx(DS3502);
```

Il ne faut pas déclarer cette instance `static` sans définir auparavant `RCUL_I2C_POT_TX_CUSTOM_INSTANCE`, car le fichier d’en-tête déclarerait aussi l’instance globale externe du même nom.

### Choix explicite du MCP4561

```cpp
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

static RculI2cPotTxClass RculI2cPotTx(MCP4561);
```

### Choix explicite de l'AD5282

```cpp
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

static RculI2cPotTxClass Pot0(
    AD5282,
    RCUL_I2C_POT_SYNCRO_INTERNAL,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_0);

static RculI2cPotTxClass Pot1(
    AD5282,
    RCUL_I2C_POT_SYNCRO_INTERNAL,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_1);
```

### Choix explicite du MCP4661

```cpp
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>

static RculI2cPotTxClass Pot0(
    MCP4661,
    RCUL_I2C_POT_SYNCRO_INTERNAL,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_0);

static RculI2cPotTxClass Pot1(
    MCP4661,
    RCUL_I2C_POT_SYNCRO_INTERNAL,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_1);
```

Les deux objets d'un composant double partagent la même adresse I²C ; le dernier argument sélectionne le wiper/RDAC interne.

## 6. Initialisation

Signature principale :

```cpp
bool begin(
    uint8_t SdaPin = SDA,
    uint8_t SclPin = SCL,
    uint32_t I2cFrequency = 100000UL,
    uint8_t Address = 0xFF,
    uint8_t PeriodMs = 18,
    uint16_t InitialWidthUs = 1500,
    TwoWire *WirePort = &Wire
);
```

Le dernier argument n’existe pas dans la compilation ATtiny85.

Exemple ESP32-C3 avec DS3502 :

```cpp
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

#define SDA_PIN 5
#define SCL_PIN 6

static RculI2cPotTxClass RculI2cPotTx(DS3502);
static RcTxSerial MyRcTxSerial(&RculI2cPotTx, 5, 8, 5);

void setup()
{
  Wire.begin();

  if(!RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      100000UL,
      RCUL_I2C_POT_TX_AUTO_ADDRESS,
      18,
      1500))
  {
    // Potentiomètre absent ou erreur I²C.
  }
}
```

`RCUL_I2C_POT_TX_AUTO_ADDRESS` choisit automatiquement :

- `0x2C` pour le MCP4561 ;
- `0x28` pour le DS3502 ;
- `0x2C` pour l’AD5282 ;
- `0x28` pour le MCP4661.

Une adresse différente peut être passée explicitement.

## 7. Boucle principale

Les appels importants sont :

```cpp
void loop()
{
  RculI2cPotTx.process();

  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[2];

    Message[0] = (uint8_t)(Contacts >> 8);
    Message[1] = (uint8_t)Contacts;

    MyRcTxSerial.sendNibbleMsg(Message, 4, 1);
  }

  RcTxSerial::process();
}
```

### `RculI2cPotTx.process()`

Cette fonction crée périodiquement un événement de synchronisation RCUL. La période par défaut est de 18 ms.

Elle doit être appelée aussi souvent que possible et ne bloque jamais.

### `RcTxSerial::process()`

Cette fonction demande le symbole suivant à transmettre. Elle appelle virtuellement :

```cpp
RculI2cPotTx.RculSetWidth_us(...)
```

La largeur demandée est convertie en position du curseur, puis envoyée au potentiomètre numérique.

## 8. Synchronisation non bloquante

La classe hérite de `Rcul`.

Toutes les `PeriodMs`, `process()` arme un masque de huit clients :

```cpp
_Synchro = 0xFF;
```

Chaque client appelle ensuite :

```cpp
RculIsSynchro(ClientIdx)
```

Le bit correspondant est consommé une seule fois. Plusieurs instances de `RcTxSerial` peuvent ainsi partager le même générateur de période, à condition d’utiliser des index de client différents.

La correction de la version 1.8.2 permet de recréer un nouvel événement à chaque période, même si certains clients n’ont pas consommé leur bit précédent.

## 9. Codage des symboles RCUL

Les centres théoriques sont :

| Index | Symbole | Largeur |
|---:|:---:|---:|
| 0 | `0` | 1024 µs |
| 1 | `1` | 1080 µs |
| 2 | `2` | 1136 µs |
| 3 | `3` | 1192 µs |
| 4 | `4` | 1248 µs |
| 5 | `5` | 1304 µs |
| 6 | `6` | 1360 µs |
| 7 | `7` | 1416 µs |
| 8 | `8` | 1472 µs |
| 9 | `9` | 1528 µs |
| 10 | `A` | 1584 µs |
| 11 | `B` | 1640 µs |
| 12 | `C` | 1696 µs |
| 13 | `D` | 1752 µs |
| 14 | `E` | 1808 µs |
| 15 | `F` | 1864 µs |
| 16 | `R` | 1920 µs |
| 17 | `I` | 1976 µs |

Formule :

```text
largeur = 1024 + index × 56
```

La fenêtre de décision d’un symbole est approximativement centrée autour de cette valeur avec un demi-pas de 28 µs.

## 10. Table spéciale du DS3502

Une conversion linéaire `0..127` ne produisait pas exactement les centres RCUL après toute la chaîne analogique et RF.

La version 1.8.2 utilise donc cette table :

```cpp
static const uint8_t Ds3502RculWiperTable[18] =
{
   5, 12, 19, 26, 33, 40, 47, 53, 60,
  67, 74, 81, 87, 94,101,108,115,122
};
```

Correspondance :

| Symbole | Wiper |
|:---:|---:|
| `0` | 5 |
| `1` | 12 |
| `2` | 19 |
| `3` | 26 |
| `4` | 33 |
| `5` | 40 |
| `6` | 47 |
| `7` | 53 |
| `8` | 60 |
| `9` | 67 |
| `A` | 74 |
| `B` | 81 |
| `C` | 87 |
| `D` | 94 |
| `E` | 101 |
| `F` | 108 |
| `R` | 115 |
| `I` | 122 |

Cette table est utilisée uniquement lorsque la largeur demandée correspond exactement à l’un des 18 centres RCUL.

Les commandes manuelles, le neutre initial et les autres largeurs continuent à utiliser la conversion linéaire.

## 11. Réglages validés pendant les essais

Chaîne testée :

```text
ESP32-C3
+ MCP23017
+ DS3502
+ radio DIY
+ liaison FrSky
+ récepteur X8R
+ décodeur ESP32-S3
```

Résultats observés :

| Période | Repeat TX | Filtre RX | Résultat |
|---:|---:|---:|---|
| 14 ms | 2 | 0 | fonctionnement variable |
| 16 ms | 2 | 0 | moins bon |
| 18 ms | 2 | 0 | jusqu’à environ 90 % |
| 18 ms | 3 | 0 | environ 95 % |
| 18 ms | 5 | 1 | 100 % observé |

Réglage conseillé pour la fiabilité maximale sur cette configuration :

```cpp
PeriodMs = 18;
RepeatNb = 5;
```

Côté réception :

```cpp
RC_RX_SERIAL_FILTER1
```

Attention : `RcTxSerial` stocke `RepeatNb + 1`. Une valeur `5` correspond donc à six présentations du symbole.

Ces réglages peuvent dépendre de la radio, de son filtrage ADC, du protocole RF et du récepteur.

## 12. API publique

### Constructeur

```cpp
explicit RculI2cPotTxClass(
    RculI2cPotType_t PotType = MCP4561
);
```

Choisit le composant et charge ses paramètres par défaut.

### `begin(...)`

Initialise le bus, vérifie la présence du composant, configure le mode volatile du DS3502 et place le curseur sur la largeur initiale.

Retourne `true` si l’ensemble de l’initialisation réussit.

### `process()`

Produit les tops périodiques de synchronisation. À appeler continuellement.

### `isSynchro(index)`

Retourne `1` une seule fois par période et par client.

### `setWiperRange(min, max)`

Change uniquement la plage de curseur utilisée par la conversion linéaire.

Contraintes :

```text
0 <= min < max <= capacité du composant
```

### `setCalibration(minWidth, maxWidth, minWiper, maxWiper)`

Change simultanément la plage de largeur et la plage de curseur utilisées par la conversion linéaire.

La table RCUL du DS3502 reste prioritaire pour les 18 centres exacts.

### `width_us(width)`

Demande une largeur RC en microsecondes.

La valeur est limitée à la plage interne, convertie en wiper, puis écrite sur le composant.

### `wiper(value)`

Écrit directement une position de curseur.

Une écriture I²C identique à la position courante est évitée.

### Accesseurs

```cpp
getWidth_us()
getWiper()
getMaxWiper()
getI2cAddress()
getPotType()
```

### `isConnected()`

Teste l’acquittement de l’adresse I²C active.

### `printInfo(Stream &Out)`

Affiche la configuration courante :

```text
----------------------------------------
RculI2cPotTx V1.10.2
Pot type     : DS3502
I2C driver   : Wire
I2C address  : 0x28
I2C clock    : 100000 Hz
Wiper        : 60 / 127
Wiper range  : 0 .. 127 (default)
RC width     : 1024 .. 1976 us (internal)
DS3502 mode  : volatile WR (EEPROM protected)
RCUL table   : 18-point radio correction enabled
Connected    : YES
----------------------------------------
```

## 13. Fonctions virtuelles `Rcul`

La classe implémente les trois fonctions attendues par `RcTxSerial` :

```cpp
RculIsSynchro(ClientIdx)
RculGetWidth_us(Ch)
RculSetWidth_us(Width_us, Ch)
```

Le paramètre de voie `Ch` est volontairement ignoré : un objet `RculI2cPotTxClass` représente une seule sortie analogique physique.

## 14. Écriture MCP4561

Le curseur utilise 9 bits :

```cpp
i2cWrite((Wiper >> 8) & 0x01);
i2cWrite(Wiper & 0xFF);
```

La première donnée contient le bit 8, la seconde les huit bits de poids faible.

## 15. Écriture DS3502

Écriture du registre de curseur :

```cpp
i2cWrite(0x00);
i2cWrite((uint8_t)Wiper);
```

Le curseur est limité à `0..127`.

## 16. Gestion des erreurs

`begin()` échoue dans les cas suivants :

- port `Wire` nul ;
- initialisation ESP32 du bus impossible ;
- composant absent à l’adresse choisie ;
- configuration volatile du DS3502 refusée ;
- première écriture de wiper en erreur.

`width_us()` et `wiper()` retournent l’état de l’écriture I²C.

`getWiper()` vaut initialement `0xFFFF` tant qu’aucune écriture réussie n’a été réalisée.

## 17. Adresses I²C

### DS3502

Adresse de base utilisée :

```text
0x28
```

Les broches d’adresse du composant peuvent modifier cette valeur. Utiliser un scanner I²C en cas de doute et passer l’adresse détectée à `begin()`.

### MCP4561

La bibliothèque utilise par défaut :

```text
0x2C
```

L’adresse réelle dépend du modèle exact et du câblage de ses broches d’adresse. Là encore, le scanner I²C fait foi.

### AD5282

Adresse par défaut utilisée par la bibliothèque :

```text
0x2C
```

Les deux RDAC utilisent cette même adresse ; le canal est sélectionné par la commande interne.

### MCP4661

Adresse par défaut utilisée par la bibliothèque :

```text
0x28
```

Avec `A2=A1=A0=0`, l'adresse est `0x28`. Une adresse différente peut être passée explicitement à `begin()`. Les deux wipers utilisent la même adresse ; le canal est sélectionné dans la commande interne.

## 18. Conseils de câblage

- relier toutes les masses ;
- vérifier la tension maximale admissible sur SDA/SCL ;
- éviter des résistances pull-up I²C vers 5 V avec un ESP32 non tolérant 5 V ;
- utiliser un convertisseur de niveau si nécessaire ;
- découpler chaque circuit avec 100 nF au plus près de VCC/GND ;
- garder les liaisons analogiques courtes ;
- relier correctement les trois bornes `RH`, `RW`, `RL` du potentiomètre numérique à la place du potentiomètre de la radio ;
- vérifier le sens de variation ; inverser les extrémités analogiques si nécessaire.

## 19. Débogage recommandé

1. Scanner le bus I²C.
2. Appeler `printInfo(Serial)`.
3. Tester un balayage direct du wiper.
4. Activer le sweep de `RcTxSerial`.
5. Vérifier les 18 centres derrière le récepteur.
6. Tester le mode Message et le checksum.
7. Ajuster seulement ensuite période, repeat et filtre.

Éviter les logs série trop abondants pendant les mesures de qualité : ils peuvent perturber le sketch espion, surtout sur ESP32 lorsque plusieurs impressions sont produites à chaque trame.

## 20. Limites

- La table DS3502 a été mesurée sur une chaîne radio précise.
- Une autre radio peut avoir un gain, un offset ou un filtrage différents.
- Le DS3502 possède 128 positions : la table réserve plusieurs pas entre symboles, mais la précision finale dépend de la chaîne complète.
- La période optimale dépend de la cadence interne de la radio et du récepteur.
- La bibliothèque ne décode pas les messages ; cette fonction appartient à `RcRxSerial`.
- La bibliothèque ne lit pas les contacts ; ils sont fournis par le sketch utilisateur.

## 21. Compatibilité ascendante

La version 1.8.2 conserve :

- l’instance globale `RculI2cPotTx` ;
- le MCP4561 comme choix implicite ;
- les signatures publiques précédentes ;
- la conversion historique du MCP4561 ;
- l’intégration avec `RcTxSerial`.

Le choix explicite du DS3502 est une extension, pas une rupture d’API.

## 22. Licence et origine

`RculI2cPotTx` s’appuie sur l’interface `Rcul` et sur les bibliothèques `RcTxSerial` / `RcRxSerial` de RC Navy.

Vérifier et respecter les licences des bibliothèques d’origine lors de toute redistribution.


## AD5282 (ajoute en V1.10.1)

La bibliotheque gere maintenant trois composants : `DS3502`, `MCP4561` et
`AD5282`. L'AD5282 contient deux potentiometres de 256 positions a la meme
adresse I2C (adresse par defaut `0x2C`).

```cpp
RculI2cPotTxClass Pot0(AD5282, RCUL_I2C_POT_SYNCRO_BY_PPMIN, Cppm32, 0);
RculI2cPotTxClass Pot1(AD5282, RCUL_I2C_POT_SYNCRO_BY_PPMIN, Cppm32, 1);
```

Lorsque deux instances partagent la meme source PPM, attribuer un index de
client distinct a chacune :

```cpp
Pot0.setPpmInSyncClientIdx(5);
Pot1.setPpmInSyncClientIdx(6);
```


---

# Complément V1.10.1

## AD5282

L'AD5282 est maintenant supporté en plus du MCP4561 et du DS3502.

```cpp
static RculI2cPotTxClass Pot0(
    AD5282,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_0);

static RculI2cPotTxClass Pot1(
    AD5282,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    Cppm32,
    RCUL_I2C_POT_TX_AD5282_CHANNEL_1);
```

## Synchronisation

Les modes disponibles sont :

- `RCUL_I2C_POT_SYNCRO_INTERNAL` : période locale ;
- `RCUL_I2C_POT_SYNCRO_BY_PPMIN` : synchronisation fournie par une source `Rcul` telle que `TinyCppmReader` ;
- `RCUL_I2C_POT_SYNCRO_BY_CALLBACK` : callback générique historique, le sketch appelle `syncPulse()` ;
- `RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A` : profil automatique PTR-6A, la bibliothèque détecte directement le paquet court sur `GDO0`.

Le profil PTR-6A ne change pas la nature du mode CALLBACK : il ajoute seulement une méthode intégrée de détection du top. Les profils suivants pourront être ajoutés dans `CallbackSynchro.h/.cpp` sans changer la forme générale du constructeur.

## Calibration automatique

La commande `c` lance la calibration automatique.

La table est :

- calculée,
- utilisée immédiatement,
- sauvegardée automatiquement.

Au démarrage, la table sauvegardée peut être rechargée par :

```cpp
Pot.useStoredRculTable();
```

Pour plusieurs instances ESP32 :

```cpp
Pot0.setTableStoragePreferences("RculAD0");
Pot1.setTableStoragePreferences("RculAD1");
```

---

# Complément V1.10.2

## MCP4661 double canal

Le MCP4661 est ajouté sans supprimer ni modifier le support existant du MCP4561, du DS3502 et de l'AD5282.

```cpp
static RculI2cPotTxClass Pot0(
    MCP4661,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    CppmReader,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_0);

static RculI2cPotTxClass Pot1(
    MCP4661,
    RCUL_I2C_POT_SYNCRO_BY_PPMIN,
    CppmReader,
    RCUL_I2C_POT_TX_MCP4661_CHANNEL_1);
```

Les deux objets utilisent la même adresse I²C. La bibliothèque écrit :

- wiper 0 : registre volatil `0x00` ;
- wiper 1 : registre volatil `0x01`.

Les wipers sont codés sur 9 bits (`0..256`). L'EEPROM interne du MCP4661 n'est pas utilisée pour les changements rapides imposés par RCUL.

### Stockage de deux tables calibrées

Sur Nano/AVR, réserver deux zones EEPROM différentes :

```cpp
Pot0.setTableStorageEEPROM(0);
Pot1.setTableStorageEEPROM(64);
```

Sur ESP32, utiliser deux namespaces différents :

```cpp
Pot0.setTableStoragePreferences("RculMCP0");
Pot1.setTableStoragePreferences("RculMCP1");
```

Puis, au démarrage :

```cpp
Pot0.useStoredRculTable();
Pot1.useStoredRculTable();
```

### Exemples inclus

- `examples/MCP4661_Dual_Nano/MCP4661_Dual_Nano.ino`
- `examples/MCP4661_Dual_ESP32/MCP4661_Dual_ESP32.ino`

