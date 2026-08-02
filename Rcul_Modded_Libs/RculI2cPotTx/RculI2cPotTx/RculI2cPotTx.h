#ifndef RCUL_I2C_POT_TX_H
#define RCUL_I2C_POT_TX_H

#include <Arduino.h>
#include <Rcul.h>

#if defined(__AVR__) && \
    !defined(__AVR_ATtiny85__) && !defined(__AVR_ATtiny45__)
  #include <EEPROM.h>
  #define RCUL_I2C_POT_TX_TABLE_STORAGE_AVR 1
#else
  #define RCUL_I2C_POT_TX_TABLE_STORAGE_AVR 0
#endif

#if defined(ARDUINO_ARCH_ESP32)
  #include <Preferences.h>
  #define RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32 1
#else
  #define RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32 0
#endif

#define RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED \
        (RCUL_I2C_POT_TX_TABLE_STORAGE_AVR || \
         RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32)

/*
 * Selection automatique du pilote I2C.
 * TinyWireM est utilise pour les ATtiny45/85; Wire est utilise ailleurs.
 */
#if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)
  #include <TinyWireM.h>
  #define RCUL_I2C_POT_TX_USE_TINYWIREM 1
#else
  #include <Wire.h>
  #define RCUL_I2C_POT_TX_USE_TINYWIREM 0
#endif

/*
 * Identification de version et parametres generaux de la bibliotheque.
 * La periode de 18 ms est le reglage valide pendant les essais radio.
 */
#define RCUL_I2C_POT_TX_VERSION                 1
#define RCUL_I2C_POT_TX_REVISION                10
#define RCUL_I2C_POT_TX_PATCH                   0
#define RCUL_I2C_POT_TX_VERSION_STRING          "1.10.0"
#define RCUL_I2C_POT_TX_VERSION_NUM             0x010A00

#define RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS       18

/*
 * Source du top de synchronisation utilise par RcTxSerial.
 *
 * INTERNAL:
 *   comportement historique; process() produit un top periodique.
 *
 * BY_PPMIN:
 *   le top provient d'un lecteur PPM compatible Rcul.
 *
 * BY_CALLBACK:
 *   le programme appelle syncPulse() lorsqu'une nouvelle trame est detectee.
 */
/*
 * Marqueurs types de synchronisation.
 *
 * Les macros construisent des objets temporaires vides: aucune RAM n'est
 * reservee dans le sketch. Le typage force le compilateur a verifier que
 * le mode PPM IN recoit bien une source Rcul.
 */
struct RculI2cPotSyncInternal_t {};
struct RculI2cPotSyncPpmIn_t {};
struct RculI2cPotSyncCallback_t {};

#define RCUL_I2C_POT_SYNCRO_INTERNAL             RculI2cPotSyncInternal_t()
#define RCUL_I2C_POT_SYNCRO_BY_PPMIN             RculI2cPotSyncPpmIn_t()
#define RCUL_I2C_POT_SYNCRO_BY_CALLBACK          RculI2cPotSyncCallback_t()

/*
 * Valeurs internes compactes stockees sur un octet.
 */
#define RCUL_I2C_POT_SYNCRO_MODE_INTERNAL        0
#define RCUL_I2C_POT_SYNCRO_MODE_PPMIN           1
#define RCUL_I2C_POT_SYNCRO_MODE_CALLBACK        2
#define RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY   100000UL
#define RCUL_I2C_POT_TX_FAST_I2C_FREQUENCY      400000UL
#define RCUL_I2C_POT_TX_DEFAULT_MIN_WIDTH_US    880
#define RCUL_I2C_POT_TX_DEFAULT_MAX_WIDTH_US    2160

#define RCUL_I2C_POT_TX_AUTO_ADDRESS            0xFF
#define RCUL_I2C_POT_TX_MCP4561_ADDRESS         0x2C
#define RCUL_I2C_POT_TX_DS3502_ADDRESS          0x28
#define RCUL_I2C_POT_TX_MCP4561_MAX_WIPER       256
#define RCUL_I2C_POT_TX_DS3502_MAX_WIPER        127
#define RCUL_I2C_POT_TX_MCP4561_WIPER_MIN       4
#define RCUL_I2C_POT_TX_MCP4561_WIPER_MAX       252
#define RCUL_I2C_POT_TX_DS3502_WIPER_MIN        0
#define RCUL_I2C_POT_TX_DS3502_WIPER_MAX        127
#define RCUL_I2C_POT_TX_DS3502_MIN_WIDTH_US      1024
#define RCUL_I2C_POT_TX_DS3502_MAX_WIDTH_US      1976

#define RCUL_I2C_POT_TX_RCUL_MIN_WIDTH_US         1024
#define RCUL_I2C_POT_TX_RCUL_STEP_WIDTH_US        56
#define RCUL_I2C_POT_TX_RCUL_SYMBOL_NB            18

#define RCUL_I2C_POT_TX_TABLE_MAGIC                 0x5243
#define RCUL_I2C_POT_TX_TABLE_FORMAT                1
#define RCUL_I2C_POT_TX_DEFAULT_SETTLE_PULSES       2
#define RCUL_I2C_POT_TX_DEFAULT_AVERAGE_PULSES      4
#define RCUL_I2C_POT_TX_CAL_MIN_VALID_US            700
#define RCUL_I2C_POT_TX_CAL_MAX_VALID_US            2300
#define RCUL_I2C_POT_TX_CAL_MAX_ERROR_US             80

typedef enum
{
  RCUL_I2C_POT_TABLE_DEFAULT = 0,
  RCUL_I2C_POT_TABLE_STORED  = 1
} RculI2cPotTableMode_t;

#ifndef SDA
#define SDA 0
#endif
#ifndef SCL
#define SCL 0
#endif

/*
 * Type de potentiometre numerique pilote par l'instance.
 * Le constructeur utilise MCP4561 par defaut pour rester compatible avec
 * les anciens sketches.
 */
typedef enum
{
  MCP4561 = 0,
  DS3502  = 1
} RculI2cPotType_t;

/*
 * Adaptateur entre RcTxSerial/Rcul et un potentiometre numerique I2C.
 *
 * La classe herite de Rcul afin que RcTxSerial puisse:
 *   - attendre un top de synchronisation periodique;
 *   - demander la largeur du niveau courant;
 *   - imposer la largeur du symbole RCUL suivant.
 */
class RculI2cPotTxClass : public Rcul
{
  private:
#if !RCUL_I2C_POT_TX_USE_TINYWIREM
    TwoWire *_Wire;
#endif
    RculI2cPotType_t _PotType;
    uint8_t _Address;
    uint8_t _PeriodMs;
    uint8_t _StartMs;
    uint8_t _Synchro;
    uint8_t _SdaPin;
    uint8_t _SclPin;
    uint32_t _I2cFrequency;

    uint8_t _SyncroMode;
    Rcul *_PpmInSyncSource;

    uint16_t _MinWidthUs;
    uint16_t _MaxWidthUs;
    uint16_t _MinWiper;
    uint16_t _MaxWiper;
    uint16_t _DeviceMaxWiper;
    uint16_t _CurrentWiper;
    uint16_t _CurrentWidthUs;

    bool _Begun;
    bool _LastWriteOk;
    bool _CustomWiperRange;

#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
    uint8_t _RculTableMode;
    uint16_t _StoredRculWiperTable[RCUL_I2C_POT_TX_RCUL_SYMBOL_NB];

  #if RCUL_I2C_POT_TX_TABLE_STORAGE_AVR
    uint16_t _StorageBaseAddress;
  #endif

  #if RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32
    char _PreferencesNamespace[16];
  #endif

    Rcul *_CalibrationPwmSource;
    Stream *_CalibrationStream;
    uint16_t _CalibrationBestError[RCUL_I2C_POT_TX_RCUL_SYMBOL_NB];
    uint16_t _CalibrationSavedWidthUs;
    uint32_t _CalibrationSumUs;
    uint16_t _CalibrationWiper;
    uint8_t _CalibrationClientIdx;
    uint8_t _CalibrationSettleLeft;
    uint8_t _CalibrationSettlePulseNb;
    uint8_t _CalibrationAveragePulseNb;
    uint8_t _CalibrationSampleNb;
    bool _CalibrationActive;
    bool _CalibrationFailed;
#endif

    uint8_t defaultAddress(void) const;
    uint16_t defaultMinWiper(void) const;
    uint16_t defaultMaxWiper(void) const;
    void loadDeviceDefaults(void);

    bool beginCommon(uint8_t Address, uint8_t PeriodMs, uint16_t InitialWidthUs);
    bool configureDs3502Volatile(void);
    bool writeMcp4561(uint16_t Wiper);
    bool writeDs3502(uint16_t Wiper);
    bool writeDeviceWiper(uint16_t Wiper);
    uint16_t widthToWiper(uint16_t Width_us) const;

#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
    uint16_t defaultRculWiper(uint8_t SymbolIdx) const;
    uint16_t crc16Update(uint16_t Crc, uint8_t Data) const;
    uint16_t tableCrc(const uint16_t *Table) const;
    bool validateRculTable(const uint16_t *Table,
                           const uint16_t *Errors = NULL) const;
    bool loadStoredRculTable(void);
    bool saveStoredRculTable(void);
    void finishRculTableCalibration(bool Success);
#endif

    void i2cBeginTransmission(uint8_t Address);
    void i2cWrite(uint8_t Value);
    uint8_t i2cEndTransmission(void);

  public:
    /*
     * API publique.
     *
     * begin() initialise le bus et le composant.
     * process() doit etre appelee continuellement.
     * width_us() effectue la conversion largeur -> curseur.
     * wiper() permet un acces direct au curseur.
     */
    /*
     * Constructeur historique: synchronisation interne par defaut.
     */
    explicit RculI2cPotTxClass(RculI2cPotType_t PotType = MCP4561);

    /*
     * Selection explicite du mode interne.
     */
    RculI2cPotTxClass(RculI2cPotType_t PotType,
                      RculI2cPotSyncInternal_t);

    /*
     * Mode PPM IN: la reference Rcul est obligatoire.
     * Son oubli provoque une erreur de compilation.
     */
    RculI2cPotTxClass(RculI2cPotType_t PotType,
                      RculI2cPotSyncPpmIn_t,
                      Rcul &PpmInSyncSource);

    /*
     * Mode callback: syncPulse() injecte chaque top.
     */
    RculI2cPotTxClass(RculI2cPotType_t PotType,
                      RculI2cPotSyncCallback_t);

    bool begin(uint8_t SdaPin = SDA,
               uint8_t SclPin = SCL,
               uint32_t I2cFrequency = RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY,
               uint8_t Address = RCUL_I2C_POT_TX_AUTO_ADDRESS,
               uint8_t PeriodMs = RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
               uint16_t InitialWidthUs = 1500
#if !RCUL_I2C_POT_TX_USE_TINYWIREM
               , TwoWire *WirePort = &Wire
#endif
               );

#if !RCUL_I2C_POT_TX_USE_TINYWIREM
    bool begin(TwoWire *WirePort,
               uint8_t Address = RCUL_I2C_POT_TX_AUTO_ADDRESS,
               uint8_t PeriodMs = RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS,
               uint16_t InitialWidthUs = 1500);
#endif

    void process(void);
    uint8_t isSynchro(uint8_t SynchroClientIdx = 7);

    /*
     * Injecte un top uniquement en mode RCUL_I2C_POT_SYNCRO_BY_CALLBACK.
     */
    void syncPulse(void);

    uint8_t getSyncroMode(void) const;
    bool setWiperRange(uint16_t MinWiper, uint16_t MaxWiper);
    bool setCalibration(uint16_t MinWidthUs, uint16_t MaxWidthUs,
                        uint16_t MinWiper, uint16_t MaxWiper);
    bool width_us(uint16_t Width_us);
    bool wiper(uint16_t Wiper);

#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
    bool setRculTableMode(RculI2cPotTableMode_t Mode);
    RculI2cPotTableMode_t getRculTableMode(void) const;
    void useDefaultRculTable(void);
    bool useStoredRculTable(void);
    bool eraseStoredRculTable(void);

  #if RCUL_I2C_POT_TX_TABLE_STORAGE_AVR
    void setTableStorageEEPROM(uint16_t BaseAddress = 0);
  #endif

  #if RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32
    void setTableStoragePreferences(const char *Namespace = "RculPotTx");
  #endif

    bool startRculTableCalibration(
        Rcul &PwmSource,
        Stream *Out = NULL,
        uint8_t ClientIdx = 6,
        uint8_t SettlePulseNb = RCUL_I2C_POT_TX_DEFAULT_SETTLE_PULSES,
        uint8_t AveragePulseNb = RCUL_I2C_POT_TX_DEFAULT_AVERAGE_PULSES);

    void processRculTableCalibration(void);
    bool isRculTableCalibrationActive(void) const;
    bool didRculTableCalibrationFail(void) const;
    void displayRculTable(Stream &Out) const;
#endif

    uint16_t getWidth_us(void) const;
    uint16_t getWiper(void) const;
    uint16_t getMaxWiper(void) const;
    uint8_t getI2cAddress(void) const;
    RculI2cPotType_t getPotType(void) const;
    bool isConnected(void);
    void printInfo(Stream &Out);

    virtual uint8_t RculIsSynchro(uint8_t ClientIdx = RCUL_DEFAULT_CLIENT_IDX);
    virtual uint16_t RculGetWidth_us(uint8_t Ch);
    virtual void RculSetWidth_us(uint16_t Width_us, uint8_t Ch = 255);
};

/*
 * Instance globale historique.
 * Definir RCUL_I2C_POT_TX_CUSTOM_INSTANCE avant l'inclusion du fichier
 * pour creer soi-meme une instance DS3502 ou MCP4561 du meme nom.
 */
#ifndef RCUL_I2C_POT_TX_CUSTOM_INSTANCE
extern RculI2cPotTxClass RculI2cPotTx;
#endif

#endif
