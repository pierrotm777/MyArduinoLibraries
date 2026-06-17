# EspBoatAudio

## High Performance Audio Library for ESP32 / ESP32-S3

---

# 🇫🇷 Français

## Présentation

EspBoatAudio est une librairie audio haute performance pour ESP32 et ESP32-S3 conçue pour les projets de modélisme naval, véhicules RC, simulateurs et systèmes embarqués.

Elle permet de lire simultanément plusieurs sons avec prise en charge du streaming SD, du préchargement PSRAM, des effets sonores prioritaires et des sons moteur à pitch variable.

### Fonctionnalités

* Lecture multi-voix
* Streaming SD
* Préchargement RAM / PSRAM
* WAV PCM 16 bits
* WAV IMA ADPCM
* Gestion des priorités
* Gestion des files d'attente audio
* Contrôle du pitch
* Fade In / Fade Out
* Ducking automatique
* Audio Handles
* Compatible ESP32 et ESP32-S3

---

## Architecture audio

La librairie utilise 7 voix indépendantes :

| Voix          | Utilisation     |
| ------------- | --------------- |
| VOICE_ENGINE  | Moteur          |
| VOICE_AMBIENT | Ambiance        |
| VOICE_ANCHOR  | Sons permanents |
| VOICE_FX0     | Effets          |
| VOICE_FX1     | Effets          |
| VOICE_FX2     | Effets          |
| VOICE_FX3     | Effets          |

---

## Formats supportés

### PCM

* WAV PCM 16 bits
* Mono
* Stéréo
* 8 kHz à 48 kHz

### IMA ADPCM

* WAV IMA ADPCM
* Mono
* Très faible consommation mémoire
* Recommandé pour :

  * Ambiance
  * Alarmes
  * USER sounds
  * Canon
  * Corne de brume
  * Mitrailleuse
  * Sons aléatoires

---

## Initialisation

```cpp
EspBoatAudio audio;

audio.begin(
    4,   // BCLK
    5,   // LRCK
    6    // DOUT
);
```

---

## Volume général

```cpp
audio.setMasterVolume(1.0f);
```

| Valeur | Description |
| ------ | ----------- |
| 0.0    | Muet        |
| 1.0    | Normal      |
| 2.0    | Maximum     |

---

## Moteur

### Précharger le moteur

```cpp
audio.preloadEngineLoop(
    "/ENGINES/DSL-TURB_IDL.wav"
);
```

### Démarrer

```cpp
audio.playPreloadedEngineLoop();
```

### Modifier le régime

```cpp
audio.engineSetPitch(1.50f);
```

### Modifier le volume

```cpp
audio.engineSetVolume(1.0f);
```

### Arrêt immédiat

```cpp
audio.engineStop();
```

### Arrêt progressif

```cpp
audio.engineGenericStop(
    1800,
    0.45f
);
```

---

## Ambiance

### Lecture

```cpp
audio.ambientPlayLoop(
    "/FIXEDSOUND/AMBIENT.wav"
);
```

### Volume

```cpp
audio.ambientSetVolume(0.5f);
```

### Arrêt

```cpp
audio.ambientStop();
```

---

## Effets sonores

### Lecture automatique

```cpp
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);
```

La librairie choisit automatiquement :

* RAM si le fichier est petit
* Streaming SD si le fichier est gros

---

### Répéter N fois

```cpp
audio.playFxRepeatAuto(
    "/WARNING/WARN1.wav",
    3
);
```

---

### Boucle infinie

```cpp
audio.playFxRepeatAuto(
    "/FIXEDSOUND/FAILSAFE.wav",
    0
);
```

---

## Priorités

```cpp
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav",
    1.0f,
    200
);
```

Si tous les slots sont occupés :

* un son plus prioritaire remplace un son moins prioritaire
* un son moins prioritaire est refusé

---

## Audio Handles

```cpp
AudioHandle h =
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);
```

### Vérifier si le son joue

```cpp
audio.isPlaying(h);
```

### Position

```cpp
audio.positionMillis(h);
```

### Durée

```cpp
audio.lengthMillis(h);
```

### Temps restant

```cpp
audio.remainingMillis(h);
```

### Stop

```cpp
audio.stop(h);
```

---

## Files d'attente audio

Permet d'enchaîner plusieurs sons automatiquement.

Exemple :

START → LOOP

```cpp
audio.playVoiceQueue(...);
```

Utilisé notamment pour :

* moteurs
* ancre
* mitrailleuse
* corne de brume

---

## Fade

### Fade vers un volume

```cpp
audio.fadeVoice(
    VOICE_AMBIENT,
    0.25f,
    1000
);
```

### Fade Out + Stop

```cpp
audio.fadeOutAndStop(
    VOICE_AMBIENT,
    1000
);
```

---

## Ducking

Réduit temporairement le volume d'une voix.

```cpp
audio.duckVoice(
    VOICE_AMBIENT,
    0.25f,
    100
);
```

Puis :

```cpp
audio.unduckVoice(
    VOICE_AMBIENT,
    500
);
```

---

## Debug

Afficher l'état complet des voix :

```cpp
audio.printVoicesStatus();
```

---

## Boucle principale

Très important :

```cpp
void loop()
{
    audio.streamTick();
    audio.streamTick();
    audio.streamTick();
    audio.streamTick();

    audio.chainTick();
}
```

---

# 🇬🇧 English

## Overview

EspBoatAudio is a high-performance audio engine for ESP32 and ESP32-S3 designed for RC boats, RC vehicles, simulators and embedded projects.

Main features:

* Multi-voice playback
* SD streaming
* RAM / PSRAM caching
* WAV PCM16 support
* WAV IMA ADPCM support
* Priority management
* Audio queues
* Pitch control
* Fade In / Fade Out
* Ducking
* Audio Handles

---

## Audio Architecture

Available voices:

| Voice         | Purpose          |
| ------------- | ---------------- |
| VOICE_ENGINE  | Engine           |
| VOICE_AMBIENT | Ambient          |
| VOICE_ANCHOR  | Permanent sounds |
| VOICE_FX0     | Effects          |
| VOICE_FX1     | Effects          |
| VOICE_FX2     | Effects          |
| VOICE_FX3     | Effects          |

---

## Initialization

```cpp
EspBoatAudio audio;

audio.begin(4,5,6);
audio.setMasterVolume(1.0f);
```

---

## Engine

```cpp
audio.preloadEngineLoop(
    "/ENGINES/DSL-TURB_IDL.wav"
);

audio.playPreloadedEngineLoop();

audio.engineSetPitch(1.5f);

audio.engineStop();
```

---

## Ambient

```cpp
audio.ambientPlayLoop(
    "/FIXEDSOUND/AMBIENT.wav"
);
```

---

## Sound Effects

```cpp
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);
```

Automatic selection:

* RAM playback for small files
* SD streaming for large files

---

## Infinite Loop

```cpp
audio.playFxRepeatAuto(
    "/FIXEDSOUND/FAILSAFE.wav",
    0
);
```

---

## Audio Handle Example

```cpp
AudioHandle h =
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);

audio.isPlaying(h);

audio.positionMillis(h);

audio.remainingMillis(h);

audio.stop(h);
```

---

## Audio Queue

```cpp
audio.playVoiceQueue(...);
```

Used for:

* Engine START → LOOP
* Anchor START → LOOP → STOP
* Machine gun START → LOOP → STOP

---

## Fade

```cpp
audio.fadeVoice(
    VOICE_AMBIENT,
    0.25f,
    1000
);
```

---

## Ducking

```cpp
audio.duckVoice(
    VOICE_AMBIENT,
    0.25f,
    100
);
```

---

## Debug

```cpp
audio.printVoicesStatus();
```

---

## Main Loop

```cpp
void loop()
{
    audio.streamTick();
    audio.streamTick();
    audio.streamTick();
    audio.streamTick();

    audio.chainTick();
}
```

---

## License

MIT License

Copyright (c) Croky_b
