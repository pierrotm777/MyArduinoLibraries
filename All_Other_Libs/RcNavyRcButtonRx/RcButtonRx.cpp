/*
 English: by RC Navy (2024-2025)
 =======
 <RcButtonRx> is a library designed to read RC pulse signal to make actions from a keyboard (push-buttons + resistors) connected to a free channel of an RC transmitter.
 This library manages the mandatory calibration phase (an hardware or software serial interface is needed).
 The action associated to each push-button can be set in pulse mode (sometime called memory mode).
 This <RcButtonRx> library is intended to facilitate the design of a decoder placed at RC receiver side.
 With this library, the exploitation of the commands from the push-buttons are greatly facilitated.
 In case of lost signal, all the commands are set to 0 after 2 seconds (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: initial release
 V1.1: (22/12/2024) One wire serial support added and #include <avr/eeprom.h> added and tolerance reduction from -/+20 to -/+15
 V1.2: (08/05/2025) Support for Teensy 4.0 and RP2040 targets, CRLF line terminator fixed (was sending only LF)
 V1.3: (30/05/2025) Added call-back function support when exiting from calibration (can be use to update non-volatile memory checksum).
                    This feature is dependent of the "#define RC_BUTTON_RX_ON_EXIT_CAL" compilation directive in RcButtonRx.h.
 V1.4: (13/05/2026) Added ESP32 EEPROM emulation support.

 Francais: par RC Navy (2024-2025)
 ========
<RcButtonRx> est une bibliotheque concue pour lire les largeurs d'impulsions RC pour faire des actions à partir d'un clavier (boutons-poussoirs + resistances) connecté
a une voie libre d'un emetteur RC.
Cette bibliotheque gere la phase nécessaire de calibration (une interface serie hardware ou software est requise).
La bibliotheque <RcButtonRx> est destinee a faciliter la conception de decodeur place cote recepteur RC.
Avec cette bibliotheque, l'exploitation des commandes depuis les boutons-poussoirs est grandement facilitee.
En cas de perte de signal, toutes les commandes sont mises à 0 apres 2 secondes (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: release initiale
 V1.1: (22/12/2024) Ajout support pour serial sur un seul fil et ajout #include <avr/eeprom.h> et reduction tolerance de -/+20 à -/+15
 V1.2: (08/05/2025) Ajout support pour cibles Teensy 4.0 et RP2040, le terminateur de ligne CRLF est corrige (il n'envoyait que LF)
 V1.3: (30/05/2025) Ajout support pour fonction call-back a la sortie du mode de calibration (peut être utilisé pour mettre à jour le checksum d'une memoire non volatile).
                    Cette fonctionnalité est dependante de la directive de compilation "#define RC_BUTTON_RX_ON_EXIT_CAL" dans RcButtonRx.h
 V1.4: (13/05/2026) Ajout du support ESP32 avec EEPROM emulee en Flash.
*/
#include "RcButtonRx.h"

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32) || defined(__MKL26Z64__) || (defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40))
#include <EEPROM.h>
#define EEPROM_READ_WORD(a)       EEPROM16_Read((a))
#define EEPROM_UPDATE_WORD(a, b)  EEPROM16_Update((a), (b))
#else
#define EEPROM_READ_WORD(a)       eeprom_read_word((uint16_t*)(a))
#define EEPROM_UPDATE_WORD(a, b)  eeprom_update_word((uint16_t*)(a), (b))
#endif

#define RC_BUTTON_MAX_NB                                            14 /* Do NOT change this value */

#define GET_PULSE_MAP(EepromBaseAddr)                               EEPROM_READ_WORD  ((EepromBaseAddr))
#define SET_PULSE_MAP(EepromBaseAddr, PulseMap)                     EEPROM_UPDATE_WORD((EepromBaseAddr), (PulseMap))

#define GET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx)           EEPROM_READ_WORD  ((EepromBaseAddr) + (((ButtonIdx) + 1) * 2))
#define SET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx, WidthUs)  EEPROM_UPDATE_WORD((EepromBaseAddr) + (((ButtonIdx) + 1) * 2), (WidthUs))

#define VALID_CMD_TIME_PULSE_NB                                     3  /* Needs at least  3 consecutive valid RC pulses to enable/disable the output */
#define INTER_CMD_TIME_PULSE_NB                                     20 /* Needs at least 20 consecutive RC pulses before accepting a new command for an output configured in Pulse Mode */

#define PUSH_BUTTON_TOLERENCE_US                                    15 /* +/- */

#define NO_SIGNAL_MAX_TIME_MS                                       2000

#define NO_BUTTON_IDX                                               0x0F

#define CR  0x0D

/* Line terminator (terminal dependent) */
const char _CRLF_STR_[] PROGMEM = "\r\n"; // CR + LF
const char   _CR_STR_[] PROGMEM = "\r";   // CR only
DECL_FLASH_STR_TBL(EndOfLineTbl)       = {_CRLF_STR_, _CR_STR_};

#define isNormalMode(ButtonId)  !isPulseMode(ButtonId)

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32) || defined(__MKL26Z64__) || (defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40))

uint16_t EEPROM16_Read(uint16_t a)
{
  return word(EEPROM.read((uint16_t)a + 1), EEPROM.read((uint16_t)a + 0));
}

void EEPROM16_Update(uint16_t a, uint16_t b)
{
  uint8_t Change = 0;

  if(EEPROM.read(a + 0) !=  lowByte(b)) {EEPROM.write((a + 0),  lowByte(b)); Change = 1;}
  if(EEPROM.read(a + 1) != highByte(b)) {EEPROM.write((a + 1), highByte(b)); Change = 1;}

  #if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_ESP32)
  if(Change) EEPROM.commit();
  #else
  Change = Change; // To avoid a compilation warning when not an RP2040/ESP32
  #endif
}

#endif

/* Constructor */
RcButtonRx::RcButtonRx()
{

}

void RcButtonRx::begin(Stream *MyStream,
                        #ifdef RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT
                        void (*TxMode)(void), void (*RxMode)(void),
                        #endif
                        uint8_t CrLineTerm, Rcul *MyRcul, uint8_t ChId, uint8_t ButtonNb, uint8_t ClientIdx /*= 5*/, uint16_t EepromBaseAddr /*= 0*/)
{
  _Priv.MyStream          = MyStream;
#ifdef RC_BUTTON_RX_ON_EXIT_CAL
  _Priv.onExitAction      = nullptr;
#endif
  #ifdef RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT
  _Priv.TxMode            = TxMode;
  _Priv.RxMode            = RxMode;
  #endif
  _Priv.CrLineTerm        = CrLineTerm;
  _Priv.MyRcul            = MyRcul;
  _Priv.ChId              = ChId;
  _Priv.ButtonNb          = (ButtonNb <= RC_BUTTON_MAX_NB)? ButtonNb: RC_BUTTON_MAX_NB;
  _Priv.ButAcqState       = 0;
  _Priv.ClientIdx         = ClientIdx;
  _Priv.EepromBaseAddr    = EepromBaseAddr;
#if defined(ARDUINO_ARCH_ESP32)
  /* ESP32 has no real EEPROM: EEPROM.h emulates it in Flash.
     The emulated area must be initialized before any read/write.
     Size includes the base offset because EepromBaseAddr can be non-zero. */
  EEPROM.begin((uint16_t)(_Priv.EepromBaseAddr + getStoredEepromBytes()));
#endif
  _Priv.Calibration       = 0;
  _Priv.RcPulseValidNb    = 0;
  _Priv.RcPulseInterCmdNb = 0;
  _Priv.Outputs           = 0;
}

void RcButtonRx::txMode(void)
{
  #ifdef RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT
  _Priv.TxMode();
  #endif
}

void RcButtonRx::rxMode(void)
{
  #ifdef RC_BUTTON_RX_SERIAL_1W_MODE_SUPPORT
  _Priv.RxMode();
  #endif
}

uint8_t RcButtonRx::getStoredEepromBytes(void)
{
  return(RC_BUTTON_RX_EEPROM_BYTES(_Priv.ButtonNb));
}

void RcButtonRx::eol(void)
{
  _Priv.MyStream->print((const __FlashStringHelper *)pgm_read_ptr(&EndOfLineTbl[!!_Priv.CrLineTerm]));
}

void RcButtonRx::enterInCalibrationMode(void)
{
  _Priv.ButtonIdx   = 0;
  _Priv.Calibration = 1;
  txMode();
  _Priv.MyStream->print(F("BP1: "));
  rxMode();
  delay(10);
  if(_Priv.MyStream->available()) _Priv.MyStream->read(); /* Flush an eventual CR */
  if(_Priv.MyStream->available()) _Priv.MyStream->read(); /* Flush an eventual LF */
}

uint8_t RcButtonRx::isInCalibrationMode(void)
{
  return(_Priv.Calibration);
}

void RcButtonRx::setPulseMap(uint16_t PulseMap)
{
  SET_PULSE_MAP(_Priv.EepromBaseAddr, PulseMap);
}

#ifdef RC_BUTTON_RX_ON_EXIT_CAL
void RcButtonRx::onExitCalibrationMode(void (*onExitAction)(void))
{
  _Priv.onExitAction = onExitAction;
}
#endif

void RcButtonRx::setPulseMode(uint8_t ButtonId, uint8_t PulseMode)
{
  uint16_t PulseMap;

  if(ButtonId && ButtonId <= _Priv.ButtonNb)
  {
    PulseMap = GET_PULSE_MAP(_Priv.EepromBaseAddr);
    bitWrite(PulseMap, ButtonId - 1, !!PulseMode);
    SET_PULSE_MAP(_Priv.EepromBaseAddr, PulseMap);
  }
}

uint8_t RcButtonRx::isPulseMode(uint8_t ButtonId)
{
  int8_t Ret = 1;

  if(ButtonId && ButtonId <= _Priv.ButtonNb)
  {
    Ret = !!bitRead(GET_PULSE_MAP(_Priv.EepromBaseAddr), ButtonId - 1);
  }

  return(Ret);
}

uint16_t RcButtonRx::process(void)
{
  uint16_t        WidthUs;
  uint8_t         RxChar;
  uint8_t         PushButtonIdx;
  static uint16_t StartMs16 = millis16();

  if(_Priv.MyRcul->RculIsSynchro(_Priv.ClientIdx))
  {
    StartMs16 = millis16();
    if(_Priv.Calibration)
    {
      if(_Priv.MyStream->available())
      {
        RxChar = _Priv.MyStream->read();
        if(RxChar == CR)
        {
          WidthUs = _Priv.MyRcul->RculGetWidth_us(_Priv.ChId);
          SET_BUTTON_PULSE_WIDTH(_Priv.EepromBaseAddr, _Priv.ButtonIdx, WidthUs);
          txMode();
          _Priv.MyStream->print(WidthUs);eol();
          if(_Priv.ButtonIdx < (_Priv.ButtonNb - 1))
          {
            _Priv.ButtonIdx++;
            _Priv.MyStream->print(F("BP"));_Priv.MyStream->print(_Priv.ButtonIdx + 1);_Priv.MyStream->print(F(": "));
          }
          else
          {
            /* Was the last button -> Exit from calibration and call onExitAction() call back function if set */
            _Priv.Calibration = 0;
            #ifdef RC_BUTTON_RX_ON_EXIT_CAL
            if(_Priv.onExitAction) _Priv.onExitAction();
            #endif
          }
          rxMode();
        }
      }
    }
    else
    {
      enum {BUT_IDLE_TO_PRESS = 0, BUT_PRESS_CONFIRM, BUT_PRESS_TO_RELASE, BUT_CONFIRM_RELEASE};
      if(_Priv.RcPulseInterCmdNb < INTER_CMD_TIME_PULSE_NB) _Priv.RcPulseInterCmdNb++;
      PushButtonIdx = getPushButtonIdx(_Priv.MyRcul->RculGetWidth_us(_Priv.ChId));
      switch(_Priv.ButAcqState)
      {
        case BUT_IDLE_TO_PRESS:
        if(PushButtonIdx != NO_BUTTON_IDX)
        {
          _Priv.InProgButtonIdx = PushButtonIdx;
          _Priv.RcPulseValidNb = 0;
          _Priv.ButAcqState = BUT_PRESS_CONFIRM;
          //Serial.println("Go to BUT_PRESS_CONFIRM");
        }
        break;

        case BUT_PRESS_CONFIRM:
        if(PushButtonIdx == _Priv.InProgButtonIdx)
        {
          _Priv.RcPulseValidNb++;
          if(_Priv.RcPulseValidNb >= VALID_CMD_TIME_PULSE_NB) /* OK: button has been pressed for valid time */
          {
            _Priv.ButtonIdx = _Priv.InProgButtonIdx;
            if(isPulseMode(_Priv.ButtonIdx + 1))
            {
              /* Pulse Mode */
              if (_Priv.RcPulseInterCmdNb == INTER_CMD_TIME_PULSE_NB)
              {
                _Priv.Outputs ^= (1 << _Priv.ButtonIdx); // Flip the output bit
                _Priv.RcPulseInterCmdNb = 0;
              }
            }
            else
            {
              /* Normal Mode */
              bitWrite(_Priv.Outputs, _Priv.ButtonIdx, 1); // Enable the output
            }
            _Priv.ButAcqState = BUT_PRESS_TO_RELASE;
            //Serial.println("Go to BUT_PRESS_TO_RELASE");
          }
        }
        else _Priv.ButAcqState = BUT_IDLE_TO_PRESS;
        break;

        case BUT_PRESS_TO_RELASE:
        if(PushButtonIdx == NO_BUTTON_IDX)
        {
          _Priv.RcPulseValidNb = 0;
          _Priv.ButAcqState = BUT_CONFIRM_RELEASE;
          //Serial.println("Go to BUT_CONFIRM_RELEASE");
        }
        break;

        case BUT_CONFIRM_RELEASE:
        if(PushButtonIdx == NO_BUTTON_IDX)
        {
          _Priv.RcPulseValidNb++;
          if(_Priv.RcPulseValidNb >= VALID_CMD_TIME_PULSE_NB)  /* OK: button has been released for valid time */
          {
            if(isNormalMode(_Priv.ButtonIdx + 1))
            {
              /* Normal Mode */
              bitWrite(_Priv.Outputs, _Priv.ButtonIdx, 0); // Enable the output
            }
            _Priv.ButAcqState = BUT_IDLE_TO_PRESS;
            //Serial.println("Go to BUT_IDLE_TO_PRESS");
          }
        }
        else _Priv.ButAcqState = BUT_PRESS_TO_RELASE;
        break;
      }
    }
  }
  else
  {
    if(ElapsedMs16Since(StartMs16) >= NO_SIGNAL_MAX_TIME_MS)
    {
      StartMs16 = millis16();
      _Priv.Outputs = 0; /* Failsafe */
    }
  }
  return(_Priv.Outputs);
}

uint8_t RcButtonRx::getPushButtonIdx(uint16_t WidthUs)
{
  uint8_t  Ret;
  uint16_t Typ, Min, Max;

	Ret = NO_BUTTON_IDX; /* Init at Not Found */
	for(uint8_t ButtonIdx = 0; ButtonIdx < _Priv.ButtonNb; ButtonIdx++)
	{
		Typ = GET_BUTTON_PULSE_WIDTH(_Priv.EepromBaseAddr, ButtonIdx);
		Min = Typ - PUSH_BUTTON_TOLERENCE_US;
		Max = Typ + PUSH_BUTTON_TOLERENCE_US;
		if((WidthUs >= Min) && (WidthUs <= Max))
		{
			Ret = ButtonIdx;
			break;
		}
	}
	return(Ret);
}

void RcButtonRx::displayButtonPulseWidth(char *Prefix /*= NULL*/)
{
  txMode();
  if(Prefix == NULL) _Priv.MyStream->print(F("B="));
  for(uint8_t Idx = 0; Idx < _Priv.ButtonNb; Idx++)
  {
    _Priv.MyStream->print(EEPROM_READ_WORD(_Priv.EepromBaseAddr + ((Idx + 1) * 2)));
    if(Idx < (_Priv.ButtonNb - 1)) _Priv.MyStream->print(F(";"));
  }
  eol();
  rxMode();
}
