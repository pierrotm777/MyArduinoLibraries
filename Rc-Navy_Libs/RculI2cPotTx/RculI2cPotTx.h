#ifndef RCUL_I2C_POT_TX_H
#define RCUL_I2C_POT_TX_H

#include <Arduino.h>
#include <Rcul.h>

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
#define RCUL_I2C_POT_TX_REVISION                8
#define RCUL_I2C_POT_TX_PATCH                   2
#define RCUL_I2C_POT_TX_VERSION_STRING          "1.8.2"
#define RCUL_I2C_POT_TX_VERSION_NUM             0x010802

#define RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS       18
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
    explicit RculI2cPotTxClass(RculI2cPotType_t PotType = MCP4561);

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
    bool setWiperRange(uint16_t MinWiper, uint16_t MaxWiper);
    bool setCalibration(uint16_t MinWidthUs, uint16_t MaxWidthUs,
                        uint16_t MinWiper, uint16_t MaxWiper);
    bool width_us(uint16_t Width_us);
    bool wiper(uint16_t Wiper);

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
