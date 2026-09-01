# Rc3RxSerial 1.1

Receiver RC3 leger et generique, compatible Rcul.

## Nouveaute V1.1 : repetition automatique

Le RX n'a plus besoin de connaitre `RepeatNb` du TX. Le mode AUTO est le mode par defaut.

`Rc3TxSerial` insere 5 trits Idle=1 entre deux trames. Le debut du SYNC `2220` est donc isole et contient exactement :

- Repeat0 : 3 echantillons de trit 2
- Repeat1 : 6
- Repeat2 : 9
- Repeat3 : 12
- Repeat4 : 15

Au passage `2 -> 0`, `Rc3RxSerial` mesure cette longueur et deduit automatiquement `RepeatNb`. Une tolerance de +/-1 echantillon est acceptee. La detection est refaite a chaque SYNC, donc un changement de repetition cote TX est suivi automatiquement.

Le CRC4 reste l'arbitre final de validite de la trame. Si le RX demarre au milieu d'un SYNC et deduit provisoirement une mauvaise repetition, la trame echoue puis le SYNC complet suivant recale automatiquement le decodeur.

## API

Mode AUTO (par defaut) :

```cpp
Rc3RxSerial Rx(&PulseIn);
```

Ou explicite :

```cpp
Rc3RxSerial Rx(&PulseIn, RC3_RX_SERIAL_REPEAT_AUTO);
```

Forcage manuel conserve pour diagnostic/compatibilite :

```cpp
Rx.setRepeatNb(RC3_RX_SERIAL_REPEAT2);
```

Retour au mode automatique :

```cpp
Rx.setAutoRepeat();
```

Diagnostic :

```cpp
Rx.autoRepeatEnabled();
Rx.repeatDetected();
Rx.getRepeatNb();
Rx.getSamplesPerTrit();
```

## Fenetres PWM

Valeurs standard par defaut :

- trit 0 : 800..1200 us
- trit 1 : 1300..1700 us
- trit 2 : 1800..2200 us

Toujours modifiables avec `setPulseWindows()`.

Aucune allocation dynamique, aucun STL.
