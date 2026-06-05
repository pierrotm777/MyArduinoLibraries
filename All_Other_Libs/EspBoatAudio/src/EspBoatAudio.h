#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include "driver/i2s_std.h"

class EspBoatAudio {
public:
  enum VoiceId : uint8_t {
    VOICE_ENGINE = 0,
    VOICE_AMBIENT,
	VOICE_ANCHOR,
    VOICE_FX0,
    VOICE_FX1,
    VOICE_FX2,
    VOICE_FX3,
    VOICE_COUNT
  };

  struct AudioHandle {
    int8_t voice = -1;
    uint32_t generation = 0;

    bool valid() const {
      return voice >= 0;
    }
  };

  struct QueueItem {
    const char* path = nullptr;
    bool loop = false;
    uint8_t repeatCount = 1;
    float volume = 1.0f;
    float pitch = 1.0f;
    uint8_t priority = 1;
  };

  bool begin(uint8_t bclk,
             uint8_t lrck,
             uint8_t dout,
             uint32_t outputRate = 44100,
             i2s_port_t port = I2S_NUM_0,
             int taskCore = 1);

  void end();

  void streamTick();
  void chainTick();

  bool enginePlayLoop(const char* path);
  void engineStop();
  void engineSetPitch(float pitch);
  void engineSetVolume(float volume);
  void engineGenericStop(uint32_t durationMs = 1800,
                         float targetPitch = 0.45f);

  bool ambientPlayLoop(const char* path);
  void ambientStop();
  void ambientSetVolume(float volume);

  AudioHandle playFx(const char* path,
                     float volume = 1.0f,
                     uint8_t priority = 1,
                     float pitch = 1.0f);

  AudioHandle playFxStream(const char* path,
                           float volume = 1.0f,
                           uint8_t priority = 1,
                           float pitch = 1.0f);

  AudioHandle playFxAuto(const char* path,
                         float volume = 1.0f,
                         uint8_t priority = 1,
                         float pitch = 1.0f);

  void stopFx(uint8_t fxIndex);
  void stopAllFx();

  AudioHandle playVoice(VoiceId id,
                        const char* path,
                        bool loop,
                        float volume,
                        float pitch,
                        uint8_t priority,
                        bool keepGeneration = false);

  AudioHandle playVoiceStream(VoiceId id,
                              const char* path,
                              bool loop,
                              float volume,
                              float pitch,
                              uint8_t priority,
                              bool keepGeneration = false);

  AudioHandle playVoiceRepeat(VoiceId id,
                              const char* path,
                              uint8_t playCount,
                              float volume = 1.0f,
                              float pitch = 1.0f,
                              uint8_t priority = 1,
                              bool keepGeneration = false);

  AudioHandle playVoiceStreamRepeat(VoiceId id,
                                    const char* path,
                                    uint8_t playCount,
                                    float volume = 1.0f,
                                    float pitch = 1.0f,
                                    uint8_t priority = 1,
                                    bool keepGeneration = false);

  AudioHandle playFxRepeat(const char* path,
                           uint8_t playCount,
                           float volume = 1.0f,
                           uint8_t priority = 1,
                           float pitch = 1.0f);

  AudioHandle playFxRepeatStream(const char* path,
                                 uint8_t playCount,
                                 float volume = 1.0f,
                                 uint8_t priority = 1,
                                 float pitch = 1.0f);

  AudioHandle playFxRepeatAuto(const char* path,
                               uint8_t playCount,
                               float volume = 1.0f,
                               uint8_t priority = 1,
                               float pitch = 1.0f);

  AudioHandle playVoiceThen(VoiceId id,
                            const char* firstPath,
                            const char* nextPath,
                            bool nextLoop,
                            float volume = 1.0f,
                            float pitch = 1.0f,
                            uint8_t priority = 1);

  AudioHandle playVoiceQueue(VoiceId id,
                             const QueueItem* items,
                             uint8_t count);

  bool isVoiceQueueDone(VoiceId id);

  bool preloadEngineLoop(const char* path,
                         float volume = 1.0f,
                         float pitch = 1.0f,
                         uint8_t priority = 255);

  AudioHandle playPreloadedEngineLoop(bool keepGeneration = false);

  void stopVoice(VoiceId id);

  void setVoiceVolume(VoiceId id, float volume);
  void requestVoiceStop(VoiceId id);
  void setVoicePitch(VoiceId id, float pitch);
  void setMasterVolume(float volume);

  void fadeVoice(VoiceId id, float targetVolume, uint32_t durationMs);
  void fadeOutAndStop(VoiceId id, uint32_t durationMs);

  void duckVoice(VoiceId id, float factor, uint32_t attackMs);
  void unduckVoice(VoiceId id, uint32_t releaseMs);

  void duck(AudioHandle h, float factor, uint32_t attackMs);
  void unduck(AudioHandle h, uint32_t releaseMs);

  bool isPlaying(VoiceId id);
  uint32_t positionMillis(VoiceId id);
  uint32_t lengthMillis(VoiceId id);

  bool isPlaying(AudioHandle h);
  uint32_t positionMillis(AudioHandle h);
  uint32_t lengthMillis(AudioHandle h);
  uint32_t remainingMillis(AudioHandle h);
  bool inWindow(AudioHandle h, uint32_t startMs, uint32_t endMs);
  void stop(AudioHandle h);

  bool preloadFxCache(const char* path);
  void clearFxCache();
  void printVoicesStatus();

private:
  enum AudioCodec : uint8_t {
    CODEC_PCM16 = 1,
    CODEC_IMA_ADPCM = 17
  };

  struct CachedFx {
    char path[96];
    uint8_t* buffer = nullptr;       // PCM16 data or compressed IMA ADPCM data
    uint32_t dataSize = 0;           // PCM bytes or compressed ADPCM data bytes
    uint32_t totalFrames = 0;
    uint16_t bytesPerFrame = 0;      // decoded PCM frame size
    uint16_t wavBlockAlign = 0;      // PCM frame size or ADPCM compressed block size
    uint16_t adpcmSamplesPerBlock = 0;
    uint32_t sampleRate = 0;
    bool stereo = false;
    AudioCodec codec = CODEC_PCM16;
    bool valid = false;
  };

  static constexpr uint8_t FX_CACHE_COUNT = 16;
  CachedFx fxCache[FX_CACHE_COUNT];

  int8_t findFxCache(const char* path);

  static constexpr uint32_t AUDIO_FRAMES = 1024;

  static constexpr uint8_t STREAM_BLOCK_COUNT = 2;
  static constexpr uint32_t STREAM_BLOCK_BYTES = 16384;

  static constexpr uint8_t VOICE_QUEUE_MAX = 4;

  struct StoredQueueItem {
    char path[64] = {0};
    bool loop = false;
    uint8_t repeatCount = 1;
    float volume = 1.0f;
    float pitch = 1.0f;
    uint8_t priority = 1;
  };

  struct Voice {
    SdFile file;

    bool active = false;
    uint32_t generation = 0;
	bool stopRequested = false;

    bool loop = false;
    bool stereo = false;
    bool streaming = false;
    AudioCodec codec = CODEC_PCM16;

    volatile bool streamStop = false;
    volatile bool streamEOF = false;

    uint8_t priority = 0;

    float volume = 1.0f;
    float baseVolume = 1.0f;
    float pitch = 1.0f;

    float fadeStartVolume = 1.0f;
    float fadeTargetVolume = 1.0f;
    uint32_t fadeStartMs = 0;
    uint32_t fadeDurationMs = 0;
    bool fading = false;
    bool stopAfterFade = false;

    float pitchFadeStart = 1.0f;
    float pitchFadeTarget = 1.0f;
    uint32_t pitchFadeStartMs = 0;
    uint32_t pitchFadeDurationMs = 0;
    bool pitchFading = false;

    uint32_t sampleRate = 44100;
    uint32_t dataStart = 0;
    uint32_t dataSize = 0;
    uint32_t totalFrames = 0;

    uint16_t bytesPerFrame = 2;        // PCM decoded frame size
    uint16_t wavBlockAlign = 2;        // PCM frame size or ADPCM compressed block size
    uint16_t adpcmSamplesPerBlock = 0;

    uint64_t posQ16 = 0;
    uint32_t stepQ16 = 65536;
    uint16_t repeatLeft = 0;

    uint8_t* buffer = nullptr;         // PCM16 data or compressed ADPCM data
    bool ownsBuffer = true;

    uint8_t* decodedBlock = nullptr;   // decoded PCM block for cached/RAM ADPCM
    uint32_t decodedBlockIndex = 0xFFFFFFFFUL;
    uint32_t decodedBlockBytes = 0;
    uint32_t decodedBlockFrames = 0;

    uint8_t* streamBlock[STREAM_BLOCK_COUNT] = {nullptr, nullptr};           // decoded PCM
    uint32_t streamBlockBytes[STREAM_BLOCK_COUNT] = {0, 0};
    volatile uint8_t streamBlockReady[STREAM_BLOCK_COUNT] = {0, 0};

    uint8_t* streamCompressedBlock[STREAM_BLOCK_COUNT] = {nullptr, nullptr}; // ADPCM compressed input
    uint32_t streamCompressedBlockBytes[STREAM_BLOCK_COUNT] = {0, 0};

    uint8_t streamPlayBlock = 0;
    uint32_t streamPlayOffset = 0;

    char streamPath[96] = {0};
	char currentPath[96] = {0};

    StoredQueueItem queue[VOICE_QUEUE_MAX];
    uint8_t queueCount = 0;
    uint8_t queueIndex = 0;
    bool queuePending = false;
  };

  Voice voices[VOICE_COUNT];
  Voice preloadedEngine;
  bool preloadedEngineValid = false;

  i2s_chan_handle_t txChan = nullptr;
  uint32_t outRate = 44100;

  float masterVolume = 1.0f;

  TaskHandle_t audioTaskHandle = nullptr;

  SemaphoreHandle_t mutex = nullptr;
  SemaphoreHandle_t sdMutex = nullptr;
  SemaphoreHandle_t renderMutex = nullptr;

  volatile bool running = false;

  static void audioTaskThunk(void* arg);
  void audioTask();

  bool openWav(Voice& v, const char* path);
  bool parseWav(SdFile& f, Voice& v);

  bool decodeImaAdpcmMonoBlock(const uint8_t* in,
                               uint32_t inBytes,
                               uint8_t* out,
                               uint32_t outCapacity,
                               uint32_t& outBytes,
                               uint32_t& outFrames);

  bool readFrame(Voice& v,
                 uint32_t frameIndex,
                 int16_t& l,
                 int16_t& r);

  void updateStep(Voice& v);
  void render(uint32_t* out, uint32_t frames);

  void freeVoiceBuffer(Voice& v);
  void resetVoiceRuntime(Voice& v);

  void startFadeNoLock(Voice& v,
                       float targetVolume,
                       uint32_t durationMs,
                       bool stopAfterFade);

  void startPitchFadeNoLock(Voice& v,
                            float targetPitch,
                            uint32_t durationMs);

  static int16_t clip16(int32_t x);
  static int32_t softLimit(int32_t x);
};
