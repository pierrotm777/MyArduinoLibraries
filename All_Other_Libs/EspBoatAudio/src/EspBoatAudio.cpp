#include "EspBoatAudio.h"
#include <cstring>
#include <esp_heap_caps.h>

extern SdFat sd;

static inline uint16_t rd16(SdFile& f) {
  uint8_t b[2];
  if (f.read(b, 2) != 2) return 0;
  return uint16_t(b[0]) | (uint16_t(b[1]) << 8);
}

static inline uint32_t rd32(SdFile& f) {
  uint8_t b[4];
  if (f.read(b, 4) != 4) return 0;

  return uint32_t(b[0]) |
         (uint32_t(b[1]) << 8) |
         (uint32_t(b[2]) << 16) |
         (uint32_t(b[3]) << 24);
}

bool EspBoatAudio::begin(uint8_t bclk,
                         uint8_t lrck,
                         uint8_t dout,
                         uint32_t outputRate,
                         i2s_port_t port,
                         int taskCore) {
  outRate = outputRate;
  txChan = nullptr;

  for (uint8_t i = 0; i < VOICE_COUNT; i++) {
    resetVoiceRuntime(voices[i]);
  }

  resetVoiceRuntime(preloadedEngine);
  preloadedEngineValid = false;

  mutex = xSemaphoreCreateBinary();
  if (!mutex) return false;
  xSemaphoreGive(mutex);

  sdMutex = xSemaphoreCreateBinary();
  if (!sdMutex) return false;
  xSemaphoreGive(sdMutex);

  renderMutex = xSemaphoreCreateBinary();
  if (!renderMutex) return false;
  xSemaphoreGive(renderMutex);

  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(port, I2S_ROLE_MASTER);
  chanCfg.dma_desc_num = 12;
  chanCfg.dma_frame_num = 512;
  chanCfg.auto_clear = true;

  if (i2s_new_channel(&chanCfg, &txChan, nullptr) != ESP_OK) {
    txChan = nullptr;
    return false;
  }

  i2s_std_config_t stdCfg = {};
  stdCfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(outRate);
  stdCfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                      I2S_DATA_BIT_WIDTH_16BIT,
                      I2S_SLOT_MODE_STEREO
                    );

  stdCfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  stdCfg.gpio_cfg.bclk = (gpio_num_t)bclk;
  stdCfg.gpio_cfg.ws   = (gpio_num_t)lrck;
  stdCfg.gpio_cfg.dout = (gpio_num_t)dout;
  stdCfg.gpio_cfg.din  = I2S_GPIO_UNUSED;

  stdCfg.gpio_cfg.invert_flags.mclk_inv = false;
  stdCfg.gpio_cfg.invert_flags.bclk_inv = false;
  stdCfg.gpio_cfg.invert_flags.ws_inv   = false;

  if (i2s_channel_init_std_mode(txChan, &stdCfg) != ESP_OK) {
    i2s_del_channel(txChan);
    txChan = nullptr;
    return false;
  }

  if (i2s_channel_enable(txChan) != ESP_OK) {
    i2s_del_channel(txChan);
    txChan = nullptr;
    return false;
  }

  running = true;

  xTaskCreatePinnedToCore(
    audioTaskThunk,
    "BoatAudio",
    12288,
    this,
    5,
    &audioTaskHandle,
    taskCore
  );

  return audioTaskHandle != nullptr;
}

void EspBoatAudio::end() {
  running = false;

  if (audioTaskHandle) {
    vTaskDelay(pdMS_TO_TICKS(20));
    vTaskDelete(audioTaskHandle);
    audioTaskHandle = nullptr;
  }

  for (uint8_t i = 0; i < VOICE_COUNT; i++) {
    stopVoice((VoiceId)i);
  }

  freeVoiceBuffer(preloadedEngine);
  resetVoiceRuntime(preloadedEngine);
  preloadedEngineValid = false;

  if (renderMutex) {
    vSemaphoreDelete(renderMutex);
    renderMutex = nullptr;
  }

  if (mutex) {
    vSemaphoreDelete(mutex);
    mutex = nullptr;
  }

  if (sdMutex) {
    vSemaphoreDelete(sdMutex);
    sdMutex = nullptr;
  }

  if (txChan) {
    i2s_channel_disable(txChan);
    i2s_del_channel(txChan);
    txChan = nullptr;
  }
}

void EspBoatAudio::audioTaskThunk(void* arg) {
  static_cast<EspBoatAudio*>(arg)->audioTask();
}

void EspBoatAudio::audioTask() {
  uint32_t out[AUDIO_FRAMES];
  size_t written = 0;

  while (running) {
    render(out, AUDIO_FRAMES);

    if (txChan) {
      i2s_channel_write(
        txChan,
        out,
        AUDIO_FRAMES * sizeof(uint32_t),
        &written,
        portMAX_DELAY
      );
    } else {
      vTaskDelay(1);
    }
  }

  vTaskDelete(nullptr);
}

void EspBoatAudio::streamTick() {
  if (!sdMutex || !renderMutex) return;

  int8_t bestVoice = -1;
  int8_t bestBlock = -1;
  int bestScore = 9999;

  xSemaphoreTake(renderMutex, portMAX_DELAY);

  for (uint8_t id = 0; id < VOICE_COUNT; id++) {
    Voice& v = voices[id];

    if (!v.active || !v.streaming || v.streamStop) {
      continue;
    }

    uint8_t readyCount = 0;
    int8_t emptyBlock = -1;

    for (uint8_t b = 0; b < STREAM_BLOCK_COUNT; b++) {
      if (v.streamBlockReady[b]) {
        readyCount++;
      } else if (emptyBlock < 0 && v.streamBlock[b]) {
        emptyBlock = b;
      }
    }

    if (emptyBlock < 0) {
      continue;
    }

    int score = readyCount;

    if (score < bestScore) {
      bestScore = score;
      bestVoice = id;
      bestBlock = emptyBlock;
    }
  }

  xSemaphoreGive(renderMutex);

  if (bestVoice < 0 || bestBlock < 0) return;

  Voice& v = voices[bestVoice];

  uint32_t bytesRead = 0;

  xSemaphoreTake(sdMutex, portMAX_DELAY);

  if (!v.file.isOpen()) {
    xSemaphoreGive(sdMutex);
    return;
  }

  uint32_t dataEnd = v.dataStart + v.dataSize;
  uint32_t filePos = v.file.curPosition();

  if (filePos >= dataEnd) {
    if (v.loop) {
      v.file.seekSet(v.dataStart);
      filePos = v.dataStart;
    } else if (v.repeatLeft > 0) {
      v.repeatLeft--;
      v.file.seekSet(v.dataStart);
      filePos = v.dataStart;
    } else {
      xSemaphoreGive(sdMutex);

      xSemaphoreTake(renderMutex, portMAX_DELAY);
      v.streamEOF = true;
      xSemaphoreGive(renderMutex);

      return;
    }
  }

  uint32_t remaining = dataEnd - filePos;
  uint32_t toRead = STREAM_BLOCK_BYTES;

  if (toRead > remaining) {
    toRead = remaining;
  }

  toRead -= (toRead % v.bytesPerFrame);

  if (toRead > 0) {
    int r = v.file.read(v.streamBlock[bestBlock], toRead);
    if (r > 0) {
      bytesRead = (uint32_t)r;
      bytesRead -= (bytesRead % v.bytesPerFrame);
    }
  }

  xSemaphoreGive(sdMutex);

  if (bytesRead == 0) {
    return;
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);

  v.streamBlockBytes[bestBlock] = bytesRead;
  v.streamBlockReady[bestBlock] = 1;

  xSemaphoreGive(renderMutex);
}

void EspBoatAudio::chainTick() {
  if (!mutex || !renderMutex) return;

  for (uint8_t id = 0; id < VOICE_COUNT; id++) {
    StoredQueueItem next;
    StoredQueueItem savedQueue[VOICE_QUEUE_MAX];

    uint8_t savedCount = 0;
    uint8_t savedIndex = 0;
    bool mustStart = false;

    xSemaphoreTake(renderMutex, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);

    Voice& v = voices[id];

    if (v.queuePending && v.queueIndex < v.queueCount) {
      next = v.queue[v.queueIndex];

      savedCount = v.queueCount;
      savedIndex = v.queueIndex + 1;

      for (uint8_t i = 0; i < VOICE_QUEUE_MAX; i++) {
        savedQueue[i] = v.queue[i];
      }

      v.queuePending = false;
      mustStart = true;
    }

    xSemaphoreGive(mutex);
    xSemaphoreGive(renderMutex);

    if (mustStart) {
      uint8_t repeat = next.loop ? 0 : next.repeatCount;

      AudioHandle h;
      bool ok = false;

      if ((VoiceId)id == VOICE_ENGINE &&
          strcmp(next.path, "@PRELOADED_ENGINE") == 0) {
        h = playPreloadedEngineLoop(true);
        ok = h.valid();
      } else {
        h = playVoiceRepeat(
              (VoiceId)id,
              next.path,
              repeat,
              next.volume,
              next.pitch,
              next.priority,
              true
            );

        ok = h.valid();
      }

      xSemaphoreTake(renderMutex, portMAX_DELAY);
      xSemaphoreTake(mutex, portMAX_DELAY);

      Voice& v2 = voices[id];

      v2.queueCount = savedCount;
      v2.queueIndex = savedIndex;

      for (uint8_t i = 0; i < VOICE_QUEUE_MAX; i++) {
        v2.queue[i] = savedQueue[i];
      }

      if (ok) {
        v2.queuePending = false;
      } else {
        v2.queuePending = (v2.queueIndex < v2.queueCount);
        v2.active = false;
      }

      xSemaphoreGive(mutex);
      xSemaphoreGive(renderMutex);
    }
  }
}

bool EspBoatAudio::enginePlayLoop(const char* path) {
  return playVoice(VOICE_ENGINE, path, true, 1.0f, 1.0f, 255).valid();
}

void EspBoatAudio::engineStop() {
  stopVoice(VOICE_ENGINE);
}

void EspBoatAudio::engineSetPitch(float pitch) {
  setVoicePitch(VOICE_ENGINE, pitch);
}

void EspBoatAudio::engineSetVolume(float volume) {
  setVoiceVolume(VOICE_ENGINE, volume);
}

bool EspBoatAudio::ambientPlayLoop(const char* path) {
  return playVoiceStream(VOICE_AMBIENT, path, true, 1.0f, 1.0f, 200).valid();
}

void EspBoatAudio::ambientStop() {
  stopVoice(VOICE_AMBIENT);
}

void EspBoatAudio::ambientSetVolume(float volume) {
  setVoiceVolume(VOICE_AMBIENT, volume);
}

EspBoatAudio::AudioHandle EspBoatAudio::playFx(const char* path,
                                               float volume,
                                               uint8_t priority,
                                               float pitch)
{
  AudioHandle h;

  if (!path || !mutex) return h;

  int8_t freeSlot = -1;
  uint8_t weakestPriority = 255;
  int8_t weakestSlot = -1;

  xSemaphoreTake(mutex, portMAX_DELAY);

  for (uint8_t i = VOICE_FX0; i <= VOICE_FX3; i++) {
    if (!voices[i].active) {
      freeSlot = i;
      break;
    }

    if (voices[i].priority < weakestPriority) {
      weakestPriority = voices[i].priority;
      weakestSlot = i;
    }
  }

  xSemaphoreGive(mutex);

  if (freeSlot < 0) {
    if (weakestSlot >= 0 && priority >= weakestPriority) {
      freeSlot = weakestSlot;
    } else {
      return h;
    }
  }

  return playVoice(
    (VoiceId)freeSlot,
    path,
    false,
    volume,
    pitch,
    priority,
    false
  );
}

EspBoatAudio::AudioHandle EspBoatAudio::playFxStream(const char* path,
                                                     float volume,
                                                     uint8_t priority,
                                                     float pitch)
{
  AudioHandle h;

  if (!path || !mutex) return h;

  int8_t freeSlot = -1;
  uint8_t weakestPriority = 255;
  int8_t weakestSlot = -1;

  xSemaphoreTake(mutex, portMAX_DELAY);

  for (uint8_t i = VOICE_FX0; i <= VOICE_FX3; i++) {
    if (!voices[i].active) {
      freeSlot = i;
      break;
    }

    if (voices[i].priority < weakestPriority) {
      weakestPriority = voices[i].priority;
      weakestSlot = i;
    }
  }

  xSemaphoreGive(mutex);

  if (freeSlot < 0) {
    if (weakestSlot >= 0 && priority >= weakestPriority) {
      freeSlot = weakestSlot;
    } else {
      return h;
    }
  }

  return playVoiceStream(
    (VoiceId)freeSlot,
    path,
    false,
    volume,
    pitch,
    priority,
    false
  );
}

EspBoatAudio::AudioHandle EspBoatAudio::playVoiceThen(VoiceId id,
                                                      const char* firstPath,
                                                      const char* nextPath,
                                                      bool nextLoop,
                                                      float volume,
                                                      float pitch,
                                                      uint8_t priority) {
  if (id >= VOICE_COUNT || !firstPath || !nextPath) {
    return AudioHandle{};
  }

  QueueItem items[2];

  items[0].path = firstPath;
  items[0].loop = false;
  items[0].repeatCount = 1;
  items[0].volume = volume;
  items[0].pitch = pitch;
  items[0].priority = priority;

  items[1].path = nextPath;
  items[1].loop = nextLoop;
  items[1].repeatCount = nextLoop ? 0 : 1;
  items[1].volume = volume;
  items[1].pitch = pitch;
  items[1].priority = priority;

  return playVoiceQueue(id, items, 2);
}

EspBoatAudio::AudioHandle EspBoatAudio::playVoiceQueue(VoiceId id,
                                                       const QueueItem* items,
                                                       uint8_t count) {
  if (id >= VOICE_COUNT || !items || count == 0) {
    return AudioHandle{};
  }

  if (count > VOICE_QUEUE_MAX) {
    count = VOICE_QUEUE_MAX;
  }

  for (uint8_t i = 0; i < count; i++) {
    if (!items[i].path) {
      return AudioHandle{};
    }
  }

  uint8_t startIndex = 0;
  AudioHandle h;
  bool started = false;

  while (startIndex < count && !started) {
    uint8_t firstRepeat = items[startIndex].loop ? 0 : items[startIndex].repeatCount;

    if (firstRepeat == 0 && !items[startIndex].loop) {
      firstRepeat = 1;
    }

    if (id == VOICE_ENGINE &&
        strcmp(items[startIndex].path, "@PRELOADED_ENGINE") == 0) {
      h = playPreloadedEngineLoop(false);
      started = h.valid();
    } else {
      h = playVoiceRepeat(id,
                          items[startIndex].path,
                          firstRepeat,
                          items[startIndex].volume,
                          items[startIndex].pitch,
                          items[startIndex].priority,
                          false);

      started = h.valid();
    }

    if (!started) {
      startIndex++;
    }
  }

  if (!started) {
    return AudioHandle{};
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[id];

  v.queueCount = count;
  v.queueIndex = startIndex + 1;
  v.queuePending = false;

  for (uint8_t i = 0; i < count; i++) {
    v.queue[i].path[0] = 0;

    if (items[i].path) {
      strncpy(v.queue[i].path, items[i].path, sizeof(v.queue[i].path) - 1);
      v.queue[i].path[sizeof(v.queue[i].path) - 1] = 0;
    }

    v.queue[i].loop = items[i].loop;
    v.queue[i].repeatCount = items[i].loop ? 0 : items[i].repeatCount;

    if (v.queue[i].repeatCount == 0 && !items[i].loop) {
      v.queue[i].repeatCount = 1;
    }

    v.queue[i].volume = items[i].volume;
    v.queue[i].pitch = items[i].pitch;
    v.queue[i].priority = items[i].priority;
  }

  for (uint8_t i = count; i < VOICE_QUEUE_MAX; i++) {
    v.queue[i].path[0] = 0;
    v.queue[i].loop = false;
    v.queue[i].repeatCount = 1;
    v.queue[i].volume = 1.0f;
    v.queue[i].pitch = 1.0f;
    v.queue[i].priority = 1;
  }

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  return h;
}

bool EspBoatAudio::isVoiceQueueDone(VoiceId id) {
  if (id >= VOICE_COUNT || !mutex) return true;

  xSemaphoreTake(mutex, portMAX_DELAY);

  bool done = voices[id].queueCount == 0 ||
              voices[id].queueIndex >= voices[id].queueCount;

  xSemaphoreGive(mutex);

  return done;
}

bool EspBoatAudio::preloadEngineLoop(const char* path,
                                     float volume,
                                     float pitch,
                                     uint8_t priority) {
  if (!path || !mutex || !renderMutex) {
    return false;
  }

  Voice loaded;
  resetVoiceRuntime(loaded);

  loaded.loop = true;
  loaded.streaming = false;
  loaded.priority = priority;
  loaded.volume = constrain(volume, 0.0f, 2.0f);
  loaded.pitch = constrain(pitch, 0.10f, 4.0f);

  if (!openWav(loaded, path)) {
    freeVoiceBuffer(loaded);
    return false;
  }

  updateStep(loaded);
  loaded.active = false;
  loaded.ownsBuffer = true;

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  freeVoiceBuffer(preloadedEngine);
  resetVoiceRuntime(preloadedEngine);

  preloadedEngine.active = false;
  preloadedEngine.loop = true;
  preloadedEngine.repeatLeft = 0;
  preloadedEngine.streaming = false;
  preloadedEngine.streamStop = false;
  preloadedEngine.streamEOF = false;
  preloadedEngine.stereo = loaded.stereo;
  preloadedEngine.priority = loaded.priority;
  preloadedEngine.volume = loaded.volume;
  preloadedEngine.pitch = loaded.pitch;
  preloadedEngine.sampleRate = loaded.sampleRate;
  preloadedEngine.dataStart = loaded.dataStart;
  preloadedEngine.dataSize = loaded.dataSize;
  preloadedEngine.totalFrames = loaded.totalFrames;
  preloadedEngine.bytesPerFrame = loaded.bytesPerFrame;
  preloadedEngine.posQ16 = 0;
  preloadedEngine.stepQ16 = loaded.stepQ16;
  preloadedEngine.buffer = loaded.buffer;
  preloadedEngine.ownsBuffer = true;

  loaded.buffer = nullptr;

  preloadedEngineValid = true;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  return true;
}

EspBoatAudio::AudioHandle EspBoatAudio::playPreloadedEngineLoop(bool keepGeneration) {
  if (!preloadedEngineValid || !mutex || !renderMutex) {
    return AudioHandle{};
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[VOICE_ENGINE];

  uint32_t oldGeneration = v.generation;

  freeVoiceBuffer(v);
  resetVoiceRuntime(v);

  if (keepGeneration) {
    v.generation = oldGeneration;
  } else {
    v.generation = oldGeneration + 1;
  }

  v.active = true;
  v.loop = true;
  v.repeatLeft = 0;
  v.streaming = false;
  v.streamStop = false;
  v.streamEOF = false;
  v.stereo = preloadedEngine.stereo;
  v.priority = preloadedEngine.priority;
  v.volume = preloadedEngine.volume;
  v.pitch = preloadedEngine.pitch;
  v.sampleRate = preloadedEngine.sampleRate;
  v.dataStart = preloadedEngine.dataStart;
  v.dataSize = preloadedEngine.dataSize;
  v.totalFrames = preloadedEngine.totalFrames;
  v.bytesPerFrame = preloadedEngine.bytesPerFrame;
  v.posQ16 = 0;
  v.stepQ16 = preloadedEngine.stepQ16;

  v.buffer = preloadedEngine.buffer;
  v.ownsBuffer = false;

  AudioHandle h;
  h.voice = VOICE_ENGINE;
  h.generation = v.generation;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  return h;
}

void EspBoatAudio::stopFx(uint8_t fxIndex) {
  if (fxIndex > 3) return;
  stopVoice((VoiceId)(VOICE_FX0 + fxIndex));
}

void EspBoatAudio::stopAllFx() {
  for (uint8_t i = 0; i < 4; i++) {
    stopFx(i);
  }
}

EspBoatAudio::AudioHandle EspBoatAudio::playVoice(VoiceId id,
                                                  const char* path,
                                                  bool loop,
                                                  float volume,
                                                  float pitch,
                                                  uint8_t priority,
                                                  bool keepGeneration) {
  if (id >= VOICE_COUNT || !path || !mutex || !renderMutex) {
    return AudioHandle{};
  }

  stopVoice(id);

  Voice loaded;
  resetVoiceRuntime(loaded);

  loaded.loop = loop;
  loaded.streaming = false;
  loaded.priority = priority;
  loaded.volume = constrain(volume, 0.0f, 2.0f);
  loaded.pitch = constrain(pitch, 0.10f, 4.0f);

  if (!openWav(loaded, path)) {
    freeVoiceBuffer(loaded);
    return AudioHandle{};
  }

  updateStep(loaded);
  loaded.active = true;

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[id];

  uint32_t oldGeneration = v.generation;

  freeVoiceBuffer(v);
  resetVoiceRuntime(v);

  if (keepGeneration) {
    v.generation = oldGeneration;
  } else {
    v.generation = oldGeneration + 1;
  }

  v.active = loaded.active;
  v.loop = loaded.loop;
  v.repeatLeft = loaded.repeatLeft;
  v.streaming = false;
  v.streamStop = false;
  v.streamEOF = false;
  v.stereo = loaded.stereo;
  v.priority = loaded.priority;
  v.volume = loaded.volume;
  v.pitch = loaded.pitch;
  v.sampleRate = loaded.sampleRate;
  v.dataStart = loaded.dataStart;
  v.dataSize = loaded.dataSize;
  v.totalFrames = loaded.totalFrames;
  v.bytesPerFrame = loaded.bytesPerFrame;
  v.posQ16 = 0;
  v.stepQ16 = loaded.stepQ16;
  v.buffer = loaded.buffer;
  v.ownsBuffer = true;
  v.streamPath[0] = 0;

  loaded.buffer = nullptr;

  AudioHandle h;
  h.voice = id;
  h.generation = v.generation;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  return h;
}

EspBoatAudio::AudioHandle EspBoatAudio::playVoiceRepeat(VoiceId id,
                                                        const char* path,
                                                        uint8_t playCount,
                                                        float volume,
                                                        float pitch,
                                                        uint8_t priority,
                                                        bool keepGeneration) {
  if (playCount == 0) {
    return playVoice(id, path, true, volume, pitch, priority, keepGeneration);
  }

  AudioHandle h = playVoice(id, path, false, volume, pitch, priority, keepGeneration);

  if (!h.valid()) {
    return AudioHandle{};
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  voices[id].repeatLeft = playCount > 1 ? playCount - 1 : 0;
  h.generation = voices[id].generation;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  return h;
}

EspBoatAudio::AudioHandle EspBoatAudio::playVoiceStream(VoiceId id,
                                                        const char* path,
                                                        bool loop,
                                                        float volume,
                                                        float pitch,
                                                        uint8_t priority,
                                                        bool keepGeneration) {
  if (id >= VOICE_COUNT || !path || !mutex || !renderMutex || !sdMutex) {
    return AudioHandle{};
  }

  stopVoice(id);

  uint8_t* b0 =
    (uint8_t*)heap_caps_malloc(
      STREAM_BLOCK_BYTES,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

  uint8_t* b1 =
    (uint8_t*)heap_caps_malloc(
      STREAM_BLOCK_BYTES,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

  if (!b0) {
    b0 =
      (uint8_t*)heap_caps_malloc(
        STREAM_BLOCK_BYTES,
        MALLOC_CAP_8BIT
      );
  }

  if (!b1) {
    b1 =
      (uint8_t*)heap_caps_malloc(
        STREAM_BLOCK_BYTES,
        MALLOC_CAP_8BIT
      );
  }

  if (!b0 || !b1) {
    if (b0) heap_caps_free(b0);
    if (b1) heap_caps_free(b1);
    return AudioHandle{};
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[id];

  uint32_t oldGeneration = v.generation;

  resetVoiceRuntime(v);

  if (keepGeneration) {
    v.generation = oldGeneration;
  } else {
    v.generation = oldGeneration + 1;
  }

  v.loop = loop;
  v.streaming = true;
  v.priority = priority;
  v.volume = constrain(volume, 0.0f, 2.0f);
  v.pitch = 1.0f;

  v.streamBlock[0] = b0;
  v.streamBlock[1] = b1;
  v.streamBlockReady[0] = 0;
  v.streamBlockReady[1] = 0;
  v.streamBlockBytes[0] = 0;
  v.streamBlockBytes[1] = 0;
  v.streamPlayBlock = 0;
  v.streamPlayOffset = 0;

  strncpy(v.streamPath, path, sizeof(v.streamPath) - 1);
  v.streamPath[sizeof(v.streamPath) - 1] = 0;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  xSemaphoreTake(sdMutex, portMAX_DELAY);

  bool opened = v.file.open(path, O_RDONLY);

  if (!opened) {
    xSemaphoreGive(sdMutex);

    xSemaphoreTake(renderMutex, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    freeVoiceBuffer(v);
    resetVoiceRuntime(v);
    xSemaphoreGive(mutex);
    xSemaphoreGive(renderMutex);

    return AudioHandle{};
  }

  bool parsed = parseWav(v.file, v);

  if (!parsed) {
    v.file.close();
    xSemaphoreGive(sdMutex);

    xSemaphoreTake(renderMutex, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    freeVoiceBuffer(v);
    resetVoiceRuntime(v);
    xSemaphoreGive(mutex);
    xSemaphoreGive(renderMutex);

    return AudioHandle{};
  }

  bool seekOk = v.file.seekSet(v.dataStart);

  if (!seekOk) {
    v.file.close();
    xSemaphoreGive(sdMutex);

    xSemaphoreTake(renderMutex, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    freeVoiceBuffer(v);
    resetVoiceRuntime(v);
    xSemaphoreGive(mutex);
    xSemaphoreGive(renderMutex);

    return AudioHandle{};
  }

  for (uint8_t b = 0; b < STREAM_BLOCK_COUNT; b++) {
    uint32_t dataEnd = v.dataStart + v.dataSize;
    uint32_t filePos = v.file.curPosition();

    if (filePos >= dataEnd) {
      if (v.loop) {
        v.file.seekSet(v.dataStart);
        filePos = v.dataStart;
      } else {
        break;
      }
    }

    uint32_t remaining = dataEnd - filePos;
    uint32_t toRead = STREAM_BLOCK_BYTES;

    if (toRead > remaining) {
      toRead = remaining;
    }

    toRead -= (toRead % v.bytesPerFrame);

    if (toRead == 0) {
      break;
    }

    int r = v.file.read(v.streamBlock[b], toRead);

    if (r <= 0) {
      break;
    }

    uint32_t got = (uint32_t)r;
    got -= (got % v.bytesPerFrame);

    if (got == 0) {
      break;
    }

    v.streamBlockBytes[b] = got;
    v.streamBlockReady[b] = 1;
  }

  xSemaphoreGive(sdMutex);

  if (!v.streamBlockReady[0] && !v.streamBlockReady[1]) {
    xSemaphoreTake(sdMutex, portMAX_DELAY);
    if (v.file.isOpen()) v.file.close();
    xSemaphoreGive(sdMutex);

    xSemaphoreTake(renderMutex, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    freeVoiceBuffer(v);
    resetVoiceRuntime(v);
    xSemaphoreGive(mutex);
    xSemaphoreGive(renderMutex);

    return AudioHandle{};
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  v.active = true;
  v.streamEOF = false;
  v.streamStop = false;
  v.posQ16 = 0;
  v.stepQ16 = 65536;
  v.streamPlayBlock = v.streamBlockReady[0] ? 0 : 1;
  v.streamPlayOffset = 0;

  AudioHandle h;
  h.voice = id;
  h.generation = v.generation;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  return h;
}

EspBoatAudio::AudioHandle EspBoatAudio::playFxAuto(const char* path,
                                                   float volume,
                                                   uint8_t priority,
                                                   float pitch)
{
  AudioHandle h;

  if (!path || !sdMutex) return h;

  SdFile f;

  xSemaphoreTake(sdMutex, portMAX_DELAY);
  bool ok = f.open(path, O_RDONLY);
  uint32_t size = ok ? f.fileSize() : 0;
  if (ok) f.close();
  xSemaphoreGive(sdMutex);

  if (!ok) return h;

  const uint32_t RAM_LIMIT = 200UL * 1024UL;

  if (size <= RAM_LIMIT) {
    return playFx(path, volume, priority, pitch);
  } else {
    return playFxStream(path, volume, priority, pitch);
  }
}

void EspBoatAudio::stopVoice(VoiceId id) {
  if (id >= VOICE_COUNT || !mutex || !renderMutex) return;

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[id];

  v.active = false;
  v.streamStop = true;
  v.queuePending = false;
  v.queueCount = 0;
  v.queueIndex = 0;

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);

  if (sdMutex) {
    xSemaphoreTake(sdMutex, portMAX_DELAY);
    if (v.file.isOpen()) {
      v.file.close();
    }
    xSemaphoreGive(sdMutex);
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);
  xSemaphoreTake(mutex, portMAX_DELAY);

  freeVoiceBuffer(v);
  resetVoiceRuntime(v);

  xSemaphoreGive(mutex);
  xSemaphoreGive(renderMutex);
}

void EspBoatAudio::setVoiceVolume(VoiceId id, float volume) {
  if (id >= VOICE_COUNT || !mutex) return;

  xSemaphoreTake(mutex, portMAX_DELAY);
  voices[id].volume = constrain(volume, 0.0f, 2.0f);
  xSemaphoreGive(mutex);
}

void EspBoatAudio::setVoicePitch(VoiceId id, float pitch) {
  if (id >= VOICE_COUNT || !mutex) return;

  xSemaphoreTake(mutex, portMAX_DELAY);

  voices[id].pitch = constrain(pitch, 0.10f, 4.0f);

  if (!voices[id].streaming) {
    updateStep(voices[id]);
  }

  xSemaphoreGive(mutex);
}

void EspBoatAudio::setMasterVolume(float volume) {
  if (!mutex) return;

  xSemaphoreTake(mutex, portMAX_DELAY);
  masterVolume = constrain(volume, 0.0f, 2.0f);
  xSemaphoreGive(mutex);
}

bool EspBoatAudio::isPlaying(VoiceId id) {
  if (id >= VOICE_COUNT || !mutex) return false;

  xSemaphoreTake(mutex, portMAX_DELAY);
  bool r = voices[id].active;
  xSemaphoreGive(mutex);

  return r;
}

uint32_t EspBoatAudio::positionMillis(VoiceId id) {
  if (id >= VOICE_COUNT || !mutex) return 0;

  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[id];
  uint32_t frame = uint32_t(v.posQ16 >> 16);
  uint32_t ms = v.sampleRate ? (frame * 1000UL) / v.sampleRate : 0;

  xSemaphoreGive(mutex);

  return ms;
}

uint32_t EspBoatAudio::lengthMillis(VoiceId id) {
  if (id >= VOICE_COUNT || !mutex) return 0;

  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[id];
  uint32_t ms = v.sampleRate ? (v.totalFrames * 1000UL) / v.sampleRate : 0;

  xSemaphoreGive(mutex);

  return ms;
}

bool EspBoatAudio::isPlaying(AudioHandle h)
{
  if (!h.valid() || h.voice >= VOICE_COUNT || !mutex) return false;

  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[h.voice];
  bool r = v.active && (v.generation == h.generation);

  xSemaphoreGive(mutex);

  return r;
}

uint32_t EspBoatAudio::positionMillis(AudioHandle h)
{
  if (!h.valid() || h.voice >= VOICE_COUNT || !mutex) return 0;

  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[h.voice];

  uint32_t ms = 0;

  if (v.active && v.generation == h.generation) {
    uint32_t frame = uint32_t(v.posQ16 >> 16);
    ms = v.sampleRate ? (frame * 1000UL) / v.sampleRate : 0;
  }

  xSemaphoreGive(mutex);

  return ms;
}

uint32_t EspBoatAudio::lengthMillis(AudioHandle h)
{
  if (!h.valid() || h.voice >= VOICE_COUNT || !mutex) return 0;

  xSemaphoreTake(mutex, portMAX_DELAY);

  Voice& v = voices[h.voice];

  uint32_t ms = 0;

  if (v.generation == h.generation) {
    ms = v.sampleRate ? (v.totalFrames * 1000UL) / v.sampleRate : 0;
  }

  xSemaphoreGive(mutex);

  return ms;
}

uint32_t EspBoatAudio::remainingMillis(AudioHandle h)
{
  uint32_t len = lengthMillis(h);
  uint32_t pos = positionMillis(h);

  if (pos >= len) return 0;
  return len - pos;
}

bool EspBoatAudio::inWindow(AudioHandle h, uint32_t startMs, uint32_t endMs)
{
  if (!isPlaying(h)) return false;

  uint32_t p = positionMillis(h);

  return p >= startMs && p <= endMs;
}

void EspBoatAudio::stop(AudioHandle h)
{
  if (!h.valid() || h.voice >= VOICE_COUNT || !mutex) return;

  bool ok = false;

  xSemaphoreTake(mutex, portMAX_DELAY);

  if (voices[h.voice].generation == h.generation) {
    ok = true;
  }

  xSemaphoreGive(mutex);

  if (ok) {
    stopVoice((VoiceId)h.voice);
  }
}

bool EspBoatAudio::openWav(Voice& v, const char* path) {
  if (!sdMutex) return false;

  SdFile f;

  xSemaphoreTake(sdMutex, portMAX_DELAY);
  bool opened = f.open(path, O_RDONLY);

  if (!opened) {
    xSemaphoreGive(sdMutex);
    return false;
  }

  bool parsed = parseWav(f, v);

  if (!parsed) {
    f.close();
    xSemaphoreGive(sdMutex);
    return false;
  }

  bool seekOk = f.seekSet(v.dataStart);

  if (!seekOk) {
    f.close();
    xSemaphoreGive(sdMutex);
    return false;
  }

  xSemaphoreGive(sdMutex);

  uint8_t* ram =
    (uint8_t*)heap_caps_malloc(
      v.dataSize,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

  if (!ram) {
    ram =
      (uint8_t*)heap_caps_malloc(
        v.dataSize,
        MALLOC_CAP_8BIT
      );
  }

  if (!ram) {
    xSemaphoreTake(sdMutex, portMAX_DELAY);
    f.close();
    xSemaphoreGive(sdMutex);
    return false;
  }

  uint32_t done = 0;

  while (done < v.dataSize) {
    uint32_t remain = v.dataSize - done;
    uint32_t chunk = remain > 4096 ? 4096 : remain;

    xSemaphoreTake(sdMutex, portMAX_DELAY);
    int n = f.read(ram + done, chunk);
    xSemaphoreGive(sdMutex);

    if (n <= 0) {
      heap_caps_free(ram);

      xSemaphoreTake(sdMutex, portMAX_DELAY);
      f.close();
      xSemaphoreGive(sdMutex);

      return false;
    }

    done += n;
  }

  xSemaphoreTake(sdMutex, portMAX_DELAY);
  f.close();
  xSemaphoreGive(sdMutex);

  v.buffer = ram;
  v.ownsBuffer = true;

  return true;
}

bool EspBoatAudio::parseWav(SdFile& f, Voice& v) {
  char tag[4];

  if (f.read(tag, 4) != 4) return false;
  if (memcmp(tag, "RIFF", 4) != 0) return false;

  rd32(f);

  if (f.read(tag, 4) != 4) return false;
  if (memcmp(tag, "WAVE", 4) != 0) return false;

  bool gotFmt = false;
  bool gotData = false;

  uint16_t audioFormat = 0;
  uint16_t channels = 0;
  uint32_t sampleRate = 0;
  uint16_t bits = 0;
  uint16_t blockAlign = 0;

  while (f.available()) {
    if (f.read(tag, 4) != 4) break;

    uint32_t chunkSize = rd32(f);
    uint32_t chunkData = f.curPosition();
    uint32_t nextChunk = chunkData + chunkSize + (chunkSize & 1);

    if (memcmp(tag, "fmt ", 4) == 0) {
      audioFormat = rd16(f);
      channels = rd16(f);
      sampleRate = rd32(f);
      rd32(f);
      blockAlign = rd16(f);
      bits = rd16(f);
      gotFmt = true;
    } else if (memcmp(tag, "data", 4) == 0) {
      v.dataStart = chunkData;
      v.dataSize = chunkSize;
      gotData = true;
    }

    f.seekSet(nextChunk);

    if (gotFmt && gotData) break;
  }

  if (!gotFmt || !gotData) return false;
  if (audioFormat != 1) return false;
  if (bits != 16) return false;
  if (channels != 1 && channels != 2) return false;
  if (sampleRate < 8000 || sampleRate > 48000) return false;
  if (blockAlign == 0) return false;

  v.stereo = channels == 2;
  v.sampleRate = sampleRate;
  v.bytesPerFrame = blockAlign;
  v.totalFrames = v.dataSize / v.bytesPerFrame;

  return v.totalFrames > 0;
}

bool EspBoatAudio::readFrame(const Voice& v,
                             uint32_t frameIndex,
                             int16_t& l,
                             int16_t& r) {
  if (v.totalFrames == 0 || !v.buffer) {
    l = 0;
    r = 0;
    return false;
  }

  if (frameIndex >= v.totalFrames) {
    if (!v.loop) {
      l = 0;
      r = 0;
      return false;
    }

    frameIndex %= v.totalFrames;
  }

  uint8_t* p = v.buffer + frameIndex * v.bytesPerFrame;

  l = int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));

  if (v.stereo) {
    r = int16_t(uint16_t(p[2]) | (uint16_t(p[3]) << 8));
  } else {
    r = l;
  }

  return true;
}

void EspBoatAudio::updateStep(Voice& v) {
  double base = double(v.sampleRate) / double(outRate);
  double step = base * double(v.pitch) * 65536.0;

  if (step < 1.0) step = 1.0;
  if (step > 262144.0) step = 262144.0;

  v.stepQ16 = uint32_t(step);
}

void EspBoatAudio::render(uint32_t* out, uint32_t frames) {
  if (!renderMutex) {
    memset(out, 0, frames * sizeof(uint32_t));
    return;
  }

  xSemaphoreTake(renderMutex, portMAX_DELAY);

  float mv = masterVolume;

  for (uint32_t i = 0; i < frames; i++) {
    int32_t mixL = 0;
    int32_t mixR = 0;

    for (uint8_t vi = 0; vi < VOICE_COUNT; vi++) {
      Voice& v = voices[vi];

      if (!v.active) continue;

      if (v.streaming) {
        uint8_t pb = v.streamPlayBlock;

        if (!v.streamBlockReady[pb] ||
            !v.streamBlock[pb] ||
            v.streamPlayOffset + v.bytesPerFrame > v.streamBlockBytes[pb]) {

          if (v.streamBlockReady[pb]) {
            v.streamBlockReady[pb] = 0;
            v.streamBlockBytes[pb] = 0;
          }

          uint8_t other = pb ^ 1;

          if (v.streamBlockReady[other]) {
            v.streamPlayBlock = other;
            v.streamPlayOffset = 0;
            pb = other;
          } else {
            if (v.streamEOF) {
              v.active = false;

              if (v.queueIndex < v.queueCount) {
                v.queuePending = true;
              }
            }

            continue;
          }
        }

        uint8_t* p = v.streamBlock[pb] + v.streamPlayOffset;

        int16_t sl =
          int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));

        int16_t sr = sl;

        if (v.stereo) {
          sr = int16_t(uint16_t(p[2]) | (uint16_t(p[3]) << 8));
        }

        v.streamPlayOffset += v.bytesPerFrame;

        if (v.streamPlayOffset >= v.streamBlockBytes[pb]) {
          v.streamBlockReady[pb] = 0;
          v.streamBlockBytes[pb] = 0;
          v.streamPlayOffset = 0;

          uint8_t other = pb ^ 1;
          if (v.streamBlockReady[other]) {
            v.streamPlayBlock = other;
          }
        }

        mixL += int32_t(float(sl) * v.volume);
        mixR += int32_t(float(sr) * v.volume);

        v.posQ16 += 65536;
        continue;
      }

      if (!v.buffer) continue;

      uint32_t frame = uint32_t(v.posQ16 >> 16);

      if (frame >= v.totalFrames) {
        if (v.loop) {
          uint64_t totalQ16 = ((uint64_t)v.totalFrames) << 16;

          if (totalQ16 > 0) {
            v.posQ16 %= totalQ16;
          }

          frame = uint32_t(v.posQ16 >> 16);
        } else if (v.repeatLeft > 0) {
          v.repeatLeft--;
          v.posQ16 = 0;
          frame = 0;
        } else {
          v.active = false;

          if (v.queueIndex < v.queueCount) {
            v.queuePending = true;
          }

          continue;
        }
      }

      uint32_t nextFrame = frame + 1;

      if (nextFrame >= v.totalFrames) {
        nextFrame = (v.loop || v.repeatLeft > 0) ? 0 : frame;
      }

      int16_t l0 = 0;
      int16_t r0 = 0;
      int16_t l1 = 0;
      int16_t r1 = 0;

      if (!readFrame(v, frame, l0, r0)) continue;

      if (!readFrame(v, nextFrame, l1, r1)) {
        l1 = l0;
        r1 = r0;
      }

      uint32_t frac = uint32_t(v.posQ16 & 0xFFFF);

      int32_t l =
        int32_t(l0) +
        (((int32_t(l1) - int32_t(l0)) * int32_t(frac)) >> 16);

      int32_t r =
        int32_t(r0) +
        (((int32_t(r1) - int32_t(r0)) * int32_t(frac)) >> 16);

      mixL += int32_t(float(l) * v.volume);
      mixR += int32_t(float(r) * v.volume);

      v.posQ16 += v.stepQ16;
    }

    mixL = int32_t(float(mixL) * mv);
    mixR = int32_t(float(mixR) * mv);

    int16_t l16 = clip16(mixL);
    int16_t r16 = clip16(mixR);

    out[i] =
      ((uint32_t)(uint16_t)l16 << 16) |
      ((uint16_t)r16);
  }

  xSemaphoreGive(renderMutex);
}

void EspBoatAudio::freeVoiceBuffer(Voice& v) {
  if (v.buffer && v.ownsBuffer) {
    heap_caps_free(v.buffer);
  }

  v.buffer = nullptr;
  v.ownsBuffer = true;

  for (uint8_t i = 0; i < STREAM_BLOCK_COUNT; i++) {
    if (v.streamBlock[i]) {
      heap_caps_free(v.streamBlock[i]);
      v.streamBlock[i] = nullptr;
    }

    v.streamBlockBytes[i] = 0;
    v.streamBlockReady[i] = 0;
  }
}

void EspBoatAudio::resetVoiceRuntime(Voice& v) {
  v.active = false;
  v.loop = false;
  v.repeatLeft = 0;
  v.stereo = false;
  v.streaming = false;
  v.streamStop = false;
  v.streamEOF = false;
  v.priority = 0;
  v.volume = 1.0f;
  v.pitch = 1.0f;
  v.sampleRate = 44100;
  v.dataStart = 0;
  v.dataSize = 0;
  v.totalFrames = 0;
  v.bytesPerFrame = 2;
  v.posQ16 = 0;
  v.stepQ16 = 65536;
  v.buffer = nullptr;
  v.ownsBuffer = true;

  for (uint8_t i = 0; i < STREAM_BLOCK_COUNT; i++) {
    v.streamBlock[i] = nullptr;
    v.streamBlockBytes[i] = 0;
    v.streamBlockReady[i] = 0;
  }

  v.streamPlayBlock = 0;
  v.streamPlayOffset = 0;
  v.streamPath[0] = 0;

  v.queueCount = 0;
  v.queueIndex = 0;
  v.queuePending = false;

  for (uint8_t i = 0; i < VOICE_QUEUE_MAX; i++) {
    v.queue[i].path[0] = 0;
    v.queue[i].loop = false;
    v.queue[i].repeatCount = 1;
    v.queue[i].volume = 1.0f;
    v.queue[i].pitch = 1.0f;
    v.queue[i].priority = 1;
  }
}

int16_t EspBoatAudio::clip16(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return int16_t(x);
}