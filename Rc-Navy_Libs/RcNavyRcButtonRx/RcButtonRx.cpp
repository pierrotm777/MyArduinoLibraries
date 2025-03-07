/*
 English: by RC Navy (2024)
 =======
 <RcButtonRx> is a library designed to read RC pulse signal to make actions from a keyboard (push-buttons + resistors) connected to a free channel of an RC transmitter.
 This library manages the mandatory calibration phase (an hardware or software serial interface is needed).
 The action associated to each push-button can be set in pulse mode (sometime called memory mode).
 This <RcButtonRx> library is intended to facilitate the design of a decoder placed at RC receiver side.
 With this library, the exploitation of the commands from the push-buttons are greatly facilitated.
 In case of lost signal, all the commands are set to 0 after 2 seconds (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: initial release
 V1.1: (dd/mm/yyyy) Wiil be the next Release

 Francais: par RC Navy (2024)
 ========
<RcButtonRx> est une bibliotheque concue pour lire les largeurs d'impulsions RC pour faire des actions à partir d'un clavier (boutons-poussoirs + resistances) connecté
a une voie libre d'un emetteur RC.
Cette bibliotheque gere la phase nécessaire de calibration (une interface serie hardware ou software est requise).
La bibliotheque <RcButtonRx> est destinee a faciliter la conception de decodeur place cote recepteur RC.
Avec cette bibliotheque, l'exploitation des commandes depuis les boutons-poussoirs est grandement facilitee.
En cas de perte de signal, toutes les commandes sont mises à 0 apres 2 secondes (Failsafe).

 http://p.loussouarn.free.fr
 V1.0: release initiale
 V1.1: (dd/mm/yyyy) Sera la prochaine Release
*/


#include "RcButtonRx.h"

#if defined(ARDUINO_ARCH_RP2040)
#include <EEPROM.h>
#elif defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40)
#include <EEPROM.h>
#elif defined(__MKL26Z64__)
#include <EEPROM.h>
#endif

#define RC_BUTTON_MAX_NB                                            14 /* Do NOT chnage this value */

#if defined(ARDUINO_ARCH_RP2040)
#define GET_PULSE_MAP(EepromBaseAddr)                               EEPROM16_Read((uint16_t)(EepromBaseAddr))
#define SET_PULSE_MAP(EepromBaseAddr, PulseMap)                     EEPROM16_Write((uint16_t)(EepromBaseAddr), (uint16_t)(PulseMap))
#define GET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx)           EEPROM16_Read((uint16_t)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)))
#define SET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx, WidthUs)  EEPROM16_Write((uint16_t)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)), (WidthUs))

#elif defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40)
#define GET_PULSE_MAP(EepromBaseAddr)                               EEPROM16_Read((uint16_t)(EepromBaseAddr))
#define SET_PULSE_MAP(EepromBaseAddr, PulseMap)                     EEPROM16_Write((uint16_t)(EepromBaseAddr), (uint16_t)(PulseMap))
#define GET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx)           EEPROM16_Read((uint16_t)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)))
#define SET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx, WidthUs)  EEPROM16_Write((uint16_t)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)), (WidthUs))

#elif defined(__MKL26Z64__)
#define GET_PULSE_MAP(EepromBaseAddr)                               EEPROM16_Read((uint16_t)(EepromBaseAddr))
#define SET_PULSE_MAP(EepromBaseAddr, PulseMap)                     EEPROM16_Write((uint16_t)(EepromBaseAddr), (uint16_t)(PulseMap))
#define GET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx)           EEPROM16_Read((uint16_t)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)))
#define SET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx, WidthUs)  EEPROM16_Write((uint16_t)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)), (WidthUs))

#else
#define GET_PULSE_MAP(EepromBaseAddr)                               eeprom_read_word  ((uint16_t*)(EepromBaseAddr))
#define SET_PULSE_MAP(EepromBaseAddr, PulseMap)                     eeprom_update_word((uint16_t*)(EepromBaseAddr), (uint16_t*)(PulseMap))
#define GET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx)           eeprom_read_word  ((uint16_t*)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)))
#define SET_BUTTON_PULSE_WIDTH(EepromBaseAddr, ButtonIdx, WidthUs)  eeprom_update_word((uint16_t*)((EepromBaseAddr) + (((ButtonIdx) + 1) * 2)), (WidthUs))
#endif

#define VALID_CMD_TIME_PULSE_NB                                     3  /* Needs at least  3 consecutive valid RC pulses to enable/disable the output */
#define INTER_CMD_TIME_PULSE_NB                                     20 /* Needs at least 20 consecutive RC pulses before accepting a new command for an output configured in Pulse Mode */

#define PUSH_BUTTON_TOLERENCE_US                                    10 /* 20 +/- */ // modifié pour être compatible avec une mc-14

#define NO_SIGNAL_MAX_TIME_MS                                       2000

#define NO_BUTTON_IDX                                               0x0F

#define CR  0x0D

/* Line terminator (terminal dependent) */
const char _CRLF_STR_[] PROGMEM = "\n";
const char   _CR_STR_[] PROGMEM = "\r";
DECL_FLASH_STR_TBL(EndOfLineTbl)       = {_CRLF_STR_, _CR_STR_};

#define isNormalMode(ButtonId)  !isPulseMode(ButtonId)

#if defined(ARDUINO_ARCH_RP2040) || defined(__MKL26Z64__) || (defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40))
void EEPROM16_Write(uint8_t a, uint16_t b){
  EEPROM.write(a, lowByte(b));
  EEPROM.write(a + 1, highByte(b));
#if defined(ARDUINO_ARCH_RP2040)
  EEPROM.commit();
#endif
}

uint16_t EEPROM16_Read(uint8_t a){
  return word(EEPROM.read(a + 1), EEPROM.read(a));
}
#endif

/* Constructor */
RcButtonRx::RcButtonRx()
{

}

void RcButtonRx::begin(Stream *MyStream, uint8_t CrLineTerm, Rcul *MyRcul, uint8_t ChId, uint8_t ButtonNb, uint8_t ClientIdx /*= 5*/, uint16_t EepromBaseAddr /*= 0*/)
{
  _Priv.MyStream          = MyStream;
  _Priv.CrLineTerm        = CrLineTerm;
  _Priv.MyRcul            = MyRcul;
  _Priv.ChId              = ChId;
  _Priv.ButtonNb          = (ButtonNb <= RC_BUTTON_MAX_NB)? ButtonNb: RC_BUTTON_MAX_NB;
  _Priv.ButAcqState       = 0;
  _Priv.ClientIdx         = ClientIdx;
  _Priv.EepromBaseAddr    = EepromBaseAddr;
  _Priv.Calibration       = 0;
  _Priv.RcPulseValidNb    = 0;
  _Priv.RcPulseInterCmdNb = 0;
  _Priv.Outputs           = 0;
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
  _Priv.MyStream->print(F("BP1: "));
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
#if defined(ARDUINO_ARCH_RP2040)
  EEPROM16_Write((uint16_t)(_Priv.EepromBaseAddr), PulseMap);
#elif defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40)
  EEPROM16_Write((uint16_t)(_Priv.EepromBaseAddr), PulseMap);
#elif defined(__MKL26Z64__)
  EEPROM16_Write((uint16_t)(_Priv.EepromBaseAddr), PulseMap);
#else
  eeprom_update_word((uint16_t*)(_Priv.EepromBaseAddr), PulseMap);
#endif
}

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
          if ((WidthUs > 1000) && (WidthUs < 2000))
		  {
            SET_BUTTON_PULSE_WIDTH(_Priv.EepromBaseAddr, _Priv.ButtonIdx, WidthUs);
            _Priv.MyStream->print(WidthUs);eol();
            if(_Priv.ButtonIdx < (_Priv.ButtonNb - 1))
            {
              _Priv.ButtonIdx++;
              _Priv.MyStream->print(F("BP"));_Priv.MyStream->print(_Priv.ButtonIdx + 1);_Priv.MyStream->print(F(": "));
            }
            else
            {
              _Priv.Calibration = 0;
            }		  
		  }
		  else
		  {
             Serial.println("Bad Value received !");
		  }
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

void RcButtonRx::displayButtonPulseWidth(void)
{
  _Priv.MyStream->print(F("Buttons:   "));
  for(uint8_t Idx = 0; Idx < _Priv.ButtonNb; Idx++)
  {
#if defined(ARDUINO_ARCH_RP2040) 
    _Priv.MyStream->print(EEPROM16_Read((uint16_t)(_Priv.EepromBaseAddr + ((Idx + 1) * 2))));
#elif defined(__IMXRT1062__) && defined(ARDUINO_TEENSY40)
    _Priv.MyStream->print(EEPROM16_Read((uint16_t)(_Priv.EepromBaseAddr + ((Idx + 1) * 2))));
#elif defined(__MKL26Z64__)
    _Priv.MyStream->print(EEPROM16_Read((uint16_t)(_Priv.EepromBaseAddr + ((Idx + 1) * 2))));
#else
    _Priv.MyStream->print(eeprom_read_word((uint16_t*)(_Priv.EepromBaseAddr + ((Idx + 1) * 2))));
#endif
    if(Idx < (_Priv.ButtonNb - 1)) _Priv.MyStream->print(F(";"));
  }
  //eol();
}


