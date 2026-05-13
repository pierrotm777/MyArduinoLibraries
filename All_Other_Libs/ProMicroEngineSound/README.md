# ProMicroEngineSound v0.2.0

Reconstruction fonctionnelle d'une librairie moteur pour Arduino Pro Micro ATmega32u4.

## Ce qui a été corrigé en v0.2.0

Les fichiers `.STA` et `.IDL` ne sont pas des RAW : ce sont de vrais fichiers WAV renommés.
Le fichier fourni `DSL-V12.STA` est reconnu comme :

- `RIFF/WAVE`
- Microsoft PCM
- mono
- 8 bits unsigned
- 16000 Hz

La librairie saute maintenant automatiquement l'en-tête WAV et lit uniquement le bloc `data`.

## Timers utilisés

Après vérification du HEX :

- Timer4 est bien utilisé côté audio PWM. Le HEX écrit dans les registres Timer4, notamment `TCCR4A`, `TCCR4B`, `TCCR4C`, `TCCR4D` et `OCR4D`.
- Le vecteur `TIMER1_COMPA` est également actif dans le HEX. Cette librairie utilise donc Timer1 COMPA pour le tick d'échantillonnage.

Dans cette reconstruction :

- sortie audio : D6 / OC4D / Timer4
- duty audio : `OCR4D`
- tick audio : Timer1 COMPA, par défaut 16000 Hz

## Cible

- Arduino Pro Micro 5V / 16 MHz
- ATmega32u4
- Sortie audio PWM sur D6 / OC4D
- SD en SPI avec CS sur D10 dans l'exemple
- Fichiers audio à la racine de la SD :
  - `/DSL-V12.STA`
  - `/DSL-V12.IDL`

## Format audio accepté

Format principal :

- WAV PCM non compressé
- mono
- 8 bits unsigned
- idéalement 16000 Hz

La librairie accepte aussi encore un RAW 8 bits sans header en dépannage, mais ce n'est plus le format recommandé.

## Exemple minimal

```cpp
#include <SPI.h>
#include <SD.h>
#include <ProMicroEngineSound.h>

void setup() {
  ProMicroEngineSound.begin(10, "/DSL-V12.STA", "/DSL-V12.IDL", 16000);
  ProMicroEngineSound.setPitchRangeQ8(256, 512);
  ProMicroEngineSound.startEngine();
}

void loop() {
  ProMicroEngineSound.setThrottleUs(1500, 1000, 2000);
  ProMicroEngineSound.update();
}
```

## Notes importantes

- Évite les librairies qui utilisent Timer1.
- La sortie D6 doit être filtrée/amplifiée avant haut-parleur.
- Le code est une reconstruction fonctionnelle, pas le source original exact du fichier HEX.
