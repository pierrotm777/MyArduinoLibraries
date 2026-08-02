#include <RculI2cPotTx.h>

/*
 * Instance globale historique: MCP4561 par defaut.
 * Elle est supprimee de la declaration publique lorsque le sketch definit
 * RCUL_I2C_POT_TX_CUSTOM_INSTANCE avant d'inclure le fichier d'en-tete.
 */
RculI2cPotTxClass RculI2cPotTx(MCP4561);

/*
 * Compteur de temps compact sur 8 bits.
 * Les soustractions non signees gerent naturellement le debordement.
 * Une periode de 18 ms reste tres inferieure aux 256 ms representables.
 */
#ifndef millis8
#define millis8() (uint8_t)(millis() & 0xFF)
#endif
#ifndef ElapsedMs8Since
#define ElapsedMs8Since(StartMs8) (uint8_t)(millis8() - (uint8_t)(StartMs8))
#endif

/*
 * DS3502 wiper positions measured through the complete analog/RF chain:
 * DS3502 -> transmitter ADC -> FrSky RF -> receiver PWM.
 *
 * Entries correspond to RCUL symbols:
 *   0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F,R,I
 * whose requested pulse centers are 1024 us to 1976 us in 56 us steps.
 *
 * The linear 0..127 sweep produced approximately 986..2012 us at the
 * receiver. These corrected positions recenter the 18 symbols.
 */
static const uint8_t Ds3502RculWiperTable[RCUL_I2C_POT_TX_RCUL_SYMBOL_NB] =
{
   5, 12, 19, 26, 33, 40, 47, 53, 60,
  67, 74, 81, 87, 94,101,108,115,122
};

/*
 * Constructeur:
 * - memorise le type de composant;
 * - initialise l'etat interne;
 * - charge les limites propres au composant;
 * - applique au DS3502 la plage RCUL 1024..1976 us.
 */
RculI2cPotTxClass::RculI2cPotTxClass(RculI2cPotType_t PotType) :
#if !RCUL_I2C_POT_TX_USE_TINYWIREM
  _Wire(NULL),
#endif
  _PotType(PotType), _Address(0),
  _PeriodMs(RCUL_I2C_POT_TX_DEFAULT_PERIOD_MS), _StartMs(0), _Synchro(0),
  _SdaPin(SDA), _SclPin(SCL),
  _I2cFrequency(RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY),
  _SyncroMode(RCUL_I2C_POT_SYNCRO_MODE_INTERNAL),
  _PpmInSyncSource(NULL),
  _MinWidthUs(RCUL_I2C_POT_TX_DEFAULT_MIN_WIDTH_US),
  _MaxWidthUs(RCUL_I2C_POT_TX_DEFAULT_MAX_WIDTH_US),
  _MinWiper(0), _MaxWiper(0), _DeviceMaxWiper(0),
  _CurrentWiper(0xFFFF), _CurrentWidthUs(1500),
  _Begun(false), _LastWriteOk(false), _CustomWiperRange(false)
#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
  , _RculTableMode(RCUL_I2C_POT_TABLE_DEFAULT)
  #if RCUL_I2C_POT_TX_TABLE_STORAGE_AVR
    , _StorageBaseAddress(0)
  #endif
    , _CalibrationPwmSource(NULL), _CalibrationStream(NULL),
      _CalibrationSavedWidthUs(1500), _CalibrationSumUs(0),
      _CalibrationWiper(0), _CalibrationClientIdx(6),
      _CalibrationSettleLeft(0),
      _CalibrationSettlePulseNb(RCUL_I2C_POT_TX_DEFAULT_SETTLE_PULSES),
      _CalibrationAveragePulseNb(RCUL_I2C_POT_TX_DEFAULT_AVERAGE_PULSES),
      _CalibrationSampleNb(0),
      _CalibrationActive(false), _CalibrationFailed(false)
#endif
{
  loadDeviceDefaults();
  if(_PotType == DS3502)
  {
    _MinWidthUs = RCUL_I2C_POT_TX_DS3502_MIN_WIDTH_US;
    _MaxWidthUs = RCUL_I2C_POT_TX_DS3502_MAX_WIDTH_US;
  }
  _Address = defaultAddress();

#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    _StoredRculWiperTable[Idx] = defaultRculWiper(Idx);
    _CalibrationBestError[Idx] = 0xFFFF;
  }

  #if RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32
  strncpy(_PreferencesNamespace,
          "RculPotTx",
          sizeof(_PreferencesNamespace));
  _PreferencesNamespace[
      sizeof(_PreferencesNamespace) - 1] = 0;
  #endif
#endif
}

/*
 * Constructeurs types de synchronisation.
 *
 * Les marqueurs sont des objets vides temporaires et ne consomment pas
 * de RAM persistante.
 */
RculI2cPotTxClass::RculI2cPotTxClass(
    RculI2cPotType_t PotType,
    RculI2cPotSyncInternal_t) :
  RculI2cPotTxClass(PotType)
{
}

RculI2cPotTxClass::RculI2cPotTxClass(
    RculI2cPotType_t PotType,
    RculI2cPotSyncPpmIn_t,
    Rcul &PpmInSyncSource) :
  RculI2cPotTxClass(PotType)
{
  _SyncroMode = RCUL_I2C_POT_SYNCRO_MODE_PPMIN;
  _PpmInSyncSource = &PpmInSyncSource;
}

RculI2cPotTxClass::RculI2cPotTxClass(
    RculI2cPotType_t PotType,
    RculI2cPotSyncCallback_t) :
  RculI2cPotTxClass(PotType)
{
  _SyncroMode = RCUL_I2C_POT_SYNCRO_MODE_CALLBACK;
}

/*
 * Parametres par defaut dependants du composant.
 */
uint8_t RculI2cPotTxClass::defaultAddress(void) const
{
  return (_PotType == DS3502) ? RCUL_I2C_POT_TX_DS3502_ADDRESS
                              : RCUL_I2C_POT_TX_MCP4561_ADDRESS;
}

uint16_t RculI2cPotTxClass::defaultMinWiper(void) const
{
  return (_PotType == DS3502) ? RCUL_I2C_POT_TX_DS3502_WIPER_MIN
                              : RCUL_I2C_POT_TX_MCP4561_WIPER_MIN;
}

uint16_t RculI2cPotTxClass::defaultMaxWiper(void) const
{
  return (_PotType == DS3502) ? RCUL_I2C_POT_TX_DS3502_WIPER_MAX
                              : RCUL_I2C_POT_TX_MCP4561_WIPER_MAX;
}

void RculI2cPotTxClass::loadDeviceDefaults(void)
{
  _DeviceMaxWiper = (_PotType == DS3502) ? RCUL_I2C_POT_TX_DS3502_MAX_WIPER
                                         : RCUL_I2C_POT_TX_MCP4561_MAX_WIPER;
  _MinWiper = defaultMinWiper();
  _MaxWiper = defaultMaxWiper();
  _CustomWiperRange = false;
}

/*
 * Initialisation portable:
 * - TinyWireM sur ATtiny45/85;
 * - Wire avec broches configurables sur ESP32;
 * - Wire materiel standard sur les autres plateformes.
 */
bool RculI2cPotTxClass::begin(uint8_t SdaPin, uint8_t SclPin,
                              uint32_t I2cFrequency, uint8_t Address,
                              uint8_t PeriodMs, uint16_t InitialWidthUs
#if !RCUL_I2C_POT_TX_USE_TINYWIREM
                              , TwoWire *WirePort
#endif
                              )
{
  if(I2cFrequency == 0) I2cFrequency = RCUL_I2C_POT_TX_DEFAULT_I2C_FREQUENCY;
  _SdaPin = SdaPin; _SclPin = SclPin; _I2cFrequency = I2cFrequency;
#if RCUL_I2C_POT_TX_USE_TINYWIREM
  (void)SdaPin; (void)SclPin;
  TinyWireM.begin();
#else
  if(WirePort == NULL) return false;
  _Wire = WirePort;
  #if defined(ESP32)
    if(!WirePort->begin((int)SdaPin, (int)SclPin, I2cFrequency)) return false;
  #else
    (void)SdaPin; (void)SclPin;
    WirePort->begin();
    WirePort->setClock(I2cFrequency);
  #endif
#endif
  return beginCommon(Address, PeriodMs, InitialWidthUs);
}

#if !RCUL_I2C_POT_TX_USE_TINYWIREM
bool RculI2cPotTxClass::begin(TwoWire *WirePort, uint8_t Address,
                              uint8_t PeriodMs, uint16_t InitialWidthUs)
{
  if(WirePort == NULL) return false;
  _Wire = WirePort;
  return beginCommon(Address, PeriodMs, InitialWidthUs);
}
#endif

/*
 * Partie commune de begin():
 * choix de l'adresse, verification de presence, configuration du DS3502
 * en mode volatile, puis application de la largeur initiale.
 */
bool RculI2cPotTxClass::beginCommon(uint8_t Address, uint8_t PeriodMs,
                                    uint16_t InitialWidthUs)
{
  _Address = (Address == RCUL_I2C_POT_TX_AUTO_ADDRESS) ? defaultAddress() : Address;
  _PeriodMs = (PeriodMs == 0) ? 1 : PeriodMs;
  _StartMs = millis8(); _Synchro = 0; _CurrentWiper = 0xFFFF;
  _LastWriteOk = false;
  _Begun = true;

#if !RCUL_I2C_POT_TX_USE_TINYWIREM
  /*
   * Wire supports a conventional address-only presence probe.
   */
  if(!isConnected())
  {
    _Begun = false;
    return false;
  }
#endif

  /*
   * On ATtiny45/85, do not use an empty TinyWireM transaction as a
   * presence test. The real DS3502 configuration and initial wiper write
   * below validate the device with the exact transactions used at runtime.
   */
  if((_PotType == DS3502) && !configureDs3502Volatile())
  {
    _Begun = false;
    return false;
  }

  if(!width_us(InitialWidthUs))
  {
    _Begun = false;
    return false;
  }

  return true;
}

/*
 * Petite couche d'abstraction I2C permettant de partager tout le reste
 * du pilote entre Wire et TinyWireM.
 */
void RculI2cPotTxClass::i2cBeginTransmission(uint8_t Address)
{
#if RCUL_I2C_POT_TX_USE_TINYWIREM
  TinyWireM.beginTransmission(Address);
#else
  _Wire->beginTransmission(Address);
#endif
}
void RculI2cPotTxClass::i2cWrite(uint8_t Value)
{
#if RCUL_I2C_POT_TX_USE_TINYWIREM
  TinyWireM.send(Value);
#else
  _Wire->write(Value);
#endif
}
uint8_t RculI2cPotTxClass::i2cEndTransmission(void)
{
#if RCUL_I2C_POT_TX_USE_TINYWIREM
  return TinyWireM.endTransmission();
#else
  return _Wire->endTransmission();
#endif
}

/*
 * Le bit MODE du DS3502 est positionne afin que les changements rapides
 * du registre WR ne soient pas recopies dans l'EEPROM.
 */
bool RculI2cPotTxClass::configureDs3502Volatile(void)
{
  i2cBeginTransmission(_Address);
  i2cWrite(0x02);       // Control Register
  i2cWrite(0x80);       // MODE=1: register 0x00 writes WR only, not EEPROM
  return (i2cEndTransmission() == 0);
}

/*
 * Pilotes d'ecriture propres a chaque potentiometre.
 */
bool RculI2cPotTxClass::writeMcp4561(uint16_t Wiper)
{
  Wiper = constrain(Wiper, (uint16_t)0, (uint16_t)RCUL_I2C_POT_TX_MCP4561_MAX_WIPER);
  i2cBeginTransmission(_Address);
  i2cWrite((uint8_t)((Wiper >> 8) & 0x01));
  i2cWrite((uint8_t)(Wiper & 0xFF));
  return (i2cEndTransmission() == 0);
}

bool RculI2cPotTxClass::writeDs3502(uint16_t Wiper)
{
  Wiper = constrain(Wiper, (uint16_t)0, (uint16_t)RCUL_I2C_POT_TX_DS3502_MAX_WIPER);
  i2cBeginTransmission(_Address);
  i2cWrite(0x00);       // Wiper Register
  i2cWrite((uint8_t)Wiper);
  return (i2cEndTransmission() == 0);
}

bool RculI2cPotTxClass::writeDeviceWiper(uint16_t Wiper)
{
  return (_PotType == DS3502) ? writeDs3502(Wiper) : writeMcp4561(Wiper);
}

/*
 * Gestion non bloquante du top de synchronisation.
 *
 * INTERNAL:
 *   produit le top periodique historique.
 *
 * BY_PPMIN:
 *   aucun top local; isSynchro() interroge directement la source PPM.
 *
 * BY_CALLBACK:
 *   aucun top local; syncPulse() rearme les clients.
 */
void RculI2cPotTxClass::process(void)
{
  if(!_Begun)
  {
    return;
  }

  if(_SyncroMode != RCUL_I2C_POT_SYNCRO_MODE_INTERNAL)
  {
    return;
  }

  if(ElapsedMs8Since(_StartMs) >= _PeriodMs)
  {
    _Synchro = 0xFF;
    _StartMs = millis8();
  }
}

/*
 * Retourne un seul top par client.
 */
uint8_t RculI2cPotTxClass::isSynchro(uint8_t SynchroClientIdx)
{
  if(_SyncroMode == RCUL_I2C_POT_SYNCRO_MODE_PPMIN)
  {
    if(_PpmInSyncSource == NULL)
    {
      return 0;
    }

    return _PpmInSyncSource->RculIsSynchro(SynchroClientIdx);
  }

  const uint8_t ClientMask = RCUL_CLIENT_MASK(SynchroClientIdx);
  const uint8_t Result = (_Synchro & ClientMask) ? 1 : 0;

  if(Result)
  {
    _Synchro &= (uint8_t)~ClientMask;
  }

  return Result;
}

void RculI2cPotTxClass::syncPulse(void)
{
  if(_Begun && (_SyncroMode == RCUL_I2C_POT_SYNCRO_MODE_CALLBACK))
  {
    _Synchro = 0xFF;
  }
}

uint8_t RculI2cPotTxClass::getSyncroMode(void) const
{
  return _SyncroMode;
}

/*
 * Reglages optionnels de la conversion lineaire.
 * La table speciale RCUL du DS3502 reste prioritaire pour les 18 centres
 * exacts demandes par RcTxSerial.
 */
bool RculI2cPotTxClass::setWiperRange(uint16_t MinWiper, uint16_t MaxWiper)
{
  MinWiper = constrain(MinWiper, (uint16_t)0, _DeviceMaxWiper);
  MaxWiper = constrain(MaxWiper, (uint16_t)0, _DeviceMaxWiper);
  if(MinWiper >= MaxWiper) return false;
  _MinWiper = MinWiper; _MaxWiper = MaxWiper;
  _CustomWiperRange = (MinWiper != defaultMinWiper()) || (MaxWiper != defaultMaxWiper());
  return _Begun ? width_us(_CurrentWidthUs) : true;
}


bool RculI2cPotTxClass::setCalibration(uint16_t MinWidthUs, uint16_t MaxWidthUs,
                                             uint16_t MinWiper, uint16_t MaxWiper)
{
  if(MinWidthUs >= MaxWidthUs) return false;

  MinWiper = constrain(MinWiper, (uint16_t)0, _DeviceMaxWiper);
  MaxWiper = constrain(MaxWiper, (uint16_t)0, _DeviceMaxWiper);
  if(MinWiper >= MaxWiper) return false;

  _MinWidthUs = MinWidthUs;
  _MaxWidthUs = MaxWidthUs;
  _MinWiper = MinWiper;
  _MaxWiper = MaxWiper;
  _CustomWiperRange = true;

  return _Begun ? width_us(_CurrentWidthUs) : true;
}

uint16_t RculI2cPotTxClass::widthToWiper(uint16_t Width_us) const
{
  /*
   * RcTxSerial requests the 18 exact RCUL centers from 1024 to 1976 us.
   * For the DS3502, use the measured correction table only for those exact
   * centers. Other values (initial neutral, manual tests, calibration, etc.)
   * continue to use the normal linear conversion.
   */
  if(_PotType == DS3502 &&
     Width_us >= RCUL_I2C_POT_TX_RCUL_MIN_WIDTH_US &&
     Width_us <= RCUL_I2C_POT_TX_DS3502_MAX_WIDTH_US)
  {
    const uint16_t Offset = Width_us - RCUL_I2C_POT_TX_RCUL_MIN_WIDTH_US;

    if((Offset % RCUL_I2C_POT_TX_RCUL_STEP_WIDTH_US) == 0)
    {
      const uint8_t SymbolIdx =
          (uint8_t)(Offset / RCUL_I2C_POT_TX_RCUL_STEP_WIDTH_US);

      if(SymbolIdx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB)
      {
#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
        if(_RculTableMode == RCUL_I2C_POT_TABLE_STORED)
        {
          return _StoredRculWiperTable[SymbolIdx];
        }
#endif
        return Ds3502RculWiperTable[SymbolIdx];
      }
    }
  }

  Width_us = constrain(Width_us, _MinWidthUs, _MaxWidthUs);
  return (uint16_t)constrain(map((long)Width_us, (long)_MinWidthUs,
      (long)_MaxWidthUs, (long)_MinWiper, (long)_MaxWiper), 0L, (long)_DeviceMaxWiper);
}

/*
 * Ecriture publique du curseur:
 * - limite la valeur;
 * - evite une transaction I2C inutile si la position ne change pas;
 * - memorise la nouvelle position uniquement apres acquittement.
 */
bool RculI2cPotTxClass::wiper(uint16_t Wiper)
{
  Wiper = constrain(Wiper, (uint16_t)0, _DeviceMaxWiper);
  if(_Begun && (_CurrentWiper == Wiper)) { _LastWriteOk = true; return true; }
#if !RCUL_I2C_POT_TX_USE_TINYWIREM
  if(!_Begun || (_Wire == NULL))
#else
  if(!_Begun)
#endif
  { _LastWriteOk = false; return false; }
  _LastWriteOk = writeDeviceWiper(Wiper);
  if(_LastWriteOk) _CurrentWiper = Wiper;
  return _LastWriteOk;
}

/*
 * Entree principale utilisee par RcTxSerial.
 * La largeur est bornee, memorisee, convertie puis envoyee au composant.
 */
bool RculI2cPotTxClass::width_us(uint16_t Width_us)
{
  Width_us = constrain(Width_us, _MinWidthUs, _MaxWidthUs);
  _CurrentWidthUs = Width_us;
  return wiper(widthToWiper(Width_us));
}


#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED

uint16_t RculI2cPotTxClass::defaultRculWiper(
    uint8_t SymbolIdx) const
{
  if(SymbolIdx >= RCUL_I2C_POT_TX_RCUL_SYMBOL_NB)
  {
    return 0;
  }

  if(_PotType == DS3502)
  {
    return Ds3502RculWiperTable[SymbolIdx];
  }

  const uint16_t WidthUs =
      RCUL_I2C_POT_TX_RCUL_MIN_WIDTH_US +
      (uint16_t)SymbolIdx *
      RCUL_I2C_POT_TX_RCUL_STEP_WIDTH_US;

  return (uint16_t)constrain(
      map((long)WidthUs,
          (long)_MinWidthUs,
          (long)_MaxWidthUs,
          (long)_MinWiper,
          (long)_MaxWiper),
      0L,
      (long)_DeviceMaxWiper);
}

uint16_t RculI2cPotTxClass::crc16Update(
    uint16_t Crc,
    uint8_t Data) const
{
  Crc ^= (uint16_t)Data << 8;

  for(uint8_t Bit = 0; Bit < 8; Bit++)
  {
    Crc = (Crc & 0x8000)
        ? (uint16_t)((Crc << 1) ^ 0x1021)
        : (uint16_t)(Crc << 1);
  }

  return Crc;
}

uint16_t RculI2cPotTxClass::tableCrc(
    const uint16_t *Table) const
{
  uint16_t Crc = 0xFFFF;

  Crc = crc16Update(
      Crc,
      RCUL_I2C_POT_TX_TABLE_FORMAT);

  Crc = crc16Update(
      Crc,
      (uint8_t)_PotType);

  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    Crc = crc16Update(Crc, lowByte(Table[Idx]));
    Crc = crc16Update(Crc, highByte(Table[Idx]));
  }

  return Crc;
}

bool RculI2cPotTxClass::validateRculTable(
    const uint16_t *Table,
    const uint16_t *Errors) const
{
  bool Increasing = true;
  bool Decreasing = true;

  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    if(Table[Idx] > _DeviceMaxWiper)
    {
      return false;
    }

    if(Errors &&
       (Errors[Idx] >
        RCUL_I2C_POT_TX_CAL_MAX_ERROR_US))
    {
      return false;
    }

    if(Idx)
    {
      if(Table[Idx] <= Table[Idx - 1])
      {
        Increasing = false;
      }

      if(Table[Idx] >= Table[Idx - 1])
      {
        Decreasing = false;
      }
    }
  }

  return Increasing || Decreasing;
}

#if RCUL_I2C_POT_TX_TABLE_STORAGE_AVR

void RculI2cPotTxClass::setTableStorageEEPROM(
    uint16_t BaseAddress)
{
  _StorageBaseAddress = BaseAddress;
}

bool RculI2cPotTxClass::loadStoredRculTable(void)
{
  uint16_t Address = _StorageBaseAddress;

  const uint16_t Magic =
      EEPROM.read(Address) |
      ((uint16_t)EEPROM.read(Address + 1) << 8);

  Address += 2;

  const uint8_t Format = EEPROM.read(Address++);
  const uint8_t PotType = EEPROM.read(Address++);

  if((Magic != RCUL_I2C_POT_TX_TABLE_MAGIC) ||
     (Format != RCUL_I2C_POT_TX_TABLE_FORMAT) ||
     (PotType != (uint8_t)_PotType))
  {
    return false;
  }

  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    _StoredRculWiperTable[Idx] =
        EEPROM.read(Address) |
        ((uint16_t)EEPROM.read(Address + 1) << 8);

    Address += 2;
  }

  const uint16_t StoredCrc =
      EEPROM.read(Address) |
      ((uint16_t)EEPROM.read(Address + 1) << 8);

  return
      (StoredCrc ==
       tableCrc(_StoredRculWiperTable)) &&
      validateRculTable(_StoredRculWiperTable);
}

bool RculI2cPotTxClass::saveStoredRculTable(void)
{
  uint16_t Address = _StorageBaseAddress;

  EEPROM.update(
      Address++,
      lowByte(RCUL_I2C_POT_TX_TABLE_MAGIC));

  EEPROM.update(
      Address++,
      highByte(RCUL_I2C_POT_TX_TABLE_MAGIC));

  EEPROM.update(
      Address++,
      RCUL_I2C_POT_TX_TABLE_FORMAT);

  EEPROM.update(
      Address++,
      (uint8_t)_PotType);

  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    EEPROM.update(
        Address++,
        lowByte(_StoredRculWiperTable[Idx]));

    EEPROM.update(
        Address++,
        highByte(_StoredRculWiperTable[Idx]));
  }

  const uint16_t Crc =
      tableCrc(_StoredRculWiperTable);

  EEPROM.update(Address++, lowByte(Crc));
  EEPROM.update(Address++, highByte(Crc));

  return true;
}

bool RculI2cPotTxClass::eraseStoredRculTable(void)
{
  EEPROM.update(_StorageBaseAddress, 0xFF);
  EEPROM.update(_StorageBaseAddress + 1, 0xFF);

  useDefaultRculTable();
  return true;
}

#elif RCUL_I2C_POT_TX_TABLE_STORAGE_ESP32

void RculI2cPotTxClass::setTableStoragePreferences(
    const char *Namespace)
{
  if(Namespace && Namespace[0])
  {
    strncpy(
        _PreferencesNamespace,
        Namespace,
        sizeof(_PreferencesNamespace));

    _PreferencesNamespace[
        sizeof(_PreferencesNamespace) - 1] = 0;
  }
}

bool RculI2cPotTxClass::loadStoredRculTable(void)
{
  Preferences Prefs;

  if(!Prefs.begin(_PreferencesNamespace, true))
  {
    return false;
  }

  const uint16_t Magic =
      Prefs.getUShort("magic", 0xFFFF);

  const uint8_t Format =
      Prefs.getUChar("format", 0xFF);

  const uint8_t PotType =
      Prefs.getUChar("pot", 0xFF);

  const size_t Length =
      Prefs.getBytesLength("table");

  bool Ok =
      (Magic ==
       RCUL_I2C_POT_TX_TABLE_MAGIC) &&
      (Format ==
       RCUL_I2C_POT_TX_TABLE_FORMAT) &&
      (PotType == (uint8_t)_PotType) &&
      (Length ==
       sizeof(_StoredRculWiperTable));

  if(Ok)
  {
    Prefs.getBytes(
        "table",
        _StoredRculWiperTable,
        sizeof(_StoredRculWiperTable));

    const uint16_t StoredCrc =
        Prefs.getUShort("crc", 0);

    Ok =
        (StoredCrc ==
         tableCrc(_StoredRculWiperTable)) &&
        validateRculTable(
            _StoredRculWiperTable);
  }

  Prefs.end();
  return Ok;
}

bool RculI2cPotTxClass::saveStoredRculTable(void)
{
  Preferences Prefs;

  if(!Prefs.begin(_PreferencesNamespace, false))
  {
    return false;
  }

  Prefs.putUShort(
      "magic",
      RCUL_I2C_POT_TX_TABLE_MAGIC);

  Prefs.putUChar(
      "format",
      RCUL_I2C_POT_TX_TABLE_FORMAT);

  Prefs.putUChar(
      "pot",
      (uint8_t)_PotType);

  Prefs.putBytes(
      "table",
      _StoredRculWiperTable,
      sizeof(_StoredRculWiperTable));

  Prefs.putUShort(
      "crc",
      tableCrc(_StoredRculWiperTable));

  Prefs.end();
  return true;
}

bool RculI2cPotTxClass::eraseStoredRculTable(void)
{
  Preferences Prefs;

  if(!Prefs.begin(_PreferencesNamespace, false))
  {
    return false;
  }

  const bool Ok = Prefs.clear();
  Prefs.end();

  useDefaultRculTable();
  return Ok;
}

#endif

bool RculI2cPotTxClass::setRculTableMode(
    RculI2cPotTableMode_t Mode)
{
  if(Mode == RCUL_I2C_POT_TABLE_DEFAULT)
  {
    useDefaultRculTable();
    return true;
  }

  return useStoredRculTable();
}

RculI2cPotTableMode_t
RculI2cPotTxClass::getRculTableMode(void) const
{
  return
      (RculI2cPotTableMode_t)_RculTableMode;
}

void RculI2cPotTxClass::useDefaultRculTable(void)
{
  _RculTableMode =
      RCUL_I2C_POT_TABLE_DEFAULT;
}

bool RculI2cPotTxClass::useStoredRculTable(void)
{
  if(!loadStoredRculTable())
  {
    _RculTableMode =
        RCUL_I2C_POT_TABLE_DEFAULT;

    return false;
  }

  _RculTableMode =
      RCUL_I2C_POT_TABLE_STORED;

  return true;
}

bool RculI2cPotTxClass::startRculTableCalibration(
    Rcul &PwmSource,
    Stream *Out,
    uint8_t ClientIdx,
    uint8_t SettlePulseNb,
    uint8_t AveragePulseNb)
{
  if(!_Begun || _CalibrationActive)
  {
    return false;
  }

  if(AveragePulseNb == 0)
  {
    AveragePulseNb = 1;
  }

  _CalibrationPwmSource = &PwmSource;
  _CalibrationStream = Out;
  _CalibrationClientIdx = ClientIdx;
  _CalibrationSettlePulseNb = SettlePulseNb;
  _CalibrationAveragePulseNb = AveragePulseNb;
  _CalibrationSavedWidthUs = _CurrentWidthUs;
  _CalibrationWiper = 0;
  _CalibrationSettleLeft = SettlePulseNb;
  _CalibrationSampleNb = 0;
  _CalibrationSumUs = 0;
  _CalibrationFailed = false;
  _CalibrationActive = true;

  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    _StoredRculWiperTable[Idx] =
        defaultRculWiper(Idx);

    _CalibrationBestError[Idx] =
        0xFFFF;
  }

  if(_CalibrationStream)
  {
    _CalibrationStream->println(
        F("[RCUL_TABLE_CAL_BEGIN]"));
  }

  if(!wiper(_CalibrationWiper))
  {
    finishRculTableCalibration(false);
    return false;
  }

  return true;
}

void RculI2cPotTxClass::
processRculTableCalibration(void)
{
  if(!_CalibrationActive ||
     !_CalibrationPwmSource)
  {
    return;
  }

  if(!_CalibrationPwmSource->
      RculIsSynchro(_CalibrationClientIdx))
  {
    return;
  }

  const uint16_t WidthUs =
      _CalibrationPwmSource->
      RculGetWidth_us(RCUL_NO_CH);

  if((WidthUs <
      RCUL_I2C_POT_TX_CAL_MIN_VALID_US) ||
     (WidthUs >
      RCUL_I2C_POT_TX_CAL_MAX_VALID_US))
  {
    return;
  }

  if(_CalibrationSettleLeft)
  {
    _CalibrationSettleLeft--;
    return;
  }

  _CalibrationSumUs += WidthUs;
  _CalibrationSampleNb++;

  if(_CalibrationSampleNb <
     _CalibrationAveragePulseNb)
  {
    return;
  }

  const uint16_t AverageUs =
      (uint16_t)(
          (_CalibrationSumUs +
           (_CalibrationAveragePulseNb / 2U)) /
          _CalibrationAveragePulseNb);

  for(uint8_t SymbolIdx = 0;
      SymbolIdx <
      RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      SymbolIdx++)
  {
    const uint16_t TargetUs =
        RCUL_I2C_POT_TX_RCUL_MIN_WIDTH_US +
        (uint16_t)SymbolIdx *
        RCUL_I2C_POT_TX_RCUL_STEP_WIDTH_US;

    const uint16_t ErrorUs =
        (AverageUs > TargetUs)
        ? (AverageUs - TargetUs)
        : (TargetUs - AverageUs);

    if(ErrorUs <
       _CalibrationBestError[SymbolIdx])
    {
      _CalibrationBestError[SymbolIdx] =
          ErrorUs;

      _StoredRculWiperTable[SymbolIdx] =
          _CalibrationWiper;
    }
  }

  if(_CalibrationStream &&
     ((_CalibrationWiper & 0x0F) == 0))
  {
    _CalibrationStream->print(
        F("[RCUL_TABLE_CAL] wiper="));

    _CalibrationStream->print(
        _CalibrationWiper);

    _CalibrationStream->print(
        F(" pwm="));

    _CalibrationStream->println(AverageUs);
  }

  if(_CalibrationWiper >=
     _DeviceMaxWiper)
  {
    finishRculTableCalibration(
        validateRculTable(
            _StoredRculWiperTable,
            _CalibrationBestError));

    return;
  }

  _CalibrationWiper++;
  _CalibrationSettleLeft =
      _CalibrationSettlePulseNb;
  _CalibrationSampleNb = 0;
  _CalibrationSumUs = 0;

  if(!wiper(_CalibrationWiper))
  {
    finishRculTableCalibration(false);
  }
}

void RculI2cPotTxClass::
finishRculTableCalibration(bool Success)
{
  _CalibrationActive = false;
  _CalibrationFailed = !Success;

  if(Success && saveStoredRculTable())
  {
    _RculTableMode =
        RCUL_I2C_POT_TABLE_STORED;

    if(_CalibrationStream)
    {
      _CalibrationStream->println(
          F("[RCUL_TABLE_CAL_OK]"));

      displayRculTable(
          *_CalibrationStream);
    }
  }
  else
  {
    _RculTableMode =
        RCUL_I2C_POT_TABLE_DEFAULT;

    if(_CalibrationStream)
    {
      _CalibrationStream->println(
          F("[RCUL_TABLE_CAL_FAILED]"));
    }
  }

  width_us(_CalibrationSavedWidthUs);
}

bool RculI2cPotTxClass::
isRculTableCalibrationActive(void) const
{
  return _CalibrationActive;
}

bool RculI2cPotTxClass::
didRculTableCalibrationFail(void) const
{
  return _CalibrationFailed;
}

void RculI2cPotTxClass::displayRculTable(
    Stream &Out) const
{
  Out.print(F("[RCUL_TABLE] mode="));

  Out.println(
      _RculTableMode ==
      RCUL_I2C_POT_TABLE_STORED
      ? F("STORED")
      : F("DEFAULT"));

  for(uint8_t Idx = 0;
      Idx < RCUL_I2C_POT_TX_RCUL_SYMBOL_NB;
      Idx++)
  {
    const char Symbol =
        (Idx < 16)
        ? "0123456789ABCDEF"[Idx]
        : ((Idx == 16) ? 'R' : 'I');

    Out.print(Symbol);
    Out.print('=');

    if(_RculTableMode ==
       RCUL_I2C_POT_TABLE_STORED)
    {
      Out.print(
          _StoredRculWiperTable[Idx]);
    }
    else
    {
      Out.print(defaultRculWiper(Idx));
    }

    if(Idx <
       RCUL_I2C_POT_TX_RCUL_SYMBOL_NB - 1)
    {
      Out.print(' ');
    }
  }

  Out.println();
}

#endif


uint16_t RculI2cPotTxClass::getWidth_us(void) const { return _CurrentWidthUs; }
uint16_t RculI2cPotTxClass::getWiper(void) const { return _CurrentWiper; }
uint16_t RculI2cPotTxClass::getMaxWiper(void) const { return _DeviceMaxWiper; }
uint8_t RculI2cPotTxClass::getI2cAddress(void) const { return _Address; }
RculI2cPotType_t RculI2cPotTxClass::getPotType(void) const { return _PotType; }

/*
 * Test de presence I2C par transmission vide vers l'adresse active.
 */
bool RculI2cPotTxClass::isConnected(void)
{
  if(!_Begun)
  {
    return false;
  }

#if RCUL_I2C_POT_TX_USE_TINYWIREM

  /*
   * TinyWireM address-only probes are not reliable with every slave.
   * Connection is therefore represented by the result of the latest real
   * device write. beginCommon() performs such a write before succeeding.
   */
  return _LastWriteOk;

#else

  if(_Wire == NULL)
  {
    return false;
  }

  i2cBeginTransmission(_Address);
  return (i2cEndTransmission() == 0);

#endif
}

/*
 * Rapport de diagnostic lisible sur tout objet derive de Stream.
 */
void RculI2cPotTxClass::printInfo(Stream &Out)
{
  Out.println(F("----------------------------------------"));
  Out.print(F("RculI2cPotTx V")); Out.println(F(RCUL_I2C_POT_TX_VERSION_STRING));
  Out.print(F("Pot type     : ")); Out.println(_PotType == DS3502 ? F("DS3502") : F("MCP4561"));
#if RCUL_I2C_POT_TX_USE_TINYWIREM
  Out.println(F("I2C driver   : TinyWireM"));
#else
  Out.println(F("I2C driver   : Wire"));
#endif
  Out.print(F("I2C address  : 0x")); if(_Address < 0x10) Out.print('0'); Out.println(_Address, HEX);
  Out.print(F("I2C clock    : ")); Out.print(_I2cFrequency); Out.println(F(" Hz"));
  Out.print(F("Syncro mode  : "));
  switch(_SyncroMode)
  {
    case RCUL_I2C_POT_SYNCRO_MODE_PPMIN:
      Out.println(F("PPM IN"));
      break;
    case RCUL_I2C_POT_SYNCRO_MODE_CALLBACK:
      Out.println(F("CALLBACK"));
      break;
    case RCUL_I2C_POT_SYNCRO_MODE_INTERNAL:
    default:
      Out.println(F("INTERNAL"));
      break;
  }
  Out.print(F("Wiper        : ")); if(_CurrentWiper == 0xFFFF) Out.print(F("unknown")); else Out.print(_CurrentWiper);
  Out.print(F(" / ")); Out.println(_DeviceMaxWiper);
  Out.print(F("Wiper range  : ")); Out.print(_MinWiper); Out.print(F(" .. ")); Out.print(_MaxWiper);
  Out.println(_CustomWiperRange ? F(" (custom)") : F(" (default)"));
  Out.print(F("RC width     : ")); Out.print(_MinWidthUs); Out.print(F(" .. ")); Out.print(_MaxWidthUs); Out.println(F(" us (internal)"));
  if(_PotType == DS3502)
  {
    Out.println(F("DS3502 mode  : volatile WR (EEPROM protected)"));
    Out.print(F("RCUL table   : "));
#if RCUL_I2C_POT_TX_TABLE_LEARNING_SUPPORTED
    Out.println(
        _RculTableMode ==
        RCUL_I2C_POT_TABLE_STORED
        ? F("stored calibration")
        : F("embedded default"));
#else
    Out.println(F("embedded default"));
#endif
  }
  Out.print(F("Connected    : ")); Out.println(isConnected() ? F("YES") : F("NO"));
  Out.println(F("----------------------------------------"));
}

/*
 * Adaptateurs virtuels imposes par l'interface Rcul.
 * Le canal est ignore car une instance correspond a une seule sortie
 * analogique physique.
 */
uint8_t RculI2cPotTxClass::RculIsSynchro(uint8_t ClientIdx) { return isSynchro(ClientIdx); }
uint16_t RculI2cPotTxClass::RculGetWidth_us(uint8_t Ch) { (void)Ch; return getWidth_us(); }
void RculI2cPotTxClass::RculSetWidth_us(uint16_t Width_us, uint8_t Ch) { (void)Ch; width_us(Width_us); }
