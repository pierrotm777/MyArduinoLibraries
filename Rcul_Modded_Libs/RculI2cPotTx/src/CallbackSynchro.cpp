#include "CallbackSynchro.h"

/*
 * NOTE DE MESURE PTR-6A / GDO0
 * --------------------------------
 * Les essais reels montrent deux largeurs typiques du paquet court GDO0 :
 * environ 804..816 us et 964..984 us, alors que son debut reste cale sur un
 * cycle RF voisin de 20 ms.
 *
 * Si la periode est mesuree entre les FRONTS DESCENDANTS des paquets courts,
 * un changement de largeur du paquet se retrouve directement dans la mesure :
 *
 *   paquet precedent ~968 us -> paquet suivant ~812 us
 *      => periode apparente legerement inferieure a 20 ms (~19.8 ms)
 *
 *   paquet precedent ~812 us -> paquet suivant ~968 us
 *      => periode apparente legerement superieure a 20 ms (~20.17 ms)
 *
 * Ce n'est donc pas une derive du cycle RF, mais simplement l'effet de la
 * variation de largeur du paquet sur la position de son front descendant.
 *
 * Choix retenu :
 *   - la SYNCHRO RCUL reste declenchee au FRONT DESCENDANT du paquet court ;
 *     on sait alors que sa largeur est valide et on se place juste apres le
 *     paquet ID, avant les trois paquets DATA suivants ;
 *   - le DIAGNOSTIC de cycle est calcule entre les FRONTS MONTANTS de deux
 *     paquets courts valides. Il mesure ainsi le vrai espacement du debut des
 *     paquets et doit rester voisin de 20000 us, independamment de leur largeur.
 *
 * BALAYAGE DE PHASE (V1.10.5)
 * ---------------------------
 * Les premiers essais a offset 0 us montrent encore des erreurs sporadiques
 * de longueur/checksum. Le fait que R=3 puisse etre meilleur que R=4 indique
 * qu'il faut rechercher une meilleure PHASE plutot que seulement augmenter
 * le nombre de repetitions.
 *
 * Le top RCUL peut donc etre retarde, sans blocage, apres le front descendant
 * du paquet court. Balayage conseille: 0..3500 us par pas de 500 us, puis
 * affinage autour de la meilleure zone par pas de 100 us.
 *
 * VALIDATION PTR-6A (V1.10.6)
 * ---------------------------
 * La fenetre 2000..3500 us a donne 100 % dans les essais. La valeur 2750 us,
 * au centre de cette fenetre, est retenue comme phase par defaut du profil.
 * Avec 2750 us : RepeatNb=2 et filtre RX=0 donnent 100 %. RepeatNb=1 chute
 * nettement et n'est donc pas retenu comme configuration minimale fiable.
 *
 * Le Phase Sweep reste conserve comme outil pour caracteriser un autre
 * emetteur ou une autre implementation RF.
 */

bool RculI2cPotGetCallbackSynchroConfig(
    RculI2cPotCallbackSynchroProfile_t Profile,
    RculI2cPotCallbackSynchroConfig_t &Config)
{
  switch(Profile)
  {
    case CALLBACK_SYNCHRO_PTR_6A:
      Config.ActiveLevel = RCUL_I2C_POT_CALLBACK_PTR_6A_ACTIVE_LEVEL;
      Config.PulseMinUs = RCUL_I2C_POT_CALLBACK_PTR_6A_PULSE_MIN_US;
      Config.PulseMaxUs = RCUL_I2C_POT_CALLBACK_PTR_6A_PULSE_MAX_US;
      Config.MinSyncSpacingUs = RCUL_I2C_POT_CALLBACK_PTR_6A_MIN_SPACING_US;
      return true;

    case CALLBACK_SYNCHRO_NONE:
    default:
      Config.ActiveLevel = HIGH;
      Config.PulseMinUs = 0;
      Config.PulseMaxUs = 0;
      Config.MinSyncSpacingUs = 0;
      return false;
  }
}
