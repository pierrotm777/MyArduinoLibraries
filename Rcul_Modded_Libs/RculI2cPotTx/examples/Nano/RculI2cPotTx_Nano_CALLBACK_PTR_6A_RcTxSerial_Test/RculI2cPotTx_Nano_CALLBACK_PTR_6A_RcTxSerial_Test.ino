/*
  RculI2cPotTx_Nano_CALLBACK_PTR_6A_RcTxSerial_Test

  Nano + DS3502 + Pro-Tronik PTR-6A
  Synchronisation RCUL sur GDO0 du CC2500 via le profil PTR-6A.

  Cablage Nano :
    DS3502 SDA -> A4
    DS3502 SCL -> A5
    GDO0 PTR-6A -> D8
    GND commun obligatoire

  Test RCUL :
    - envoie un compteur 8 bits : 00, 01, 02, ... FF, 00 ...
    - 2 nibbles utiles + checksum RcTxSerial
    - RCUL_REPEAT = 4 pour le premier essai

  IMPORTANT :
    - aucun print dans loop() par defaut afin de ne pas perturber le polling GDO0;
    - I2C a 400 kHz pour reduire le temps pendant lequel le Nano est occupe par
      une ecriture du DS3502 entre deux appels a RculI2cPotTx.process().
*/

#include <Arduino.h>
#include <Wire.h>

#define RCUL_I2C_POT_DEVICE DS3502
#define RCUL_I2C_POT_TX_CUSTOM_INSTANCE
#include <RculI2cPotTx.h>
#include <RcTxSerial.h>

// ============================================================================
// Nano / PTR-6A
// ============================================================================
#define SDA_PIN             A4
#define SCL_PIN             A5
#define GDO0_PIN            8

#define DIGIPOT_ADDRESS     RCUL_I2C_POT_TX_DS3502_ADDRESS
#define I2C_FREQUENCY       RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY

// ============================================================================
// RcTxSerial
// ============================================================================
#define RCUL_REPEAT         4
#define RCUL_FIFO_SIZE      8
#define RCUL_CHANNEL        8

// Mettre a 1 uniquement pour un diagnostic court.
// Pour les mesures de qualite reception, laisser a 0.
#define LOCAL_DEBUG         0

static RculI2cPotTxClass RculI2cPotTx(
    RCUL_I2C_POT_DEVICE,
    RCUL_I2C_POT_SYNCRO_BY_CALLBACK_PTR_6A,
    GDO0_PIN
);

static RcTxSerial MyRcTxSerial(
    &RculI2cPotTx,
    RCUL_REPEAT,
    RCUL_FIFO_SIZE,
    RCUL_CHANNEL
);

static uint8_t Counter = 0;
static uint32_t MessageCount = 0;

// ============================================================================
// Setup
// ============================================================================
void setup()
{
  Serial.begin(115200);

  if(!RculI2cPotTx.begin(
      SDA_PIN,
      SCL_PIN,
      I2C_FREQUENCY,
      DIGIPOT_ADDRESS,
      RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS, // ignore en CALLBACK PTR-6A
      1500))
  {
    Serial.println(F("RculI2cPotTx begin ERROR"));
    while(1);
  }

  // Recharge la table issue de l'auto-calibration si elle existe.
  RculI2cPotTx.setTableStorageEEPROM(0);

  if(RculI2cPotTx.useStoredRculTable())
  {
    Serial.println(F("RCUL table: STORED EEPROM"));
  }
  else
  {
    Serial.println(F("RCUL table: EMBEDDED DEFAULT"));
  }

  RculI2cPotTx.printInfo(Serial);

  Serial.println(F("PTR-6A RcTxSerial test started"));
  Serial.println(F("Payload: 1-byte counter 00..FF"));
  Serial.print(F("RCUL_REPEAT="));
  Serial.println(RCUL_REPEAT);
  Serial.println(F("No loop debug during reception-quality test"));
}

// ============================================================================
// Loop
// ============================================================================
void loop()
{
  /*
    A appeler le plus souvent possible :
    le profil PTR-6A mesure GDO0 et produit automatiquement le top RCUL
    lorsqu'il reconnait le paquet court.
  */
  RculI2cPotTx.process();

  /*
    Lorsque RcTxSerial est libre, on charge le message suivant.

    1 octet = 2 nibbles utiles.
    Le 3e argument a 1 demande a RcTxSerial d'ajouter son checksum.
  */
  if(MyRcTxSerial.isReadyForTx())
  {
    uint8_t Message[1];
    Message[0] = Counter++;

    MyRcTxSerial.sendNibbleMsg(Message, 2, 1);
    MessageCount++;
  }

  /*
    RcTxSerial consomme le top de synchro fourni par RculI2cPotTx et change
    le niveau RCUL du DS3502 au moment voulu.
  */
  RcTxSerial::process();

#if LOCAL_DEBUG
  static uint32_t LastPrintMs = 0;

  if((uint32_t)(millis() - LastPrintMs) >= 1000UL)
  {
    LastPrintMs = millis();

    Serial.print(F("Msg="));
    Serial.print(MessageCount);
    Serial.print(F(" next=0x"));
    if(Counter < 0x10) Serial.print('0');
    Serial.print(Counter, HEX);
    Serial.print(F(" GDO="));
    Serial.print(RculI2cPotTx.getCallbackLastPulseWidthUs());
    Serial.print(F(" us cycle="));
    Serial.print(RculI2cPotTx.getCallbackCycleUs());
    Serial.print(F(" us wiper="));
    Serial.println(RculI2cPotTx.getWiper());
  }
#endif
}
