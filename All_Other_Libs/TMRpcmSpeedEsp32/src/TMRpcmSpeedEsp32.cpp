#include "TMRpcmSpeedEsp32.h"

TMRpcmSpeedEsp32::TMRpcmSpeedEsp32() {}

void TMRpcmSpeedEsp32::useI2S() {}

void TMRpcmSpeedEsp32::setI2SPins(int bclk, int ws, int dout) {
  _bclk = bclk;
  _ws = ws;
  _dout = dout;
}

void TMRpcmSpeedEsp32::setFormat(TMRpcmEsp32Format fmt) {
  _format = fmt;
}

void TMRpcmSpeedEsp32::setVolume(uint16_t percent) {
  if (percent > 300) percent = 300;
  _volume = percent;
}

uint16_t TMRpcmSpeedEsp32::getVolume() const {
  return _volume;
}

void TMRpcmSpeedEsp32::setCompressor(bool enabled) {
  _compressorEnabled = enabled;
}

void TMRpcmSpeedEsp32::setCompressor(uint16_t threshold, float ratio, uint16_t makeupGainPercent) {
  if (threshold < 1000) threshold = 1000;
  if (threshold > 32767) threshold = 32767;
  if (ratio < 1.0f) ratio = 1.0f;
  if (ratio > 20.0f) ratio = 20.0f;
  if (makeupGainPercent > 300) makeupGainPercent = 300;

  _compressorThreshold = threshold;
  _compressorRatio = ratio;
  _compressorMakeup = makeupGainPercent;
  _compressorEnabled = true;
}

bool TMRpcmSpeedEsp32::getCompressorEnabled() const {
  return _compressorEnabled;
}

void TMRpcmSpeedEsp32::setAutoMixNormalize(bool enabled) {
  _autoMixNormalize = enabled;
}

bool TMRpcmSpeedEsp32::getAutoMixNormalize() const {
  return _autoMixNormalize;
}



void TMRpcmSpeedEsp32::setPlaybackRate(float rate) {
  setMotorRate(rate);
}

float TMRpcmSpeedEsp32::getPlaybackRate() const {
  return getMotorRate();
}

void TMRpcmSpeedEsp32::setMotorRate(float rate) {
  if (rate < 0.25f) rate = 0.25f;
  if (rate > 4.00f) rate = 4.00f;

  if (fabsf(rate - _rate) < 0.001f) return;

  _rate = rate;
  _motor.rate = rate;
  updateMotorStep();

  // v3.7 IMPORTANT:
  // On ne change plus la fréquence I2S globale.
  // Les AUX restent donc à leur hauteur/vitesse normale.
}

float TMRpcmSpeedEsp32::getMotorRate() const {
  return _rate;
}

void TMRpcmSpeedEsp32::updateMotorStep() {
  _motor.stepQ16 = (uint32_t)(_rate * (float)RATE_ONE);
  if (_motor.stepQ16 < 1) _motor.stepQ16 = 1;
}

void TMRpcmSpeedEsp32::setMotorVolume(uint16_t percent) {
  if (percent > 300) percent = 300;
  _motor.volume = percent;
}

void TMRpcmSpeedEsp32::setMotorInterpolation(bool enabled) {
  _motorInterpolation = enabled;
}

bool TMRpcmSpeedEsp32::getMotorInterpolation() const {
  return _motorInterpolation;
}


void TMRpcmSpeedEsp32::setAuxVolume(uint8_t slot, uint16_t percent) {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) return;
  if (percent > 300) percent = 300;
  _aux[slot].volume = percent;
}

bool TMRpcmSpeedEsp32::begin() {
  _begun = true;
  setError("OK");
  return true;
}

bool TMRpcmSpeedEsp32::play(const char *filename) {
  return playMotor(filename);
}

bool TMRpcmSpeedEsp32::playMotor(const char *filename) {
  if (!_begun) {
    setError("begin() not called");
    return false;
  }

  closeStream(_motor);

  if (!openStream(_motor, filename, loopPlayback, _motor.volume)) return false;

  _motor.rate = _rate;
  _motor.phaseQ16 = 0;
  _motor.hasCachedFrame = false;
  updateMotorStep();

  if (!_i2sStarted) {
    if (!initI2S(_motor.wav.sampleRate)) {
      closeStream(_motor);
      return false;
    }
    if (!startAudioTask()) {
      closeStream(_motor);
      return false;
    }
  } else {
    startAudioTask();
  }

  return true;
}

bool TMRpcmSpeedEsp32::preloadAux(uint8_t slot, const char *filename) {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) {
    setError("bad aux slot");
    return false;
  }

  if (!_begun) {
    setError("begin() not called");
    return false;
  }

  Stream &s = _aux[slot];
  uint16_t vol = s.volume;

  // Stoppe la lecture en cours et libère seulement l'ancien buffer RAM,
  // mais prépare ensuite le fallback SD avec le nouveau nom.
  stopStream(s);

  if (s.ramOwned && s.ramData) {
    free(s.ramData);
  }

  s.ramData = nullptr;
  s.ramSize = 0;
  s.ramPos = 0;
  s.fromRam = false;
  s.ramOwned = false;
  s.fallbackToSD = false;

  strncpy(s.filename, filename, sizeof(s.filename) - 1);
  s.filename[sizeof(s.filename) - 1] = 0;
  s.hasFilename = true;

  WavInfo w;
  if (!getWavInfoFromFile(filename, w)) return false;

  s.wav = w;
  s.volume = vol;

  // Trop gros pour le seuil auto : preload volontairement ignoré,
  // mais playAux(slot) basculera automatiquement en SD.
  if (_autoPreloadMaxBytes > 0 && w.dataSize > _autoPreloadMaxBytes) {
    s.fallbackToSD = true;
    setError("preload skipped: SD fallback");
    return true;
  }

  // Essai RAM/PSRAM.
  if (!loadStreamToRam(s, filename, vol)) {
    // malloc ou lecture échouée : pas bloquant, fallback SD.
    s.wav = w;
    s.volume = vol;
    s.fallbackToSD = true;
    s.hasFilename = true;
    strncpy(s.filename, filename, sizeof(s.filename) - 1);
    s.filename[sizeof(s.filename) - 1] = 0;
    setError("preload failed: SD fallback");
    return true;
  }

  s.fallbackToSD = false;
  setError("OK");
  return true;
}

void TMRpcmSpeedEsp32::clearPreloadedAux(uint8_t slot) {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) return;

  stopStream(_aux[slot]);

  if (_aux[slot].ramOwned && _aux[slot].ramData) {
    free(_aux[slot].ramData);
  }

  _aux[slot].ramData = nullptr;
  _aux[slot].ramSize = 0;
  _aux[slot].ramPos = 0;
  _aux[slot].fromRam = false;
  _aux[slot].ramOwned = false;
  _aux[slot].fallbackToSD = false;
  _aux[slot].filename[0] = 0;
  _aux[slot].hasFilename = false;
}

bool TMRpcmSpeedEsp32::playAux(uint8_t slot) {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) {
    setError("bad aux slot");
    return false;
  }

  Stream &s = _aux[slot];

  // Si préchargé en RAM/PSRAM : lecture RAM.
  if (s.ramData && s.ramSize > 0) {
    s.ramPos = 0;
    s.remaining = s.ramSize;
    s.playing = true;
    s.loop = false;
    s.fromRam = true;
    s.bufPos = 0;
    s.bufLen = 0;

    if (!_i2sStarted) {
      if (!initI2S(s.wav.sampleRate)) {
        stopStream(s);
        return false;
      }
      if (!startAudioTask()) {
        stopStream(s);
        return false;
      }
    } else {
      startAudioTask();
    }

    setError("OK");
    return true;
  }

  // Fallback SD automatique si preload trop gros ou malloc failed.
  if ((s.fallbackToSD || s.hasFilename) && s.filename[0]) {
    return playAuxFromSD(slot, s.filename);
  }

  setError("aux not preloaded");
  return false;
}

bool TMRpcmSpeedEsp32::playAux(uint8_t slot, const char *filename) {
  // v3.16 : mode automatique RAM/SD.
  // Petit fichier -> RAM/PSRAM pour éviter les parasites au déclenchement.
  // Gros fichier -> lecture SD directe pour les musiques longues.
  if (slot >= TMRPCM_MAX_AUX_SLOTS) {
    setError("bad aux slot");
    return false;
  }

  WavInfo w;
  if (!getWavInfoFromFile(filename, w)) return false;

  if (_autoPreloadMaxBytes > 0 && w.dataSize <= _autoPreloadMaxBytes) {
    if (!preloadAux(slot, filename)) return false;
    return playAux(slot);
  }

  return playAuxFromSD(slot, filename);
}

bool TMRpcmSpeedEsp32::playAuxFromSD(uint8_t slot, const char *filename) {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) {
    setError("bad aux slot");
    return false;
  }

  if (!_begun) {
    setError("begin() not called");
    return false;
  }

  uint16_t vol = _aux[slot].volume;

  // Arrête le slot sans libérer une éventuelle précharge RAM.
  stopStream(_aux[slot]);
  _aux[slot].fromRam = false;

  if (!openStream(_aux[slot], filename, false, vol)) return false;

  if (!_i2sStarted) {
    if (!initI2S(_aux[slot].wav.sampleRate)) {
      stopStream(_aux[slot]);
      return false;
    }
    if (!startAudioTask()) {
      stopStream(_aux[slot]);
      return false;
    }
  } else {
    startAudioTask();
  }

  return true;
}

void TMRpcmSpeedEsp32::stopAux(uint8_t slot) {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) return;
  closeStream(_aux[slot]);
}

bool TMRpcmSpeedEsp32::isAuxPlaying(uint8_t slot) const {
  if (slot >= TMRPCM_MAX_AUX_SLOTS) return false;
  return _aux[slot].playing;
}

void TMRpcmSpeedEsp32::update() {
  if (!_i2sStarted) return;
  if (!_motor.playing && !_aux[0].playing && !_aux[1].playing && !_aux[2].playing) return;

  int16_t temp[XT_SLOT_FRAMES_MAX * 2];

  // Producteur de blocs audio pour la tâche I2S.
  for (int n = 0; n < 3; n++) {
    if (_ringCount >= XT_RING_SLOTS - 1) break;

    fillMixFrames(temp, _xtChunkFrames);

    if (!ringPush(temp, _xtChunkFrames)) break;
  }
}


void TMRpcmSpeedEsp32::setAudioTaskCore(int core) {
  _audioTaskCore = core;
}

void TMRpcmSpeedEsp32::setAudioTaskPriority(uint8_t prio) {
  if (prio < 1) prio = 1;
  if (prio > 24) prio = 24;
  _audioTaskPriority = prio;
}

void TMRpcmSpeedEsp32::setXTChunkFrames(uint16_t frames) {
  if (frames < 32) frames = 32;
  if (frames > XT_SLOT_FRAMES_MAX) frames = XT_SLOT_FRAMES_MAX;
  _xtChunkFrames = frames;
}

void TMRpcmSpeedEsp32::fillMixFrames(int16_t *dst, size_t frames) {
  for (size_t i = 0; i < frames; i++) {
    int32_t mixL = 0;
    int32_t mixR = 0;
    int16_t l = 0, r = 0;
    uint8_t contributors = 0;

    if (readMotorFrame(l, r)) {
      mixL += applyGainClip(l, _motor.volume);
      mixR += applyGainClip(r, _motor.volume);
      contributors++;
    }

    for (uint8_t s = 0; s < TMRPCM_MAX_AUX_SLOTS; s++) {
      if (readRawStereoFrame(_aux[s], l, r)) {
        mixL += applyGainClip(l, _aux[s].volume);
        mixR += applyGainClip(r, _aux[s].volume);
        contributors++;
      }
    }

    if (_autoMixNormalize && contributors > 1) {
      mixL /= contributors;
      mixR /= contributors;
    }

    mixL = (mixL * (int32_t)_volume) / 100;
    mixR = (mixR * (int32_t)_volume) / 100;

    mixL = compressSample(mixL);
    mixR = compressSample(mixR);

    dst[i * 2 + 0] = finalClip(mixL);
    dst[i * 2 + 1] = finalClip(mixR);
  }
}

bool TMRpcmSpeedEsp32::ringPush(const int16_t *samples, size_t frames) {
  if (!_ringMutex) return false;
  if (frames > XT_SLOT_FRAMES_MAX) frames = XT_SLOT_FRAMES_MAX;

  if (xSemaphoreTake(_ringMutex, pdMS_TO_TICKS(1)) != pdTRUE) return false;

  if (_ringCount >= XT_RING_SLOTS) {
    xSemaphoreGive(_ringMutex);
    return false;
  }

  memcpy(_ring[_ringWrite], samples, frames * 2 * sizeof(int16_t));
  _ringWrite = (_ringWrite + 1) % XT_RING_SLOTS;
  _ringCount++;

  xSemaphoreGive(_ringMutex);
  return true;
}

bool TMRpcmSpeedEsp32::ringPop(int16_t *samples, size_t frames) {
  if (!_ringMutex) return false;
  if (frames > XT_SLOT_FRAMES_MAX) frames = XT_SLOT_FRAMES_MAX;

  if (xSemaphoreTake(_ringMutex, pdMS_TO_TICKS(1)) != pdTRUE) {
    memset(samples, 0, frames * 2 * sizeof(int16_t));
    return false;
  }

  if (_ringCount == 0) {
    xSemaphoreGive(_ringMutex);
    memset(samples, 0, frames * 2 * sizeof(int16_t));
    return false;
  }

  memcpy(samples, _ring[_ringRead], frames * 2 * sizeof(int16_t));
  _ringRead = (_ringRead + 1) % XT_RING_SLOTS;
  _ringCount--;

  xSemaphoreGive(_ringMutex);
  return true;
}

void TMRpcmSpeedEsp32::audioTaskThunk(void *arg) {
  ((TMRpcmSpeedEsp32*)arg)->audioTask();
}

void TMRpcmSpeedEsp32::audioTask() {
  int16_t local[XT_SLOT_FRAMES_MAX * 2];

  while (!_taskStop) {
    bool ok = ringPop(local, _xtChunkFrames);
    size_t written = 0;
    i2s_write(I2S_NUM_0, local, _xtChunkFrames * 2 * sizeof(int16_t), &written, portMAX_DELAY);

    if (!ok) {
      vTaskDelay(1);
    }
  }

  memset(local, 0, sizeof(local));
  for (int i = 0; i < 4; i++) {
    size_t written = 0;
    i2s_write(I2S_NUM_0, local, _xtChunkFrames * 2 * sizeof(int16_t), &written, pdMS_TO_TICKS(20));
  }

  _taskRunning = false;
  _audioTaskHandle = nullptr;
  vTaskDelete(NULL);
}

bool TMRpcmSpeedEsp32::startAudioTask() {
  if (_audioTaskHandle) return true;

  if (!_ringMutex) {
    _ringMutex = xSemaphoreCreateMutex();
    if (!_ringMutex) {
      setError("ring mutex failed");
      return false;
    }
  }

  _ringRead = 0;
  _ringWrite = 0;
  _ringCount = 0;
  _taskStop = false;
  _taskRunning = true;

  BaseType_t ok = xTaskCreatePinnedToCore(
    audioTaskThunk,
    "tmrpcm_i2s",
    4096,
    this,
    _audioTaskPriority,
    &_audioTaskHandle,
    _audioTaskCore
  );

  if (ok != pdPASS) {
    _audioTaskHandle = nullptr;
    _taskRunning = false;
    setError("audio task failed");
    return false;
  }

  return true;
}

void TMRpcmSpeedEsp32::stopAudioTask() {
  if (!_audioTaskHandle) return;

  _taskStop = true;

  uint32_t t0 = millis();
  while (_taskRunning && (millis() - t0) < 500) {
    delay(5);
  }

  _audioTaskHandle = nullptr;
  _taskRunning = false;
}

void TMRpcmSpeedEsp32::stop() {
  stopAudioTask();

  closeStream(_motor);
  for (uint8_t i = 0; i < TMRPCM_MAX_AUX_SLOTS; i++) closeStream(_aux[i]);

  if (_i2sStarted) {
    memset(_mixBuf, 0, sizeof(_mixBuf));
    for (int i = 0; i < 4; i++) {
      size_t written = 0;
      i2s_write(I2S_NUM_0, _mixBuf, sizeof(_mixBuf), &written, 20 / portTICK_PERIOD_MS);
    }
    i2s_driver_uninstall(I2S_NUM_0);
    _i2sStarted = false;
  }
}

bool TMRpcmSpeedEsp32::isPlaying() const {
  if (_motor.playing) return true;
  for (uint8_t i = 0; i < TMRPCM_MAX_AUX_SLOTS; i++) if (_aux[i].playing) return true;
  return false;
}

const char* TMRpcmSpeedEsp32::getLastError() const { return _lastError; }
const char* TMRpcmSpeedEsp32::version() const { return TMRPCM_SPEED_ESP32_VERSION; }
uint32_t TMRpcmSpeedEsp32::getWavSampleRate() const { return _motor.wav.sampleRate; }
uint16_t TMRpcmSpeedEsp32::getWavBitsPerSample() const { return _motor.wav.bitsPerSample; }
uint16_t TMRpcmSpeedEsp32::getWavChannels() const { return _motor.wav.channels; }
void TMRpcmSpeedEsp32::setError(const char *err) { _lastError = err; }

bool TMRpcmSpeedEsp32::openStream(Stream &s, const char *filename, bool loop, uint16_t vol) {
  s.file = SD.open(filename, FILE_READ);
  if (!s.file) {
    setError("open failed");
    return false;
  }

  if (!parseWav(s.file, s.wav)) {
    s.file.close();
    return false;
  }

  if (!formatAccepted(s.wav)) {
    s.file.close();
    setError("WAV format mismatch");
    return false;
  }

  if (_i2sStarted && s.wav.sampleRate != _i2sSampleRate) {
    s.file.close();
    setError("sample rate mismatch");
    return false;
  }

  s.file.seek(s.wav.dataOffset);
  s.remaining = s.wav.dataSize;
  s.playing = true;
  s.loop = loop;
  s.volume = vol;
  s.bufPos = 0;
  s.bufLen = 0;
  s.fromRam = false;
  s.ramPos = 0;
  s.rate = 1.0f;
  s.stepQ16 = RATE_ONE;
  s.phaseQ16 = 0;
  s.hasCachedFrame = false;
  s.hasNextFrame = false;
  s.cachedL = 0;
  s.cachedR = 0;
  s.hasNextFrame = false;
  s.nextL = 0;
  s.nextR = 0;

  refillStream(s);

  setError("OK");
  return true;
}

bool TMRpcmSpeedEsp32::getWavInfoFromFile(const char *filename, WavInfo &w) {
  File f = SD.open(filename, FILE_READ);
  if (!f) {
    setError("open failed");
    return false;
  }

  bool ok = parseWav(f, w);
  f.close();

  if (!ok) return false;

  if (!formatAccepted(w)) {
    setError("WAV format mismatch");
    return false;
  }

  return true;
}

bool TMRpcmSpeedEsp32::loadStreamToRam(Stream &s, const char *filename, uint16_t vol) {
  File f = SD.open(filename, FILE_READ);
  if (!f) {
    setError("open failed");
    return false;
  }

  WavInfo w;
  if (!parseWav(f, w)) {
    f.close();
    return false;
  }

  if (!formatAccepted(w)) {
    f.close();
    setError("WAV format mismatch");
    return false;
  }

  if (_i2sStarted && w.sampleRate != _i2sSampleRate) {
    f.close();
    setError("sample rate mismatch");
    return false;
  }

  uint8_t *mem = nullptr;
#if defined(BOARD_HAS_PSRAM)
  mem = (uint8_t*)ps_malloc(w.dataSize);
#endif
  if (!mem) mem = (uint8_t*)malloc(w.dataSize);
  if (!mem) {
    f.close();
    setError("malloc failed");
    return false;
  }

  f.seek(w.dataOffset);

  uint32_t done = 0;
  while (done < w.dataSize) {
    size_t chunk = w.dataSize - done;
    if (chunk > 1024) chunk = 1024;

    int n = f.read(mem + done, chunk);
    if (n <= 0) {
      free(mem);
      f.close();
      setError("ram load failed");
      return false;
    }
    done += (uint32_t)n;
  }

  f.close();

  s.wav = w;
  s.remaining = 0;
  s.playing = false;
  s.loop = false;
  s.volume = vol;
  s.bufPos = 0;
  s.bufLen = 0;
  s.ramData = mem;
  s.ramSize = w.dataSize;
  s.ramPos = 0;
  s.fromRam = true;
  s.ramOwned = true;
  s.fallbackToSD = false;
  s.phaseQ16 = 0;
  s.hasCachedFrame = false;
  s.hasNextFrame = false;

  setError("OK");
  return true;
}

void TMRpcmSpeedEsp32::stopStream(Stream &s) {
  if (s.file) s.file.close();
  s.remaining = 0;
  s.playing = false;
  s.bufPos = 0;
  s.bufLen = 0;
  s.ramPos = 0;
  s.phaseQ16 = 0;
  s.hasCachedFrame = false;
  s.hasNextFrame = false;
}

void TMRpcmSpeedEsp32::closeStream(Stream &s) {
  stopStream(s);

  // Pour un stream RAM préchargé, closeStream() ne libère pas la RAM :
  // ça permet stopAux() puis playAux(slot) sans recharger la SD.
  // clearPreloadedAux() libère explicitement la RAM.
}

bool TMRpcmSpeedEsp32::refillStream(Stream &s) {
  if (!s.playing) return false;

  if (s.fromRam) {
    if (!s.ramData || s.ramSize == 0) {
      stopStream(s);
      return false;
    }

    if (s.ramPos >= s.ramSize) {
      if (s.loop) {
        s.ramPos = 0;
      } else {
        stopStream(s);
        return false;
      }
    }

    size_t want = STREAM_BUF_BYTES;
    size_t remain = s.ramSize - s.ramPos;
    if (want > remain) want = remain;

    if (s.wav.bitsPerSample == 16 && s.wav.channels == 2) want &= ~((size_t)3);
    else if (s.wav.bitsPerSample == 16 && s.wav.channels == 1) want &= ~((size_t)1);

    if (want == 0) {
      stopStream(s);
      return false;
    }

    memcpy(s.buf, s.ramData + s.ramPos, want);
    s.ramPos += want;
    s.bufPos = 0;
    s.bufLen = want;
    return true;
  }

  if (!s.file) return false;

  if (s.remaining == 0) {
    if (s.loop) {
      s.file.seek(s.wav.dataOffset);
      s.remaining = s.wav.dataSize;
    } else {
      closeStream(s);
      return false;
    }
  }

  size_t want = STREAM_BUF_BYTES;
  if (s.remaining < want) want = s.remaining;

  if (s.wav.bitsPerSample == 16 && s.wav.channels == 2) want &= ~((size_t)3);
  else if (s.wav.bitsPerSample == 16 && s.wav.channels == 1) want &= ~((size_t)1);

  if (want == 0) {
    s.remaining = 0;
    return false;
  }

  int n = s.file.read(s.buf, want);
  if (n <= 0) {
    closeStream(s);
    return false;
  }

  s.bufPos = 0;
  s.bufLen = (size_t)n;
  s.remaining -= s.bufLen;
  return true;
}

bool TMRpcmSpeedEsp32::readRawStereoFrame(Stream &s, int16_t &l, int16_t &r) {
  l = 0;
  r = 0;

  if (!s.playing) return false;

  size_t need = 4;
  if (s.wav.bitsPerSample == 16 && s.wav.channels == 1) need = 2;
  if (s.wav.bitsPerSample == 8 && s.wav.channels == 1) need = 1;

  if (s.bufPos + need > s.bufLen) {
    if (!refillStream(s)) return false;
  }

  if (s.bufPos + need > s.bufLen) return false;

  if (s.wav.bitsPerSample == 16 && s.wav.channels == 2) {
    uint8_t *b = &s.buf[s.bufPos];
    s.bufPos += 4;
    l = (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    r = (int16_t)((uint16_t)b[2] | ((uint16_t)b[3] << 8));
    return true;
  }

  if (s.wav.bitsPerSample == 16 && s.wav.channels == 1) {
    uint8_t *b = &s.buf[s.bufPos];
    s.bufPos += 2;
    l = r = (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    return true;
  }

  if (s.wav.bitsPerSample == 8 && s.wav.channels == 1) {
    uint8_t v = s.buf[s.bufPos++];
    l = r = (int16_t)(((int16_t)v - 128) << 8);
    return true;
  }

  closeStream(s);
  return false;
}

bool TMRpcmSpeedEsp32::ensureMotorFrames() {
  if (!_motor.playing) return false;

  if (!_motor.hasCachedFrame) {
    if (!readRawStereoFrame(_motor, _motor.cachedL, _motor.cachedR)) return false;
    _motor.hasCachedFrame = true;
    _motor.phaseQ16 = 0;
  }

  if (!_motor.hasNextFrame) {
    if (readRawStereoFrame(_motor, _motor.nextL, _motor.nextR)) {
      _motor.hasNextFrame = true;
    } else {
      _motor.nextL = _motor.cachedL;
      _motor.nextR = _motor.cachedR;
      _motor.hasNextFrame = true;
    }
  }

  return true;
}

bool TMRpcmSpeedEsp32::readMotorFrame(int16_t &l, int16_t &r) {
  l = 0;
  r = 0;

  if (!ensureMotorFrames()) return false;

  if (_motorInterpolation) {
    uint32_t frac = _motor.phaseQ16 & 0xFFFFUL;

    int32_t dl = (int32_t)_motor.nextL - (int32_t)_motor.cachedL;
    int32_t dr = (int32_t)_motor.nextR - (int32_t)_motor.cachedR;

    l = (int16_t)((int32_t)_motor.cachedL + ((dl * (int32_t)frac) >> 16));
    r = (int16_t)((int32_t)_motor.cachedR + ((dr * (int32_t)frac) >> 16));
  } else {
    l = _motor.cachedL;
    r = _motor.cachedR;
  }

  _motor.phaseQ16 += _motor.stepQ16;

  while (_motor.phaseQ16 >= RATE_ONE) {
    _motor.cachedL = _motor.nextL;
    _motor.cachedR = _motor.nextR;
    _motor.phaseQ16 -= RATE_ONE;

    if (readRawStereoFrame(_motor, _motor.nextL, _motor.nextR)) {
      _motor.hasNextFrame = true;
    } else {
      _motor.nextL = _motor.cachedL;
      _motor.nextR = _motor.cachedR;
      _motor.hasNextFrame = true;
      break;
    }
  }

  return true;
}

int16_t TMRpcmSpeedEsp32::applyGainClip(int32_t sample, uint16_t vol) const {
  sample = (sample * (int32_t)vol) / 100;
  if (sample > 32767) sample = 32767;
  if (sample < -32768) sample = -32768;
  return (int16_t)sample;
}

int32_t TMRpcmSpeedEsp32::compressSample(int32_t sample) const {
  if (!_compressorEnabled) {
    sample = (sample * (int32_t)_compressorMakeup) / 100;
    return sample;
  }

  int32_t sign = 1;
  int32_t x = sample;
  if (x < 0) {
    sign = -1;
    x = -x;
  }

  int32_t y = x;
  int32_t threshold = _compressorThreshold;

  if (x > threshold) {
    float over = (float)(x - threshold);
    y = threshold + (int32_t)(over / _compressorRatio);
  }

  y = (y * (int32_t)_compressorMakeup) / 100;

  if (y > 32767) y = 32767;
  return y * sign;
}

int16_t TMRpcmSpeedEsp32::finalClip(int32_t sample) const {
  if (sample > 32767) sample = 32767;
  if (sample < -32768) sample = -32768;
  return (int16_t)sample;
}

bool TMRpcmSpeedEsp32::initI2S(uint32_t sampleRate) {
  _i2sSampleRate = sampleRate;

  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = (int)sampleRate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = _bclk,
    .ws_io_num = _ws,
    .data_out_num = _dout,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  if (_i2sStarted) {
    i2s_driver_uninstall(I2S_NUM_0);
    _i2sStarted = false;
  }

  esp_err_t e = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  if (e != ESP_OK) {
    setError("i2s_driver_install failed");
    return false;
  }

  e = i2s_set_pin(I2S_NUM_0, &pins);
  if (e != ESP_OK) {
    setError("i2s_set_pin failed");
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  _i2sStarted = true;
  return true;
}

bool TMRpcmSpeedEsp32::formatAccepted(const WavInfo &w) const {
  if (w.audioFormat != 1) return false;

  if (_format == TMRPCM_FMT_AUTO) {
    if (w.bitsPerSample == 16 && w.channels == 2 && (w.sampleRate == 16000 || w.sampleRate == 44100)) return true;
    if (w.sampleRate == 16000 && w.bitsPerSample == 16 && w.channels == 1) return true;
    if (w.sampleRate == 16000 && w.bitsPerSample == 8 && w.channels == 1) return true;
    return false;
  }

  if (_format == TMRPCM_FMT_S16_STEREO_16000) return w.sampleRate == 16000 && w.bitsPerSample == 16 && w.channels == 2;
  if (_format == TMRPCM_FMT_S16_MONO_16000)   return w.sampleRate == 16000 && w.bitsPerSample == 16 && w.channels == 1;
  if (_format == TMRPCM_FMT_U8_MONO_16000)    return w.sampleRate == 16000 && w.bitsPerSample == 8  && w.channels == 1;
  if (_format == TMRPCM_FMT_S16_STEREO_44100) return w.sampleRate == 44100 && w.bitsPerSample == 16 && w.channels == 2;

  return false;
}

uint16_t TMRpcmSpeedEsp32::rd16(File &f) {
  uint8_t b0 = f.read();
  uint8_t b1 = f.read();
  return (uint16_t)b0 | ((uint16_t)b1 << 8);
}

uint32_t TMRpcmSpeedEsp32::rd32(File &f) {
  uint8_t b0 = f.read();
  uint8_t b1 = f.read();
  uint8_t b2 = f.read();
  uint8_t b3 = f.read();
  return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}

bool TMRpcmSpeedEsp32::readTag(File &f, char tag[5]) {
  if (f.read((uint8_t*)tag, 4) != 4) return false;
  tag[4] = 0;
  return true;
}

bool TMRpcmSpeedEsp32::parseWav(File &f, WavInfo &w) {
  char tag[5];
  w = WavInfo();

  f.seek(0);
  if (!readTag(f, tag) || strcmp(tag, "RIFF") != 0) {
    setError("not RIFF");
    return false;
  }

  (void)rd32(f);

  if (!readTag(f, tag) || strcmp(tag, "WAVE") != 0) {
    setError("not WAVE");
    return false;
  }

  bool gotFmt = false;
  bool gotData = false;

  while (f.available()) {
    if (!readTag(f, tag)) break;

    uint32_t chunkSize = rd32(f);
    uint32_t nextChunk = f.position() + chunkSize;

    if (strcmp(tag, "fmt ") == 0) {
      w.audioFormat = rd16(f);
      w.channels = rd16(f);
      w.sampleRate = rd32(f);
      (void)rd32(f);
      (void)rd16(f);
      w.bitsPerSample = rd16(f);
      gotFmt = true;
    } else if (strcmp(tag, "data") == 0) {
      w.dataOffset = f.position();
      w.dataSize = chunkSize;
      gotData = true;
    }

    if (chunkSize & 1) nextChunk++;
    f.seek(nextChunk);

    if (gotFmt && gotData) break;
  }

  if (!gotFmt || !gotData) {
    setError("fmt/data missing");
    return false;
  }

  if (w.audioFormat != 1) {
    setError("not PCM");
    return false;
  }

  return true;
}
