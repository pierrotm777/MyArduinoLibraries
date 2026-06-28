# EspBoatAudio

## High Performance Audio Library for ESP32 / ESP32-S3

---

# 🇫🇷 Français

## Présentation

EspBoatAudio est une librairie audio haute performance conçue pour les ESP32 et ESP32-S3.

Elle a été développée pour les projets de modélisme naval, véhicules RC, simulateurs et systèmes embarqués nécessitant une lecture audio fluide avec plusieurs sons simultanés.

La bibliothèque combine lecture en RAM, préchargement en PSRAM, streaming SD, gestion automatique des priorités et moteurs à pitch variable afin d'obtenir un rendu réaliste tout en restant très légère sur le processeur.

---

## Fonctionnalités

* Lecture multi-voix.
* Jusqu'à 15 voix audio indépendantes.
* Lecture simultanée du moteur, de l'ambiance et de plusieurs effets.
* Streaming SD avec double buffer.
* Préchargement automatique en RAM / PSRAM.
* Cache intelligent des effets sonores.
* Cache dédié aux sons moteur.
* Profils mémoire PSRAM 2 MB et 8 MB.
* Support WAV PCM 16 bits.
* Support WAV IMA ADPCM.
* Pitch variable.
* Contrôle du volume par voix.
* Fondu (Fade In / Fade Out).
* Ducking automatique.
* Priorités entre effets.
* Audio Handles.
* Files d'attente audio (Queues).
* Crossfade moteur et ambiance.
* Compatible ESP32 et ESP32-S3.

---

## Architecture audio

La bibliothèque dispose de plusieurs voix indépendantes :

| Voix            | Utilisation         |
| --------------- | ------------------- |
| VOICE_ENGINE_A  | Moteur principal    |
| VOICE_ENGINE_B  | Crossfade moteur    |
| VOICE_AMBIENT_A | Ambiance principale |
| VOICE_AMBIENT_B | Crossfade ambiance  |
| VOICE_BRIDGE    | Voix auxiliaire     |
| VOICE_RAIN      | Pluie               |
| VOICE_THUNDER   | Orage               |
| VOICE_RANDOM_A  | Sons aléatoires A   |
| VOICE_RANDOM_B  | Sons aléatoires B   |
| VOICE_ANCHOR    | Sons d'ancre        |
| VOICE_ALARM     | Alarmes             |
| VOICE_FX0       | Effets              |
| VOICE_FX1       | Effets              |
| VOICE_FX2       | Effets              |
| VOICE_FX3       | Effets              |

Plusieurs sons peuvent donc être joués simultanément sans interruption.

---

## Formats supportés

EspBoatAudio prend en charge deux formats audio :

### PCM 16 bits

* WAV PCM signé 16 bits
* Mono ou Stéréo
* Fréquence d'échantillonnage de 8 kHz à 48 kHz

Ce format est entièrement compatible avec la bibliothèque mais produit des fichiers plus volumineux et consomme davantage de mémoire.

### IMA ADPCM

* WAV IMA ADPCM
* Mono recommandé
* Fréquence d'échantillonnage de 44,1 kHz recommandée

L'IMA ADPCM réduit fortement la taille des fichiers tout en conservant une excellente qualité sonore. Il est parfaitement pris en charge par EspBoatAudio, y compris pour les sons utilisant un pitch dynamique.

### Quel format choisir ?

Pour les nouveaux projets, **il est recommandé d'utiliser l'IMA ADPCM pour tous les sons**, y compris :

* Sons moteur
* Ambiances
* Alarmes
* USER sounds
* Sons aléatoires
* Canon
* Corne de brume
* Mitrailleuse
* Ancre
* Voix et annonces

Le format PCM reste compatible et peut être utilisé pour des raisons de compatibilité ou pour conserver des fichiers existants.

### Paramètres recommandés

Pour obtenir les meilleures performances avec EspBoatAudio :

| Paramètre | Valeur recommandée |
| --------- | ------------------ |
| Format    | WAV                |
| Codec     | IMA ADPCM          |
| Canaux    | Mono               |
| Fréquence | **44 100 Hz**      |

Cette configuration offre le meilleur compromis entre qualité audio, taille des fichiers, vitesse de chargement et consommation mémoire sur ESP32 et ESP32-S3.

## Conversion avec FFmpeg

Conversion d'un fichier WAV vers le format recommandé :

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a adpcm_ima_wav output.wav
```

Cette commande :

* convertit le fichier en **mono** (`-ac 1`) ;
* fixe la fréquence à **44 100 Hz** (`-ar 44100`) ;
* encode le son au format **WAV IMA ADPCM** (`-c:a adpcm_ima_wav`).

## Conversion automatique d'un dossier (Windows)

Créer un fichier `convert_adpcm.bat` contenant :

```batch
@echo off
if not exist converted mkdir converted

for %%f in (*.wav) do (
    ffmpeg -y -i "%%f" ^
        -ac 1 ^
        -ar 44100 ^
        -c:a adpcm_ima_wav ^
        "converted\%%~nf.wav"
)

echo Conversion terminee.
pause
```

Tous les fichiers `.wav` présents dans le dossier seront convertis automatiquement dans le sous-dossier `converted`.


## Initialisation

```cpp
EspBoatAudio audio;

audio.begin(
    4,   // BCLK
    5,   // LRCK
    6    // DOUT
);

audio.setMasterVolume(1.0f);
```

---

## Profils PSRAM

Deux profils mémoire sont disponibles :

```cpp
audio.setPsramProfile(
    EspBoatAudio::PSRAM_PROFILE_2MB
);
```

ou

```cpp
audio.setPsramProfile(
    EspBoatAudio::PSRAM_PROFILE_8MB
);
```

Le profil adapte automatiquement :

* les limites de cache,
* les stratégies de préchargement,
* l'utilisation de la PSRAM.

---

## Moteur

### Précharger

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

### Crossfade moteur

```cpp
audio.engineCrossfade(
    "/ENGINES/NEW_ENGINE.wav",
    1200
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

### Crossfade

```cpp
audio.ambientCrossfade(
    "/FIXEDSOUND/AMBIENT2.wav",
    1500
);
```

---

## Effets sonores

### Lecture automatique

```cpp
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);
```

La bibliothèque choisit automatiquement :

* lecture RAM/PSRAM si possible ;
* streaming SD sinon.

---

### Lecture avec mise en cache automatique

```cpp
audio.playFxAutoCached(
    "/FIXEDSOUND/CANNON.wav"
);
```

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

* un effet plus prioritaire remplace un effet moins prioritaire ;
* un effet moins prioritaire est refusé.

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

### Arrêt

```cpp
audio.stop(h);
```

---

## Files d'attente audio

Permettent d'enchaîner automatiquement plusieurs sons.

Exemple :

```
START
   ↓
LOOP
```

ou

```
START
   ↓
LOOP
   ↓
STOP
```

Fonctions disponibles :

```cpp
audio.playVoiceQueue(...);

audio.playVoiceThen(...);

audio.chainTick();
```

Utilisé notamment pour :

* moteurs,
* ancre,
* mitrailleuse,
* corne de brume,
* séquences personnalisées.

---

## Fade

```cpp
audio.fadeVoice(
    EspBoatAudio::VOICE_AMBIENT_A,
    0.25f,
    1000
);
```

### Fade Out + Stop

```cpp
audio.fadeOutAndStop(
    EspBoatAudio::VOICE_AMBIENT_A,
    1000
);
```

---

## Ducking

Réduit temporairement le volume d'une voix.

```cpp
audio.duckVoice(
    EspBoatAudio::VOICE_AMBIENT_A,
    0.25f,
    100
);
```

Puis :

```cpp
audio.unduckVoice(
    EspBoatAudio::VOICE_AMBIENT_A,
    500
);
```

---

## Cache audio

### Précharger un effet

```cpp
audio.preloadFxCache(path);
```

### Précharger un effet moteur

```cpp
audio.preloadEngineFxCache(path);
```

### Précharger un dossier

```cpp
audio.preloadFolder("/FIXEDSOUND");
```

### Précharger uniquement les fichiers ADPCM

```cpp
audio.preloadFolderAdpcmOnly("/RANDOM");
```

---

## Debug

Afficher l'état des voix :

```cpp
audio.printVoicesStatus();
```

Afficher le cache FX :

```cpp
audio.printFxCacheStatus();
```

Afficher le cache moteur :

```cpp
audio.printEngineFxCacheStatus();
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

It supports simultaneous playback of multiple sounds while combining RAM playback, PSRAM caching and SD streaming.

The library is designed to keep the real-time audio rendering task as light as possible. SD card access is handled outside the audio task through `streamTick()`, while queue management is handled through `chainTick()`.

This makes it suitable for projects that require engine sounds, ambient sounds, alarms, random sounds and multiple sound effects running at the same time.

---

## Main Features

* Multi-voice playback
* Up to 15 independent audio voices
* Simultaneous engine, ambient, alarm, random and FX playback
* SD streaming with double buffering
* RAM / PSRAM playback
* Intelligent FX cache
* Dedicated engine FX cache
* PSRAM profiles for 2 MB and 8 MB boards
* PCM 16-bit WAV support
* IMA ADPCM WAV support
* Automatic RAM / PSRAM / SD streaming selection
* Priority management for FX voices
* Audio queues
* Audio Handles
* Dynamic pitch control
* Volume control per voice
* Fade In / Fade Out
* Ducking
* Engine crossfade
* Ambient crossfade
* Debug functions for voices and caches
* Compatible with ESP32 and ESP32-S3

---

## Audio Architecture

EspBoatAudio uses several independent voices.

| Voice             | Purpose                                    |
| ----------------- | ------------------------------------------ |
| `VOICE_ENGINE_A`  | Main engine voice                          |
| `VOICE_ENGINE_B`  | Secondary engine voice used for crossfade  |
| `VOICE_AMBIENT_A` | Main ambient voice                         |
| `VOICE_AMBIENT_B` | Secondary ambient voice used for crossfade |
| `VOICE_BRIDGE`    | Auxiliary / bridge voice                   |
| `VOICE_RAIN`      | Rain ambience                              |
| `VOICE_THUNDER`   | Thunder effects                            |
| `VOICE_RANDOM_A`  | Random sounds group A                      |
| `VOICE_RANDOM_B`  | Random sounds group B                      |
| `VOICE_ANCHOR`    | Anchor or continuous special sounds        |
| `VOICE_ALARM`     | Alarm sounds                               |
| `VOICE_FX0`       | General-purpose sound effect               |
| `VOICE_FX1`       | General-purpose sound effect               |
| `VOICE_FX2`       | General-purpose sound effect               |
| `VOICE_FX3`       | General-purpose sound effect               |

The engine and ambient systems use two voices each so that smooth crossfades can be performed without stopping the current loop abruptly.

---

## Supported Audio Formats

EspBoatAudio supports two audio formats.

### PCM 16-bit

* Signed 16-bit PCM WAV
* Mono or Stereo
* Sample rates from 8 kHz to 48 kHz

This format is fully supported but produces larger files and requires more memory.

### IMA ADPCM

* WAV IMA ADPCM
* Mono recommended
* 44.1 kHz sample rate recommended

IMA ADPCM significantly reduces file size while maintaining excellent audio quality. It is fully supported by EspBoatAudio, including sounds using dynamic pitch control.

### Which format should I use?

For new projects, **IMA ADPCM is the recommended format for all sounds**, including:

* Engine sounds
* Ambient loops
* Alarm sounds
* USER sounds
* Random sounds
* Cannon
* Horn
* Machine gun
* Anchor sounds
* Voice prompts and announcements

PCM remains supported for compatibility with existing sound libraries or when preserving original recordings is desired, but it is no longer required for high-quality pitch variation.

### Recommended settings

| Parameter   | Recommended value |
| ----------- | ----------------- |
| Container   | WAV               |
| Codec       | IMA ADPCM         |
| Channels    | Mono              |
| Sample rate | 44.1 kHz          |

---

## Converting WAV files to IMA ADPCM

To convert a WAV file to the recommended format with FFmpeg:

```bash
ffmpeg -i input.wav -ac 1 -ar 44100 -c:a adpcm_ima_wav output.wav
```

This command:

* converts the audio to mono with `-ac 1`;
* resamples it to 44.1 kHz with `-ar 44100`;
* encodes it as WAV IMA ADPCM with `-c:a adpcm_ima_wav`.

### Batch conversion on Windows

Create a file named `convert_adpcm.bat`:

```batch
@echo off
if not exist converted mkdir converted

for %%f in (*.wav) do (
    ffmpeg -y -i "%%f" ^
        -ac 1 ^
        -ar 44100 ^
        -c:a adpcm_ima_wav ^
        "converted\%%~nf.wav"
)

echo Conversion completed.
pause
```

All `.wav` files in the current folder will be converted and saved into the `converted` folder.

---

## Initialization

```cpp
#include <EspBoatAudio.h>

EspBoatAudio audio;

void setup()
{
    audio.begin(
        4,   // BCLK
        5,   // LRCK / WS
        6    // DOUT
    );

    audio.setMasterVolume(1.0f);
}
```

Optional parameters can also be used depending on the board and project:

```cpp
audio.begin(
    4,          // BCLK
    5,          // LRCK / WS
    6,          // DOUT
    44100,      // Output sample rate
    I2S_NUM_0,  // I2S port
    0           // Audio task core
);
```

---

## Master Volume

```cpp
audio.setMasterVolume(1.0f);
```

| Value  | Description    |
| ------ | -------------- |
| `0.0f` | Mute           |
| `1.0f` | Normal volume  |
| `2.0f` | Maximum volume |

---

## PSRAM Profiles

EspBoatAudio provides two PSRAM profiles:

```cpp
audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_2MB);
```

or:

```cpp
audio.setPsramProfile(EspBoatAudio::PSRAM_PROFILE_8MB);
```

The selected profile changes how the cache is used.

### 2 MB profile

The 2 MB profile is conservative. It keeps a larger safety reserve and limits the maximum size of cached files.

This profile is recommended for boards with limited PSRAM.

### 8 MB profile

The 8 MB profile allows larger files and more sounds to be cached.

This profile is recommended for ESP32-S3 boards with 8 MB PSRAM.

---

## Engine Playback

### Preload an engine loop

```cpp
audio.preloadEngineLoop(
    "/ENGINES/DSL-TURB_IDL.wav",
    1.0f,   // volume
    1.0f,   // pitch
    255     // priority
);
```

### Start the preloaded engine loop

```cpp
audio.playPreloadedEngineLoop();
```

### Change engine pitch

```cpp
audio.engineSetPitch(1.50f);
```

### Change engine volume

```cpp
audio.engineSetVolume(1.0f);
```

### Stop the engine immediately

```cpp
audio.engineStop();
```

### Generic engine stop

```cpp
audio.engineGenericStop(
    1800,   // fade duration in ms
    0.45f   // target pitch
);
```

This fades out the active engine voice and can also lower the pitch during the stop phase.

---

## Engine Crossfade

The engine system uses two voices: `VOICE_ENGINE_A` and `VOICE_ENGINE_B`.

This allows smooth transitions between two engine loops.

```cpp
audio.engineCrossfade(
    "/ENGINES/OTHER_ENGINE.wav",
    1000,   // crossfade duration in ms
    1.0f,   // volume
    1.0f,   // pitch
    255,    // priority
    false   // false = RAM/cache playback, true = streaming
);
```

---

## Ambient Playback

### Start ambient loop

```cpp
audio.ambientPlayLoop(
    "/FIXEDSOUND/AMBIENT.wav"
);
```

### Change ambient volume

```cpp
audio.ambientSetVolume(0.5f);
```

### Stop ambient

```cpp
audio.ambientStop();
```

---

## Ambient Crossfade

The ambient system uses two voices: `VOICE_AMBIENT_A` and `VOICE_AMBIENT_B`.

This allows smooth transitions between two ambient loops.

```cpp
audio.ambientCrossfade(
    "/FIXEDSOUND/AMBIENT2.wav",
    1500,   // crossfade duration in ms
    0.8f,   // volume
    1.0f,   // pitch
    200,    // priority
    true    // true = streaming
);
```

---

## Sound Effects

### Automatic playback

```cpp
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);
```

`playFxAuto()` opens the WAV header, checks the codec and file size, then automatically chooses between:

* RAM / PSRAM playback if the file can be cached;
* SD streaming if the file is too large.

### Automatic playback with cache preload

```cpp
audio.playFxAutoCached(
    "/FIXEDSOUND/CANNON.wav"
);
```

`playFxAutoCached()` tries to preload the file into the FX cache before playing it. If it cannot be cached, it falls back to SD streaming.

### Repeat a sound N times

```cpp
audio.playFxRepeatAuto(
    "/WARNING/WARN1.wav",
    3
);
```

### Infinite loop

```cpp
audio.playFxRepeatAuto(
    "/FIXEDSOUND/FAILSAFE.wav",
    0
);
```

A repeat count of `0` means infinite loop.

---

## FX Priority Management

Each FX sound can have a priority.

```cpp
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav",
    1.0f,   // volume
    200,    // priority
    1.0f    // pitch
);
```

When all FX slots are already used:

* a higher-priority sound may replace a lower-priority sound;
* a lower-priority sound is rejected.

This makes it possible to keep critical sounds such as alarms or failsafe warnings audible.

---

## Audio Handles

Most playback functions return an `AudioHandle`.

```cpp
EspBoatAudio::AudioHandle h =
audio.playFxAuto(
    "/FIXEDSOUND/CANNON.wav"
);
```

### Check if the sound is still playing

```cpp
audio.isPlaying(h);
```

### Get playback position

```cpp
audio.positionMillis(h);
```

### Get sound length

```cpp
audio.lengthMillis(h);
```

### Get remaining time

```cpp
audio.remainingMillis(h);
```

### Stop only this sound

```cpp
audio.stop(h);
```

### Check a playback time window

```cpp
if (audio.inWindow(h, 100, 400)) {
    // The sound is currently between 100 ms and 400 ms
}
```

Handles use an internal generation counter. This prevents an old handle from accidentally controlling a new sound that reused the same voice.

---

## Audio Queues

Audio queues allow several sounds to be chained automatically.

They are useful for sequences such as:

* START → LOOP
* START → LOOP → STOP
* Engine start followed by idle loop
* Anchor start, loop and stop
* Machine gun start, loop and stop
* Custom sound sequences

Example structure:

```cpp
EspBoatAudio::QueueItem items[] = {
    { "/FIXEDSOUND/START.wav", false, 1, 1.0f, 1.0f, 200 },
    { "/FIXEDSOUND/LOOP.wav",  true,  0, 1.0f, 1.0f, 200 }
};

audio.playVoiceQueue(
    EspBoatAudio::VOICE_ANCHOR,
    items,
    2
);
```

The next sound in the queue is started by:

```cpp
audio.chainTick();
```

To know when a queue is finished:

```cpp
audio.isVoiceQueueDone(EspBoatAudio::VOICE_ANCHOR);
```

---

## Fade

Fade a voice to a target volume:

```cpp
audio.fadeVoice(
    EspBoatAudio::VOICE_AMBIENT_A,
    0.25f,
    1000
);
```

Fade out and stop:

```cpp
audio.fadeOutAndStop(
    EspBoatAudio::VOICE_AMBIENT_A,
    1000
);
```

Fade an AudioHandle:

```cpp
audio.fadeHandle(
    h,
    0.25f,
    500
);
```

---

## Ducking

Ducking temporarily reduces the volume of a voice or a specific handle.

```cpp
audio.duckVoice(
    EspBoatAudio::VOICE_AMBIENT_A,
    0.25f,
    100
);
```

Restore the volume:

```cpp
audio.unduckVoice(
    EspBoatAudio::VOICE_AMBIENT_A,
    500
);
```

Handle-based ducking is also available:

```cpp
audio.duck(h, 0.25f, 100);

audio.unduck(h, 500);
```

---

## FX Cache

EspBoatAudio can preload sound effects into RAM / PSRAM.

```cpp
audio.preloadFxCache(
    "/FIXEDSOUND/CANNON.wav"
);
```

Pinned sounds can be used for critical effects:

```cpp
audio.pinFx(
    "/FIXEDSOUND/FAILSAFE.wav"
);
```

A pinned sound is kept in the cache whenever possible.

### Preload a list

```cpp
const char* sounds[] = {
    "/FIXEDSOUND/CANNON.wav",
    "/FIXEDSOUND/HORN.wav"
};

audio.preloadFxList(sounds, 2);
```

### Preload a folder

```cpp
audio.preloadFolder("/FIXEDSOUND");
```

### Preload a folder with prefix

```cpp
audio.preloadFolderPrefix(
    "/FIXEDSOUND",
    "SHORT_"
);
```

### Preload only ADPCM files

```cpp
audio.preloadFolderAdpcmOnly("/RANDOM");
```

### Preload only ADPCM files with prefix

```cpp
audio.preloadFolderPrefixAdpcmOnly(
    "/FIXEDSOUND",
    "SHORT_"
);
```

The ADPCM-only preload functions skip non-ADPCM files and also ignore unwanted duplicated files such as files containing `Copie` or starting with `222`.

---

## Engine FX Cache

Engine-related effects can be stored in a separate cache.

```cpp
audio.preloadEngineFxCache(
    "/ENGINES/BEIER/PENICHE/anlassgeraeusch.wav"
);
```

This is useful for engine systems where start, run, deceleration and stop sounds must remain separate from general FX sounds.

The playback system searches both caches when playing a file:

1. FX cache
2. Engine FX cache
3. SD file if not cached

---

## Debug

Display all voice states:

```cpp
audio.printVoicesStatus();
```

Display FX cache status:

```cpp
audio.printFxCacheStatus();
```

Display engine FX cache status:

```cpp
audio.printEngineFxCacheStatus();
```

These functions show active voices, playback mode, priorities, volume, pitch, paths, cache usage, pinned sounds and PSRAM statistics.

---

## Main Loop

SD streaming and audio queues must be serviced regularly.

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

For larger projects, it is recommended to wrap this into a helper function:

```cpp
void pumpAudioStream()
{
    audio.streamTick();
    audio.streamTick();
    audio.streamTick();
    audio.streamTick();

    audio.chainTick();
}
```

Then call it frequently from `loop()`.

The audio rendering task itself does not access the SD card. SD access is performed by `streamTick()`, which must therefore be called often.

---

## Performance Recommendations

* Use IMA ADPCM 44.1 kHz mono for most or all sounds.
* Use PSRAM caching whenever possible on ESP32-S3 boards.
* Use the 8 MB PSRAM profile on ESP32-S3 boards with enough memory.
* Use the 2 MB PSRAM profile for smaller boards.
* Keep critical sounds such as failsafe alarms pinned in the FX cache.
* Call `streamTick()` frequently when using SD streaming.
* Call `chainTick()` regularly when using queues.
* Avoid heavy SD operations inside time-critical code.
* Prefer `playFxAuto()` or `playFxAutoCached()` for general effects.

---

## License

MIT License

Copyright (c) Croky_b
