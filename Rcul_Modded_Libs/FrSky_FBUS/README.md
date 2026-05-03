# FrSky_FBUS v0.9.0

Librairie Arduino/Teensy expérimentale pour lire les trames **FrSky FBUS** d'un récepteur FrSky récent, par exemple **Archer Plus R10+**, avec un **Teensy 4.0** ou un **ESP32**.

Cette version garde la lecture RC/RCUL dans `FrSky_FBUS.h/.cpp` et sépare toute la télémétrie dans :

```text
FrSky_FBUS_Telemetry.h
FrSky_FBUS_Telemetry.cpp
```

## Objectif de cette version

Version **RX + télémétrie séparée + support ESP32** :

- lire les canaux RC envoyés par le récepteur ;
- fonctionner à **460800 bauds** ;
- utiliser un UART **8N1 non inversé** ;
- accepter des trames 16 voies ;
- prévoir expérimentalement les trames 24 voies ;
- fournir les valeurs en brut 11 bits et en microsecondes ;
- répondre aux polls télémétrie FBUS avec VFAS, CURR, T1, T2, RPM et GPS expérimental ;
- ne plus fournir `Rcul.h`, car RCUL est une librairie séparée.

## Structure

```text
FrSky_FBUS/
 ├── FrSky_FBUS.h
 ├── FrSky_FBUS.cpp
 ├── FrSky_FBUS_Telemetry.h
 ├── FrSky_FBUS_Telemetry.cpp
 ├── keywords.txt
 ├── library.properties
 ├── README.md
 └── examples/
```

## Dépendances

Cette librairie dépend de ta librairie RCUL externe :

```cpp
#include <Rcul.h>
```

`Rcul.h` n'est volontairement **pas inclus** dans le dossier `FrSky_FBUS`.

## Point très important : FBUS n'est pas inversé

Contrairement au SBUS classique :

| Protocole | Baudrate typique | Inversion | Sens |
|---|---:|---|---|
| SBUS | 100000 | inversé | RX vers contrôleur |
| F.Port / F.Port2 | 115200 | selon matériel/config | half-duplex |
| FBUS | 460800 | **non inversé** | half-duplex |

Dans cette librairie, l'UART est donc ouvert ainsi :

```cpp
Serial1.begin(460800, SERIAL_8N1);
```

Il n'y a volontairement **aucune inversion SBUS** dans le code.

## Support RCUL

`FrSky_FBUS` hérite de `Rcul`, comme ton objet `SBusRx`/`RcBusRx`.

Tu peux donc l'utiliser avec `RcRxSerial` :

```cpp
#include <FrSky_FBUS.h>
#include <RcRxSerial.h>

#define DATA_RC_CHANNEL 5

RcRxSerial MyRcRxSerial(&FrSky_FBUS, RC_FILTER_LEVEL, DATA_RC_CHANNEL, 1);

void setup() {
  FrSky_FBUS.begin(Serial1);
}

void loop() {
  FrSky_FBUS.read();
}
```

Convention conservée comme dans `SBusRx` :

```text
RculGetWidth_us(1) = voie 1
RculGetWidth_us(2) = voie 2
...
```

## Branchement RX only

En lecture seule, le microcontrôleur écoute simplement le fil FBUS du récepteur.

### Teensy 4.0 RX only

```text
Archer Plus R10+ port configuré FBUS  --->  RX1 Teensy 4.0
GND récepteur                         --->  GND Teensy
```

Sur Teensy 4.0 :

```text
Serial1 RX = pin 0
Serial1 TX = pin 1
```

Pour la lecture seule, seule la broche RX est nécessaire.

### ESP32 RX only

Exemple avec `Serial1` et des pins libres :

```text
Archer Plus R10+ port configuré FBUS  --->  RX_PIN ESP32
GND récepteur                         --->  GND ESP32
```

Exemple code :

```cpp
HardwareSerial FbusSerial(1);

#define FBUS_RX_PIN  4
#define FBUS_TX_PIN  7

void setup() {
  FbusSerial.setRxBufferSize(512);
  FrSky_FBUS.begin(FbusSerial, FBUS_RX_PIN, FBUS_TX_PIN);
}
```

Pour la lecture seule, seule la broche RX est réellement utilisée.

## Branchement avec télémétrie

Le FBUS est un bus **1 fil half-duplex** : RX et TX partagent la même ligne.  
Quand la télémétrie est activée, le microcontrôleur doit répondre au récepteur. Il faut donc relier TX au fil FBUS, mais **jamais directement**.

### Pourquoi ajouter une résistance entre TX et la ligne FBUS ?

La résistance série entre TX et le fil FBUS sert à limiter le courant si deux sorties essaient de piloter la ligne en même temps.

Sans résistance :

- le TX du Teensy/ESP32 peut entrer en conflit avec la sortie du récepteur ;
- les trames peuvent être corrompues ;
- le bus peut devenir instable ;
- dans le pire cas, une broche peut être endommagée.

Valeur conseillée :

```text
470 ohms à 1 kOhm
```

Je conseille de commencer avec :

```text
1 kOhm
```

### Teensy 4.0 avec télémétrie

```text
Teensy RX1  <-------------------------+
Teensy TX1 --[470 ohms à 1 kOhm]------+---- FBUS Archer R10+
GND récepteur ----------------------------- GND Teensy
```

Sur Teensy 4.0 :

```text
Serial1 RX = pin 0
Serial1 TX = pin 1
```

Exemple :

```cpp
FrSky_FBUS.begin(Serial1);
FrSky_FBUS_Telemetry.setEnabled(true);
```

### ESP32 avec télémétrie

Exemple avec `Serial1`, RX sur GPIO4 et TX sur GPIO7 :

```text
ESP32 RX_PIN  <-----------------------+
ESP32 TX_PIN --[470 ohms à 1 kOhm]----+---- FBUS Archer R10+
GND récepteur ----------------------------- GND ESP32
```

Exemple :

```cpp
HardwareSerial FbusSerial(1);

#define FBUS_RX_PIN  4
#define FBUS_TX_PIN  7

void setup() {
  FbusSerial.setRxBufferSize(512);
  FrSky_FBUS.begin(FbusSerial, FBUS_RX_PIN, FBUS_TX_PIN);
  FrSky_FBUS_Telemetry.setEnabled(true);
}
```

### Cas RX uniquement

Si tu lis seulement les canaux RC et que tu n'envoies pas de télémétrie :

```text
FBUS RX -> RX microcontrôleur uniquement
```

La résistance n'est pas nécessaire.

Dès que tu actives :

```cpp
FrSky_FBUS_Telemetry.setEnabled(true);
```

la résistance série sur TX devient fortement recommandée.

## Télémétrie v0.8.0

La télémétrie se pilote maintenant par l'objet global :

```cpp
FrSky_FBUS_Telemetry
```

Exemple :

```cpp
#include <FrSky_FBUS.h>
#include <FrSky_FBUS_Telemetry.h>

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1);

  FrSky_FBUS_Telemetry.setVoltage_V(12.60f);
  FrSky_FBUS_Telemetry.setCurrent_A(3.4f);
  FrSky_FBUS_Telemetry.setTemperature1_C(24.0f);
  FrSky_FBUS_Telemetry.setTemperature2_C(42.0f);
  FrSky_FBUS_Telemetry.setRpm(8500);
  FrSky_FBUS_Telemetry.setEnabled(true);
}

void loop() {
  FrSky_FBUS.read();
}
```

Des wrappers sont gardés dans `FrSky_FBUS` pour compatibilité avec les sketches v0.5/v0.6/v0.7 :

```cpp
FrSky_FBUS.setVoltage_V(12.60f);
FrSky_FBUS.setTelemetryEnabled(true);
```

Mais pour les nouveaux sketches, il vaut mieux utiliser directement :

```cpp
FrSky_FBUS_Telemetry.setVoltage_V(12.60f);
FrSky_FBUS_Telemetry.setEnabled(true);
```

## Installation

Copier le dossier `FrSky_FBUS` dans :

```text
Documents/Arduino/libraries/
```

Puis redémarrer l'IDE Arduino.

## Exemple ESP32

```cpp
#include <FrSky_FBUS.h>
#include <FrSky_FBUS_Telemetry.h>

HardwareSerial FbusSerial(1);

#define FBUS_RX_PIN  4
#define FBUS_TX_PIN  7

void setup() {
  Serial.begin(115200);

  FbusSerial.setRxBufferSize(512);
  FrSky_FBUS.begin(FbusSerial, FBUS_RX_PIN, FBUS_TX_PIN);

  FrSky_FBUS_Telemetry.setVoltage_V(12.6f);
  FrSky_FBUS_Telemetry.setCurrent_A(3.4f);
  FrSky_FBUS_Telemetry.setEnabled(true);
}

void loop() {
  FrSky_FBUS.read();
}
```

## Exemple minimal

```cpp
#include <FrSky_FBUS.h>

void setup() {
  Serial.begin(115200);
  FrSky_FBUS.begin(Serial1); // 460800, 8N1, non inversé
}

void loop() {
  if (FrSky_FBUS.read()) {
    for (uint8_t i = 0; i < FrSky_FBUS.channelCount(); i++) {
      Serial.print(FrSky_FBUS.channelUs(i));
      Serial.print('\t');
    }
    Serial.println();
  }
}
```

## API principale

```cpp
FrSky_FBUS.begin(Serial1);
bool ok = FrSky_FBUS.read();
uint8_t count = FrSky_FBUS.channelCount();
uint16_t us = FrSky_FBUS.channelUs(0);
```

## API télémétrie

```cpp
FrSky_FBUS_Telemetry.setEnabled(true);
FrSky_FBUS_Telemetry.setVoltage_V(12.6f);
FrSky_FBUS_Telemetry.setCurrent_A(3.4f);
FrSky_FBUS_Telemetry.setTemperature1_C(24.0f);
FrSky_FBUS_Telemetry.setTemperature2_C(42.0f);
FrSky_FBUS_Telemetry.setRpm(8500);
FrSky_FBUS_Telemetry.setGpsEnabled(true);
FrSky_FBUS_Telemetry.setGps(8, 485123456, 23512345, 12000, 500, 9000);
```

## Remarque importante

La partie télémétrie FBUS reste expérimentale tant qu'elle n'a pas été validée avec un Archer Plus R10+ réel et l'affichage télémétrie côté radio.
