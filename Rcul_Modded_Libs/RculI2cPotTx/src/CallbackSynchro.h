#ifndef RCUL_I2C_POT_CALLBACK_SYNCHRO_H
#define RCUL_I2C_POT_CALLBACK_SYNCHRO_H

#include <Arduino.h>

/*
 * Profils de synchronisation automatique utilises par le mode CALLBACK.
 *
 * CALLBACK_SYNCHRO_NONE:
 *   comportement historique. Le sketch detecte lui-meme l'evenement puis
 *   appelle RculI2cPotTx.syncPulse().
 *
 * CALLBACK_SYNCHRO_PTR_6A:
 *   profil Pro-Tronik PTR-6A / FlyDream V3 observe sur GDO0 du CC2500.
 *   Le paquet court GDO0 sert de repere d'une trame RF complete.
 */
typedef enum
{
  CALLBACK_SYNCHRO_NONE   = 0,
  CALLBACK_SYNCHRO_PTR_6A = 1
} RculI2cPotCallbackSynchroProfile_t;

/*
 * Description d'un profil base sur la largeur d'une impulsion GPIO.
 * La detection est volontairement generique afin de pouvoir ajouter ensuite
 * d'autres radios sans modifier l'API publique de RculI2cPotTx.
 */
typedef struct
{
  uint8_t ActiveLevel;
  uint16_t PulseMinUs;
  uint16_t PulseMaxUs;
  uint32_t MinSyncSpacingUs;
} RculI2cPotCallbackSynchroConfig_t;

/* Valeur reservee signifiant qu'aucune broche de synchro n'est configuree. */
#define RCUL_I2C_POT_CALLBACK_SYNCHRO_NO_PIN          0xFF

/* Mesures PTR-6A retenues pour le premier profil. */
#define RCUL_I2C_POT_CALLBACK_PTR_6A_ACTIVE_LEVEL     HIGH
#define RCUL_I2C_POT_CALLBACK_PTR_6A_PULSE_MIN_US     600U
#define RCUL_I2C_POT_CALLBACK_PTR_6A_PULSE_MAX_US     1200U
#define RCUL_I2C_POT_CALLBACK_PTR_6A_MIN_SPACING_US   15000UL

/*
 * Phase PTR-6A validee sur emetteur Pro-Tronik PTR-6A :
 *   - front de reference : front descendant du paquet court GDO0 ;
 *   - top RCUL : 2750 us apres ce front ;
 *   - resultat valide : 100 % avec RcTxSerial RepeatNb=2 et filtre RX=0.
 *
 * Cette valeur n'est qu'un defaut de profil. Elle peut etre remplacee dans
 * un sketch par setCallbackSyncOffsetUs(), notamment pour un autre emetteur.
 */
#define RCUL_I2C_POT_CALLBACK_PTR_6A_SYNC_OFFSET_US    2750U

/*
 * Plage de balayage pratique pour rechercher la meilleure phase PTR-6A.
 * L'API setCallbackSyncOffsetUs() n'est pas limitee a cette plage.
 */
#define RCUL_I2C_POT_CALLBACK_PTR_6A_PHASE_MIN_US      0U
#define RCUL_I2C_POT_CALLBACK_PTR_6A_PHASE_MAX_US      3500U
#define RCUL_I2C_POT_CALLBACK_PTR_6A_PHASE_STEP_US     500U

/* Retourne la configuration correspondant au profil demande. */
bool RculI2cPotGetCallbackSynchroConfig(
    RculI2cPotCallbackSynchroProfile_t Profile,
    RculI2cPotCallbackSynchroConfig_t &Config);

#endif
