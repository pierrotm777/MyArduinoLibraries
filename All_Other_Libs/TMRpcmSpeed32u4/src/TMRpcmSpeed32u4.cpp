#include "TMRpcmSpeed32u4.h"
#include <string.h>
#include <avr/interrupt.h>

#if !defined(__AVR_ATmega32U4__)
  #error "TMRpcmSpeed32u4 is intended for ATmega32u4 boards (Pro Micro / Micro / Leonardo)."
#endif

// -------- Static members --------
TMRpcmSpeed32u4* TMRpcmSpeed32u4::_active = nullptr;
volatile uint8_t  TMRpcmSpeed32u4::_buf[TMRpcmSpeed32u4::BUF_SZ];
volatile uint16_t TMRpcmSpeed32u4::_rd = 0;
volatile uint16_t TMRpcmSpeed32u4::_wr = 0;
volatile uint16_t TMRpcmSpeed32u4::_count = 0;

// Optional ISR counter (useful for debugging)
volatile uint32_t g_isrCount = 0;

#if (TMRPCM_32U4_AUDIO_TIMER == 1)
  #define TMRPCM_TIFR   TIFR1
  #define TMRPCM_TIMSK  TIMSK1
  #define TMRPCM_OCIEA  OCIE1A
  #define TMRPCM_OCFA   OCF1A
  #define TMRPCM_OCR_A  OCR1A
  #define TMRPCM_TCNT   TCNT1
  #define TMRPCM_TCCRA  TCCR1A
  #define TMRPCM_TCCRB  TCCR1B
  #define TMRPCM_WGM_CTC WGM12
  #define TMRPCM_CS_N1  CS10
  ISR(TIMER1_COMPA_vect) { g_isrCount++; TMRpcmSpeed32u4::_isrService(); }
#elif (TMRPCM_32U4_AUDIO_TIMER == 3)
  #define TMRPCM_TIFR   TIFR3
  #define TMRPCM_TIMSK  TIMSK3
  #define TMRPCM_OCIEA  OCIE3A
  #define TMRPCM_OCFA   OCF3A
  #define TMRPCM_OCR_A  OCR3A
  #define TMRPCM_TCNT   TCNT3
  #define TMRPCM_TCCRA  TCCR3A
  #define TMRPCM_TCCRB  TCCR3B
  #define TMRPCM_WGM_CTC WGM32
  #define TMRPCM_CS_N1  CS30
  ISR(TIMER3_COMPA_vect) { g_isrCount++; TMRpcmSpeed32u4::_isrService(); }
#else
  #error "TMRPCM_32U4_AUDIO_TIMER must be 1 or 3"
#endif

TMRpcmSpeed32u4::TMRpcmSpeed32u4() {}

void TMRpcmSpeed32u4::_setError(const char* msg) {
  _lastError = msg ? msg : "ERR";
}

bool TMRpcmSpeed32u4::begin() {
  _playing = false;
  _paused  = false;
  _active  = nullptr;
  _setError("OK");

  noInterrupts();
  _rd = _wr = _count = 0;
  interrupts();

  _setupPwmTimer4();
  OCR4D = 127;
  return true;
}

void TMRpcmSpeed32u4::setVolume(uint8_t v) {
  if (v > 8) v = 8;
  // v=8 => loudest => shift 0; v=0 => quiet => shift 4 approx.
  _volumeShift = (uint8_t)((8 - v) / 2);
}

bool TMRpcmSpeed32u4::play(const char* filename) {
  stop();
  _paused = false;
  _setError("OK");

  if (filename) {
    strncpy(_currentName, filename, sizeof(_currentName) - 1);
    _currentName[sizeof(_currentName) - 1] = '\0';
  } else {
    _currentName[0] = '\0';
  }

  if (!_openAndParseWav(filename)) {
    return false;
  }

  noInterrupts();
  _rd = _wr = _count = 0;
  interrupts();

  _active = this;
  _playing = true;

  // Prime buffer generously. This reduces the very first click/underrun.
  for (uint8_t i = 0; i < 8; i++) update();

  _forceAudioTimerCtc(_wavSampleRate);
  _applyRateToAudioTimer();

  noInterrupts();
  TMRPCM_TIFR  = (1 << TMRPCM_OCFA);
  TMRPCM_TIMSK |= (1 << TMRPCM_OCIEA);
  interrupts();

  return true;
}

void TMRpcmSpeed32u4::stop() {
  noInterrupts();
  TMRPCM_TIMSK &= ~(1 << TMRPCM_OCIEA);
  interrupts();

  _playing = false;
  _paused  = false;
  _active  = nullptr;

  if (_file) _file.close();

  OCR4D = 127;
}

void TMRpcmSpeed32u4::pause(bool muteOutput) {
  if (!_playing || _paused) return;

  noInterrupts();
  _paused = true;
  TMRPCM_TIMSK &= ~(1 << TMRPCM_OCIEA);
  if (muteOutput) OCR4D = 127;
  interrupts();
}

void TMRpcmSpeed32u4::resume() {
  if (!_playing || !_paused) return;

  noInterrupts();
  _paused = false;
  TMRPCM_TIFR  = (1 << TMRPCM_OCFA);
  TMRPCM_TIMSK |= (1 << TMRPCM_OCIEA);
  interrupts();
}

void TMRpcmSpeed32u4::update() {
  if (!_playing || _paused || !_file) return;

  // Keep file cursor synchronized with WAV data position.
  uint32_t want = _dataStart + _dataPos;
  if ((uint32_t)_file.position() != want) {
    _file.seek(want);
  }

  // Fill a limited number of converted output samples per call.
  // This keeps update() non-blocking enough for RC/EKMFA/DFPlayer logic.
  uint8_t convertedThisCall = 0;
  while (_count < (BUF_SZ - 8) && _dataPos < _dataSize && convertedThisCall < 64) {
    uint8_t s = 127;
    if (!_readAndConvertOneSample(s)) break;

    noInterrupts();
    if (_count < BUF_SZ) {
      _buf[_wr] = s;
      _wr++;
      if (_wr >= BUF_SZ) _wr = 0;
      _count++;
    }
    interrupts();

    convertedThisCall++;
  }

  // End reached and buffer empty => loop or stop.
  if (_dataPos >= _dataSize && _count == 0) {
    if (loopPlayback && _currentName[0] != '\0') {
      // Avoid recursive stop/play from inside interrupt context; update() is loop context.
      char name[sizeof(_currentName)];
      strncpy(name, _currentName, sizeof(name));
      name[sizeof(name) - 1] = '\0';
      stop();
      play(name);
    } else {
      stop();
    }
  }
}

void TMRpcmSpeed32u4::_isrService() {
  TMRpcmSpeed32u4* self = _active;
  if (!self || !self->_playing || self->_paused) {
    OCR4D = 127;
    return;
  }

  uint8_t s = 127;

  if (_count) {
    s = _buf[_rd];
    _rd++;
    if (_rd >= BUF_SZ) _rd = 0;
    _count--;
  } else {
    // Underrun: silence. If this happens often, call audio.update() more often
    // or use a lower sample rate WAV.
    s = 127;
  }

  if (self->_volumeShift) {
    int16_t centered = (int16_t)s - 128;
    centered >>= self->_volumeShift;
    s = (uint8_t)(centered + 128);
  }

  OCR4D = s;
}

// ---------- WAV parsing helpers ----------
static uint32_t readLE32(File &f) {
  uint8_t b[4];
  if (f.read(b,4) != 4) return 0;
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static uint16_t readLE16(File &f) {
  uint8_t b[2];
  if (f.read(b,2) != 2) return 0;
  return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

bool TMRpcmSpeed32u4::_openAndParseWav(const char* filename) {
  if (!filename || !filename[0]) {
    _setError("empty filename");
    return false;
  }

  _file = SD.open(filename, FILE_READ);
  if (!_file) {
    _setError("SD.open failed");
    return false;
  }

  char riff[4];
  char wave[4];
  if (_file.read((uint8_t*)riff, 4) != 4) { _file.close(); _setError("short RIFF"); return false; }
  (void)readLE32(_file);
  if (_file.read((uint8_t*)wave, 4) != 4) { _file.close(); _setError("short WAVE"); return false; }

  if (memcmp(riff, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) {
    _file.close();
    _setError("not RIFF/WAVE");
    return false;
  }

  bool gotFmt = false, gotData = false;
  uint32_t sr = 0;
  uint32_t dataStart = 0, dataSize = 0;
  uint16_t fmtSize = 0;

  _audioFormat = 0;
  _numChannels = 0;
  _bitsPerSample = 0;
  _blockAlign = 0;

  while (_file.available()) {
    char id[4];
    if (_file.read((uint8_t*)id, 4) != 4) break;
    uint32_t sz32 = readLE32(_file);
    if (sz32 > 0x7FFF0000UL) { _file.close(); _setError("bad chunk size"); return false; }

    if (memcmp(id, "fmt ", 4) == 0) {
      fmtSize = (uint16_t)sz32;
      _audioFormat   = readLE16(_file);
      _numChannels   = (uint8_t)readLE16(_file);
      sr             = readLE32(_file);
      (void)readLE32(_file);       // byteRate
      _blockAlign    = readLE16(_file);
      _bitsPerSample = (uint8_t)readLE16(_file);

      if (fmtSize > 16) _skip((uint32_t)fmtSize - 16);
      gotFmt = true;
    }
    else if (memcmp(id, "data", 4) == 0) {
      dataStart = (uint32_t)_file.position();
      dataSize  = sz32;
      gotData = true;
      break;
    }
    else {
      _skip(sz32);
    }

    // Chunks are word-aligned in RIFF. Skip pad byte if needed.
    if (sz32 & 1) _skip(1);
  }

  if (!gotFmt) { _file.close(); _setError("missing fmt chunk"); return false; }
  if (!gotData) { _file.close(); _setError("missing data chunk"); return false; }
  if (_audioFormat != 1) { _file.close(); _setError("not PCM"); return false; }
  if (_numChannels < 1 || _numChannels > 2) { _file.close(); _setError("channels must be 1 or 2"); return false; }
  if (_bitsPerSample != 8 && _bitsPerSample != 16) { _file.close(); _setError("bits must be 8 or 16"); return false; }

  uint16_t expectedAlign = (uint16_t)_numChannels * (uint16_t)(_bitsPerSample / 8);
  if (_blockAlign == 0) _blockAlign = expectedAlign;
  if (_blockAlign < expectedAlign || _blockAlign > 8) { _file.close(); _setError("unsupported blockAlign"); return false; }

  if (sr < 4000) sr = 4000;
  // The library can play higher rates, but 32u4 + SD + pitch variation is much
  // more stable with 8/11/16/22 kHz WAV. Clamp only the timer later.
  _wavSampleRate = sr;
  _dataStart = dataStart;
  _dataSize  = dataSize;
  _dataPos   = 0;

  _file.seek(_dataStart);
  _setError("OK");
  return true;
}

bool TMRpcmSpeed32u4::_readAndConvertOneSample(uint8_t &out) {
  if (_dataPos >= _dataSize) return false;

  uint8_t raw[8];
  uint16_t need = _blockAlign;
  uint32_t rem = _dataSize - _dataPos;
  if (need > rem) need = (uint16_t)rem;
  if (need == 0) return false;

  int r = _file.read(raw, need);
  if (r <= 0) return false;
  _dataPos += (uint32_t)r;

  // Incomplete final frame: return silence rather than noisy bytes.
  uint16_t bytesPerSample = (uint16_t)(_bitsPerSample / 8);
  uint16_t minFrame = (uint16_t)_numChannels * bytesPerSample;
  if ((uint16_t)r < minFrame) {
    out = 127;
    return true;
  }

  if (_bitsPerSample == 8) {
    // WAV 8-bit PCM is already unsigned.
    if (_numChannels == 1) {
      out = raw[0];
    } else {
      uint16_t mix = (uint16_t)raw[0] + (uint16_t)raw[1];
      out = (uint8_t)(mix >> 1);
    }
    return true;
  }

  // WAV 16-bit PCM is signed little-endian. Convert to unsigned 8-bit PWM.
  int16_t s0 = (int16_t)((uint16_t)raw[0] | ((uint16_t)raw[1] << 8));
  int32_t mix = s0;

  if (_numChannels == 2) {
    int16_t s1 = (int16_t)((uint16_t)raw[2] | ((uint16_t)raw[3] << 8));
    mix = ((int32_t)s0 + (int32_t)s1) / 2;
  }

  int16_t signed8 = (int16_t)(mix >> 8);      // -128..127 approx.
  int16_t u = signed8 + 128;                  // 0..255
  if (u < 0) u = 0;
  if (u > 255) u = 255;
  out = (uint8_t)u;
  return true;
}

void TMRpcmSpeed32u4::_skip(uint32_t n) {
  if (!n) return;
  uint32_t pos = (uint32_t)_file.position();
  _file.seek(pos + n);
}

// ---------- Timers ----------
void TMRpcmSpeed32u4::_setupPwmTimer4() {
  pinMode(6, OUTPUT);

  // Preserve USB PLL configuration where possible.
  uint8_t pll = PLLCSR;
  if (!(pll & (1 << PLLE))) {
    PLLCSR = pll | (1 << PLLE);
    while (!(PLLCSR & (1 << PLOCK))) { }
  }

  // Enable Timer4 clock from PLL.
  PLLCSR |= (1 << PLLTM0);

  TCCR4A = 0;
  TCCR4B = 0;
  TCCR4C = 0;
  TCCR4D = 0;
  TCCR4E = 0;
  TCNT4  = 0;

  // Fast PWM on OC4D (Arduino D6 on ATmega32u4).
  TCCR4C |= (1 << COM4D1);
  TCCR4C |= (1 << PWM4D);

  // Prescaler /1 for high-frequency PWM carrier.
  TCCR4B |= (1 << CS40);

  OCR4D = 127;
}

void TMRpcmSpeed32u4::_forceAudioTimerCtc(uint32_t sampleRate) {
  if (sampleRate < 4000)  sampleRate = 4000;
  // Practical maximum for this simple SD streaming player on 32u4.
  if (sampleRate > 32000) sampleRate = 32000;

  uint32_t ocr = (F_CPU / sampleRate) - 1;
  if (ocr > 65535) ocr = 65535;
  if (ocr < 50)    ocr = 50;

  _baseOcrA = (uint16_t)ocr;

  noInterrupts();
  TMRPCM_TCCRA = 0;
  TMRPCM_TCCRB = 0;
  TMRPCM_TCNT  = 0;
  TMRPCM_OCR_A  = (uint16_t)ocr;
  TMRPCM_TCCRB = (1 << TMRPCM_WGM_CTC) | (1 << TMRPCM_CS_N1); // CTC, /1
  interrupts();
}

void TMRpcmSpeed32u4::setPlaybackRate(float rate) {
  if (rate < 0.3f) rate = 0.3f;
  if (rate > 3.0f) rate = 3.0f;
  _playbackRate = rate;
  _applyRateToAudioTimer();
}

void TMRpcmSpeed32u4::setSpeedFromPulseUs(uint16_t us,
                                         uint16_t inMinUs, uint16_t inMaxUs,
                                         float rateMin, float rateMax)
{
  if (inMinUs >= inMaxUs) return;
  if (us < inMinUs) us = inMinUs;
  if (us > inMaxUs) us = inMaxUs;

  float t = (float)(us - inMinUs) / (float)(inMaxUs - inMinUs);
  float r = rateMin + t * (rateMax - rateMin);
  setPlaybackRate(r);
}

void TMRpcmSpeed32u4::_applyRateToAudioTimer() {
  if (!_baseOcrA) return;

  float rate = _playbackRate;
  if (rate < 0.3f) rate = 0.3f;
  if (rate > 3.0f) rate = 3.0f;

  uint32_t basePlus1 = (uint32_t)_baseOcrA + 1u;
  uint32_t ocr = (uint32_t)((float)basePlus1 / rate);
  if (ocr == 0) ocr = 1;
  ocr -= 1;

  if (ocr > 65535u) ocr = 65535u;
  if (ocr < 50u)    ocr = 50u;

  noInterrupts();
  TMRPCM_OCR_A = (uint16_t)ocr;
  interrupts();
}
