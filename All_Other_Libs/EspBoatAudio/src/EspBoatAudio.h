#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include "driver/i2s_std.h"

class EspBoatAudio {
public:
  enum VoiceId : uint8_t {
    VOICE_ENGINE = 0,
    VOICE_AMBIENT,
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
  void setVoicePitch(VoiceId id, float pitch);
  void setMasterVolume(float volume);

  bool isPlaying(VoiceId id);
  uint32_t positionMillis(VoiceId id);
  uint32_t lengthMillis(VoiceId id);

  bool isPlaying(AudioHandle h);
  uint32_t positionMillis(AudioHandle h);
  uint32_t lengthMillis(AudioHandle h);
  uint32_t remainingMillis(AudioHandle h);
  bool inWindow(AudioHandle h, uint32_t startMs, uint32_t endMs);
  void stop(AudioHandle h);

private:
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

    bool loop = false;
    bool stereo = false;
    bool streaming = false;

    volatile bool streamStop = false;
    volatile bool streamEOF = false;

    uint8_t priority = 0;

    float volume = 1.0f;
    float pitch = 1.0f;

    uint32_t sampleRate = 44100;
    uint32_t dataStart = 0;
    uint32_t dataSize = 0;
    uint32_t totalFrames = 0;

    uint16_t bytesPerFrame = 2;

    uint64_t posQ16 = 0;
    uint32_t stepQ16 = 65536;
    uint16_t repeatLeft = 0;

    uint8_t* buffer = nullptr;
    bool ownsBuffer = true;

    uint8_t* streamBlock[STREAM_BLOCK_COUNT] = {nullptr, nullptr};
    uint32_t streamBlockBytes[STREAM_BLOCK_COUNT] = {0, 0};
    volatile uint8_t streamBlockReady[STREAM_BLOCK_COUNT] = {0, 0};
    uint8_t streamPlayBlock = 0;
    uint32_t streamPlayOffset = 0;

    char streamPath[96] = {0};

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

  bool readFrame(const Voice& v,
                 uint32_t frameIndex,
                 int16_t& l,
                 int16_t& r);

  void updateStep(Voice& v);
  void render(uint32_t* out, uint32_t frames);

  void freeVoiceBuffer(Voice& v);
  void resetVoiceRuntime(Voice& v);

  static int16_t clip16(int32_t x);
};