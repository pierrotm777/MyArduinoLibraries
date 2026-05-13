#include "ProMicroEngineSound.h"

#if !defined(__AVR_ATmega32U4__)
#warning "ProMicroEngineSound is intended for ATmega32u4 / Arduino Pro Micro."
#endif

ProMicroEngineSoundClass ProMicroEngineSound;
static ProMicroEngineSoundClass *g_engineSound = nullptr;

ISR(TIMER1_COMPA_vect)
{
  if (g_engineSound) {
    g_engineSound->onSampleTick();
  }
}

bool ProMicroEngineSoundClass::begin(uint8_t sdCsPin,
                                     const char *startFile,
                                     const char *idleFile,
                                     uint16_t sampleRate)
{
  _sdCsPin = sdCsPin;
  _startFileName = startFile;
  _idleFileName = idleFile;
  _sampleRate = sampleRate;

  _audioHead = 0;
  _audioTail = 0;
  _sourcePos = 0;
  _sourceLen = 0;
  _lastSourceSample = 128;
  _state = STOPPED;
  _dataStart = 0;
  _dataEnd = 0;
  _fileSampleRate = 0;
  _currentIsWav = false;

  if (!SD.begin(_sdCsPin)) {
    return false;
  }

  setupAudioPwmTimer4();
  setupSampleTimer1(_sampleRate);
  g_engineSound = this;

  return true;
}

void ProMicroEngineSoundClass::setFiles(const char *startFile, const char *idleFile)
{
  _startFileName = startFile;
  _idleFileName = idleFile;
}

void ProMicroEngineSoundClass::startEngine()
{
  _audioHead = 0;
  _audioTail = 0;
  _sourcePos = 0;
  _sourceLen = 0;
  _phaseQ8 = 0;

  if (!openStart()) {
    openIdle();
  }
}

void ProMicroEngineSoundClass::stop()
{
  closeCurrent();
  _state = STOPPED;
  _audioHead = 0;
  _audioTail = 0;
  OCR4D = 128;
}

void ProMicroEngineSoundClass::update()
{
  if (_state == STOPPED) {
    return;
  }
  refillAudioBuffer();
}

void ProMicroEngineSoundClass::setThrottleUs(uint16_t us)
{
  setThrottleUs(us, _thrMinUs, _thrMaxUs);
}

void ProMicroEngineSoundClass::setThrottleUs(uint16_t us, uint16_t minUs, uint16_t maxUs)
{
  _thrMinUs = minUs;
  _thrMaxUs = maxUs;

  if (us < minUs) us = minUs;
  if (us > maxUs) us = maxUs;

  const uint32_t span = (uint32_t)(maxUs - minUs);
  const uint32_t pos = (uint32_t)(us - minUs);
  if (span == 0) {
    _pitchQ8 = _pitchIdleQ8;
  } else {
    _pitchQ8 = _pitchIdleQ8 + (uint16_t)(((uint32_t)(_pitchFullQ8 - _pitchIdleQ8) * pos) / span);
  }
}

void ProMicroEngineSoundClass::setPitchRangeQ8(uint16_t idleQ8, uint16_t fullQ8)
{
  _pitchIdleQ8 = idleQ8;
  _pitchFullQ8 = fullQ8;
}

void ProMicroEngineSoundClass::setVolume(uint8_t volume)
{
  _volume = volume;
}

bool ProMicroEngineSoundClass::openStart()
{
  return openSoundFile(_startFileName, STARTING);
}

bool ProMicroEngineSoundClass::openIdle()
{
  const bool ok = openSoundFile(_idleFileName, IDLE);
  _phaseQ8 = 0;
  return ok;
}

bool ProMicroEngineSoundClass::openSoundFile(const char *name, State newState)
{
  closeCurrent();
  _file = SD.open(name, FILE_READ);
  if (!_file) {
    _state = STOPPED;
    return false;
  }

  uint32_t dataStart = 0;
  uint32_t dataSize = 0;
  uint16_t rate = 0;
  _currentIsWav = parseWavHeader(_file, dataStart, dataSize, rate);

  if (_currentIsWav) {
    _dataStart = dataStart;
    _dataEnd = dataStart + dataSize;
    _fileSampleRate = rate;
    _file.seek(_dataStart);
  } else {
    // Fallback volontaire : accepte encore un fichier RAW sans header.
    _dataStart = 0;
    _dataEnd = _file.size();
    _fileSampleRate = _sampleRate;
    _file.seek(0);
  }

  _state = newState;
  _sourcePos = 0;
  _sourceLen = 0;
  return true;
}

uint16_t ProMicroEngineSoundClass::readLE16(File &f)
{
  uint16_t v = (uint8_t)f.read();
  v |= ((uint16_t)(uint8_t)f.read()) << 8;
  return v;
}

uint32_t ProMicroEngineSoundClass::readLE32(File &f)
{
  uint32_t v = (uint8_t)f.read();
  v |= ((uint32_t)(uint8_t)f.read()) << 8;
  v |= ((uint32_t)(uint8_t)f.read()) << 16;
  v |= ((uint32_t)(uint8_t)f.read()) << 24;
  return v;
}

bool ProMicroEngineSoundClass::parseWavHeader(File &f, uint32_t &dataStart, uint32_t &dataSize, uint16_t &rate)
{
  if (f.size() < 44) {
    return false;
  }

  f.seek(0);
  char id[4];
  if (f.read((uint8_t *)id, 4) != 4) return false;
  if (id[0] != 'R' || id[1] != 'I' || id[2] != 'F' || id[3] != 'F') return false;
  (void)readLE32(f); // RIFF size
  if (f.read((uint8_t *)id, 4) != 4) return false;
  if (id[0] != 'W' || id[1] != 'A' || id[2] != 'V' || id[3] != 'E') return false;

  bool fmtFound = false;
  bool dataFound = false;
  uint16_t audioFormat = 0;
  uint16_t channels = 0;
  uint16_t bits = 0;

  while (f.available() >= 8) {
    if (f.read((uint8_t *)id, 4) != 4) break;
    const uint32_t chunkSize = readLE32(f);
    const uint32_t chunkDataPos = f.position();

    if (id[0] == 'f' && id[1] == 'm' && id[2] == 't' && id[3] == ' ') {
      audioFormat = readLE16(f);
      channels = readLE16(f);
      rate = (uint16_t)readLE32(f);  // 16000 Hz dans ton DSL-V12.STA.
      (void)readLE32(f);             // byteRate
      (void)readLE16(f);             // blockAlign
      bits = readLE16(f);
      fmtFound = true;
    } else if (id[0] == 'd' && id[1] == 'a' && id[2] == 't' && id[3] == 'a') {
      dataStart = chunkDataPos;
      dataSize = chunkSize;
      dataFound = true;
      break;
    }

    uint32_t next = chunkDataPos + chunkSize;
    if (chunkSize & 1) next++; // alignement RIFF pair
    if (next >= f.size()) break;
    f.seek(next);
  }

  // Format accepté : Microsoft PCM, mono, unsigned 8 bits.
  return fmtFound && dataFound && audioFormat == 1 && channels == 1 && bits == 8;
}

void ProMicroEngineSoundClass::closeCurrent()
{
  if (_file) {
    _file.close();
  }
}

uint16_t ProMicroEngineSoundClass::freeAudioSpace() const
{
  const uint8_t head = _audioHead;
  const uint8_t tail = _audioTail;
  return (uint8_t)(tail - head - 1);
}

void ProMicroEngineSoundClass::pushSample(uint8_t v)
{
  const uint8_t next = (uint8_t)(_audioHead + 1);
  if (next == _audioTail) {
    return;
  }
  _audioBuf[_audioHead] = scaleSample(v);
  _audioHead = next;
}

uint8_t ProMicroEngineSoundClass::scaleSample(uint8_t v) const
{
  int16_t centered = (int16_t)v - 128;
  centered = (centered * (int16_t)_volume) / 255;
  centered += 128;
  if (centered < 0) centered = 0;
  if (centered > 255) centered = 255;
  return (uint8_t)centered;
}

bool ProMicroEngineSoundClass::readNextSourceByte(uint8_t &v, bool loopFile)
{
  if (_file.position() >= _dataEnd && _sourcePos >= _sourceLen) {
    if (loopFile) {
      _file.seek(_dataStart);
      _sourcePos = 0;
      _sourceLen = 0;
    } else {
      return false;
    }
  }

  if (_sourcePos >= _sourceLen) {
    uint32_t remaining = _dataEnd - _file.position();
    if (remaining == 0) {
      if (loopFile) {
        _file.seek(_dataStart);
        remaining = _dataEnd - _dataStart;
      } else {
        return false;
      }
    }

    uint8_t toRead = sizeof(_sourceBuf);
    if (remaining < toRead) {
      toRead = (uint8_t)remaining;
    }

    _sourceLen = _file.read(_sourceBuf, toRead);
    _sourcePos = 0;

    if (_sourceLen == 0) {
      return false;
    }
  }

  v = _sourceBuf[_sourcePos++];
  _lastSourceSample = v;
  return true;
}

void ProMicroEngineSoundClass::refillAudioBuffer()
{
  while (freeAudioSpace() > 16 && _state != STOPPED) {
    if (_state == STARTING) {
      uint8_t s;
      if (readNextSourceByte(s, false)) {
        pushSample(s);
      } else {
        openIdle();
      }
    } else {
      while (_phaseQ8 >= 256) {
        uint8_t s;
        if (!readNextSourceByte(s, true)) {
          return;
        }
        _phaseQ8 -= 256;
      }

      pushSample(_lastSourceSample);
      _phaseQ8 += _pitchQ8;
    }
  }
}

void ProMicroEngineSoundClass::onSampleTick()
{
  uint8_t out = 128;
  if (_audioTail != _audioHead) {
    out = _audioBuf[_audioTail];
    _audioTail = (uint8_t)(_audioTail + 1);
  }
  OCR4D = out;
}

void ProMicroEngineSoundClass::setupAudioPwmTimer4()
{
#if defined(__AVR_ATmega32U4__)
  // Arduino Pro Micro D6 = PD7 = OC4D.
  pinMode(6, OUTPUT);

  // Même principe que le HEX : Timer4 est utilisé pour la sortie PWM audio.
  // Le duty audio est écrit dans OCR4D.
  TCCR4A = 0;
  TCCR4B = 0;
  TCCR4C = 0;
  TCCR4D = 0;
  TCCR4E = 0;

  TCCR4B = _BV(CS40);                  // Timer4 actif, sans prescaler.
  TCCR4C = _BV(COM4D1) | _BV(PWM4D);   // PWM sur OC4D / D6.
  OCR4C = 255;
  OCR4D = 128;
#else
  pinMode(6, OUTPUT);
#endif
}

void ProMicroEngineSoundClass::setupSampleTimer1(uint16_t sampleRate)
{
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Le HEX contient aussi un vecteur TIMER1_COMPA actif.
  // Ici Timer1 génère le tick d'échantillonnage, par défaut 16000 Hz.
  OCR1A = (uint16_t)((F_CPU / (uint32_t)sampleRate) - 1U);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  TIMSK1 |= _BV(OCIE1A);
  interrupts();
}

void ProMicroEngineSoundClass::stopSampleTimer1()
{
  TIMSK1 &= ~_BV(OCIE1A);
}
