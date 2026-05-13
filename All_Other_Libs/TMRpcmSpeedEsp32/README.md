# TMRpcmSpeedEsp32 v4.2-XT

Librairie ESP32-S3 pour lire un fichier WAV depuis SD vers un MAX98357A en I2S.

## Base

v3.3 repart du chemin audio de v3.1, jugé plus joli que v3.2.

## Fichier moteur par défaut

```text
/SCAN-V12_16B.IDL
```

Format recommandé :

```text
WAV PCM signed 16-bit stereo 16000 Hz
```

## Rate

`setPlaybackRate()` change la fréquence I2S effective.

Certaines zones de rate peuvent être moins propres selon l'horloge I2S.
Dans l'exemple, on évite volontairement :

```text
0.64 à 0.68
```

et on limite temporairement le max à :

```text
1.30
```

Tu peux modifier ces valeurs dans l'exemple.

## Volume

```cpp
audio.setVolume(100); // niveau original
audio.setVolume(150); // +50 %
audio.setVolume(200); // x2, risque de saturation selon le WAV
```


## v3.4

Ajout du format :

```cpp
TMRPCM_FMT_S16_STEREO_44100
```

et prise en charge automatique par :

```cpp
audio.setFormat(TMRPCM_FMT_AUTO);
```

si le fichier est :

```text
WAV PCM signed 16-bit stereo 44100 Hz
```

Exemple ajouté :

```text
I2S_S16_STEREO_44100_STABLE
```

Fichier attendu dans cet exemple :

```text
/SCAN-V12_IDL.wav
```


## v3.5 mixer

Ajout d'un mixer simple :

```text
1 moteur en boucle
+
3 sons auxiliaires ponctuels
->
mixage logiciel
->
I2S MAX98357A
```

API :

```cpp
audio.playMotor("/SCAN-V12_IDL.wav");
audio.setMotorRate(rate);
audio.setMotorVolume(100);

audio.playAux(0, "/AUX1.wav");
audio.playAux(1, "/AUX2.wav");
audio.playAux(2, "/AUX3.wav");

audio.setAuxVolume(0, 100);
audio.stopAux(0);
```

Important pour cette première version mixer :
- idéalement tous les sons doivent être au même format/fréquence.
- recommandé : WAV PCM signed 16-bit stereo 44100 Hz.
- le pitch/rate agit sur la fréquence I2S globale, donc il agit aussi sur les auxiliaires pendant qu'ils jouent. On corrigera plus tard si besoin avec un vrai resampler moteur uniquement.


## v3.6

Correction mixer : lecture par buffers RAM par piste au lieu de `SD.read()` échantillon par échantillon. Cela doit corriger le son très dégradé au lancement des AUX.


## v3.7

Correction importante mixer :

- L'I2S reste maintenant à la fréquence nominale du fichier, par exemple 44100 Hz.
- `setMotorRate()` ne change plus la fréquence I2S globale.
- Les auxiliaires restent donc à leur vitesse/hauteur normale.
- Le moteur utilise un resampling logiciel simple pour varier sa vitesse/pitch.

Note : le resampling moteur v3.7 est volontairement simple, type nearest-neighbor. On pourra ajouter une interpolation linéaire plus tard si nécessaire.


## v3.9

Base : retour à la v3.7, qui avait un mixer propre.

Correction minimale :
- le volume global `setVolume()` est maintenant appliqué après le mixage et le limiteur.
- le mixer, les buffers et le resampling moteur restent identiques à v3.7.

Tests conseillés :
```cpp
audio.setVolume(30);   // baisse globale
audio.setMotorVolume(100);
audio.setAuxVolume(0, 100);
audio.setAuxVolume(1, 100);
audio.setAuxVolume(2, 100);
```


## v3.10

Correction volume individuel :
- `setMotorVolume(0)` coupe vraiment le moteur.
- `setAuxVolume(slot, 0)` coupe vraiment l'auxiliaire correspondant.
- correction minimale basée sur v3.9.


## v3.12

Base : v3.10.

Objectif : améliorer le `rate` moteur.

Changement :
- le moteur utilise maintenant une interpolation linéaire entre deux frames.
- les AUX restent indépendants du rate moteur.
- l'I2S reste fixe.

Nouvelle option :

```cpp
audio.setMotorInterpolation(true);   // défaut
audio.setMotorInterpolation(false);  // comportement v3.10
```


## v3.13

Base : v3.12.

Ajout d'un compresseur/limiteur doux sur le mix final, utile quand plusieurs AUX jouent ensemble :

```cpp
audio.setCompressor(true);
audio.setCompressor(22000, 4.0f, 100);
```

Réglage conseillé :

```cpp
audio.setVolume(100);
audio.setMotorVolume(80);
audio.setAuxVolume(0, 60);
audio.setAuxVolume(1, 60);
audio.setAuxVolume(2, 60);
audio.setCompressor(20000, 5.0f, 100);
```

Le compresseur est appliqué après le mixage moteur + AUX, donc il évite mieux les saturations que les simples volumes individuels.


## v3.14

Ajout de la normalisation automatique du mix :

```cpp
audio.setAutoMixNormalize(true);  // défaut
```

Principe :
- 1 son actif : niveau normal
- 2 sons actifs : somme divisée par 2
- 3 sons actifs : somme divisée par 3
- 4 sons actifs : somme divisée par 4

Objectif : éviter la saturation quand plusieurs AUX jouent ensemble.


## v3.15

Correction importante pour les parasites au lancement des AUX :

Les auxiliaires peuvent maintenant être préchargés en RAM :

```cpp
audio.preloadAux(0, "/AUX1.wav");
audio.preloadAux(1, "/AUX2.wav");
audio.preloadAux(2, "/AUX3.wav");

audio.playAux(0);   // lecture depuis RAM, sans accès SD au déclenchement
audio.playAux(1);
audio.playAux(2);
```

`playAux(slot, filename)` reste compatible, mais il charge alors le fichier juste avant lecture.
Pour éviter les parasites, préférer `preloadAux()` dans `setup()` puis `playAux(slot)` dans `loop()`.

Avec PSRAM désactivée, cela fonctionne si les AUX ne sont pas trop gros.


## v3.16

Mode hybride RAM + SD pour les auxiliaires.

Objectif :
- effets courts : préchargés en RAM/PSRAM pour éviter les parasites ;
- musiques longues : lecture directe depuis SD, sans tentative de tout charger en mémoire.

### API

```cpp
audio.setAutoPreloadMaxBytes(1024UL * 1024UL); // seuil auto 1 Mo

audio.playAux(0, "/AUX1.wav");       // auto : RAM si petit, SD si gros
audio.playAuxFromSD(1, "/MUSIC.wav"); // force lecture SD directe
audio.preloadAux(2, "/BIP.wav");      // force préchargement RAM
audio.playAux(2);                     // joue le son déjà préchargé
```

### Flash, PSRAM et SD

Il n'est pas utile d'ajouter de mémoire Flash pour les musiques longues.
Les WAV longs doivent rester sur la carte SD.

La PSRAM sert surtout à précharger les petits effets sonores.
Une musique de 4 minutes en 44.1 kHz 16-bit stéréo peut dépasser 40 Mo : impossible à précharger entièrement.


## v3.17

Amélioration du mode hybride RAM + SD :

`preloadAux(slot, filename)` mémorise maintenant toujours le nom du fichier.

Si le fichier est trop gros pour être préchargé ou si `malloc()` échoue, ce n'est plus bloquant :
```cpp
audio.preloadAux(1, "/AUX2.wav"); // peut décider SD fallback
audio.playAux(1);                 // joue depuis RAM si possible, sinon SD
```

Donc la même commande `playAux(slot)` fonctionne pour :
- petit AUX préchargé en RAM/PSRAM ;
- gros AUX ou musique longue depuis SD.


## v3.18

Correction fallback AUX RAM/SD :

- `preloadAux(slot, filename)` mémorise toujours le fichier.
- Si le fichier est trop gros ou si `malloc()` échoue, `playAux(slot)` bascule automatiquement vers la SD.
- `playAux(slot)` ne retourne plus `aux not preloaded` si le chemin fichier est connu.
- Essai `ps_malloc()` si la PSRAM est disponible, puis fallback `malloc()`.

Important :
si le moniteur affiche :

```text
esp_psram: PSRAM enabled but initialization failed
```

alors la PSRAM n'est pas utilisable. La lib bascule donc vers la SD pour les gros AUX.


## v4.2-XT_ENGINE expérimental

Branche inspirée de XT :

- tâche FreeRTOS dédiée pour `i2s_write()`,
- ring buffer de 8 slots audio,
- `update()` produit des blocs mixés,
- la tâche audio consomme les blocs et alimente l'I2S plus régulièrement.

Réglages :

```cpp
audio.setAudioTaskCore(0);
audio.setAudioTaskPriority(5);
audio.setXTChunkFrames(128);
```

Objectif : tester si une tâche I2S dédiée réduit les parasites lors des lectures SD simultanées.


## v4.2b

Correction compilation : ajout des implémentations manquantes XT dans `TMRpcmSpeedEsp32.cpp`.


## v4.2c

Correction linker : bloc XT ajouté explicitement en fin de `TMRpcmSpeedEsp32.cpp`.
