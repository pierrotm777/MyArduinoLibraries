# TMRpcmSpeed32u4

Lecteur audio simple pour Arduino Pro Micro / Leonardo / ATmega32u4.

## Version v1.3

Cette version garde le nom de la librairie, le nom de classe et l'API existante :

```cpp
#include <TMRpcmSpeed32u4.h>
TMRpcmSpeed32u4 audio;

audio.begin();
audio.play("/DSL-V12.STA");
audio.update();
audio.setPlaybackRate(1.0f);
audio.stop();
```

## Corrections importantes

- Support des vrais fichiers WAV renommés en `.STA` ou `.IDL`.
- Accepte maintenant :
  - PCM 8-bit mono
  - PCM 8-bit stéréo, mixé en mono
  - PCM 16-bit mono, converti en PWM 8-bit
  - PCM 16-bit stéréo, mixé en mono puis converti en PWM 8-bit
- Buffer augmenté à 512 octets pour limiter les underruns.
- Parsing RIFF/WAV amélioré : chunks `fmt `, `data`, chunks inconnus, alignement RIFF.
- Ajout de `getLastError()` pour diagnostiquer pourquoi un fichier ne démarre pas.
- Ajout de `getWavBitsPerSample()` et `getWavChannels()` pour le debug.

## Timers

- Timer4 reste utilisé pour la sortie PWM sur D6 / OC4D.
- La cadence d'échantillonnage est générée par Timer1 ou Timer3.
- Par défaut : Timer3.

Dans ton sketch actuel, tu as déjà :

```cpp
#define TMRPCM_32U4_AUDIO_TIMER 3
#include <TMRpcmSpeed32u4.h>
```

C'est conservé et recommandé.

## Format WAV conseillé pour la meilleure qualité

Même si la lib accepte le 16-bit/stéréo, le meilleur format pour un Pro Micro reste :

- PCM non compressé
- mono
- 8-bit ou 16-bit
- 11025 Hz ou 16000 Hz
- éviter 44100 Hz, trop lourd pour SD + AVR + variation de pitch

## Debug conseillé dans ton sketch

Après `audio.play(fn);`, tu peux faire :

```cpp
if (!audio.play(fn)) {
  Serial.print("audio.play FAIL: ");
  Serial.println(audio.getLastError());
} else {
  Serial.print("WAV Hz="); Serial.print(audio.getWavSampleRate());
  Serial.print(" bits="); Serial.print(audio.getWavBitsPerSample());
  Serial.print(" ch="); Serial.println(audio.getWavChannels());
}
```
