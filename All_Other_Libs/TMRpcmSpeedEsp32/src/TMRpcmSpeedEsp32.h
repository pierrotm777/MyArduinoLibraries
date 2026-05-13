#pragma once
#include <Arduino.h>
#include <SD.h>
#include "driver/i2s.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define TMRPCM_SPEED_ESP32_VERSION "4.2-XT"
#define TMRPCM_MAX_AUX_SLOTS 3

enum TMRpcmEsp32Format {
  TMRPCM_FMT_AUTO = 0,
  TMRPCM_FMT_S16_STEREO_16000 = 1,
  TMRPCM_FMT_S16_MONO_16000 = 2,
  TMRPCM_FMT_U8_MONO_16000 = 3,
  TMRPCM_FMT_S16_STEREO_44100 = 4
};

class TMRpcmSpeedEsp32 {
public:
  TMRpcmSpeedEsp32();

  void useI2S();
  void setI2SPins(int bclk, int ws, int dout);
  void setFormat(TMRpcmEsp32Format fmt);

  void setVolume(uint16_t percent);
  uint16_t getVolume() const;

  // v3.13: limiteur/compresseur du mix final.
  void setCompressor(bool enabled);
  void setCompressor(uint16_t threshold, float ratio, uint16_t makeupGainPercent = 100);
  bool getCompressorEnabled() const;

  void setAutoMixNormalize(bool enabled);
  bool getAutoMixNormalize() const;

  void setPlaybackRate(float rate);   // compat: moteur uniquement
  float getPlaybackRate() const;

  bool begin();
  bool play(const char *filename);

  bool playMotor(const char *filename);
  void setMotorRate(float rate);      // v3.7: moteur uniquement, I2S reste fixe
  float getMotorRate() const;
  void setMotorVolume(uint16_t percent);
  void setMotorInterpolation(bool enabled);
  bool getMotorInterpolation() const;

  bool preloadAux(uint8_t slot, const char *filename);  // charge l'AUX en RAM/PSRAM
  void clearPreloadedAux(uint8_t slot);

  // v3.16:
  // playAux(slot, filename) devient automatique:
  // - petit fichier -> preload RAM puis lecture
  // - gros fichier -> lecture directe SD
  bool playAux(uint8_t slot, const char *filename);
  bool playAux(uint8_t slot);                         // joue l'AUX déjà préchargé
  bool playAuxFromSD(uint8_t slot, const char *filename); // force lecture SD directe

  void setAutoPreloadMaxBytes(uint32_t maxBytes);
  uint32_t getAutoPreloadMaxBytes() const;

  // v4.2-XT expérimental : tâche audio I2S + ring buffer.
  void setAudioTaskCore(int core);
  void setAudioTaskPriority(uint8_t prio);
  void setXTChunkFrames(uint16_t frames);

  void stopAux(uint8_t slot);
  bool isAuxPlaying(uint8_t slot) const;
  void setAuxVolume(uint8_t slot, uint16_t percent);

  void update();
  void stop();
  bool isPlaying() const;

  const char* getLastError() const;
  const char* version() const;

  uint32_t getWavSampleRate() const;
  uint16_t getWavBitsPerSample() const;
  uint16_t getWavChannels() const;

  bool loopPlayback = false;

private:
  struct WavInfo {
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint16_t channels = 0;
    uint16_t audioFormat = 0;
  };

  static const size_t STREAM_BUF_BYTES = 4096;
  static const size_t MIX_FRAMES = 256;
  static const uint32_t RATE_ONE = 65536UL;

  struct Stream {
    File file;
    WavInfo wav;
    uint32_t remaining = 0;
    bool playing = false;
    bool loop = false;
    uint16_t volume = 100;

    uint8_t buf[STREAM_BUF_BYTES];
    size_t bufPos = 0;
    size_t bufLen = 0;

    // v3.15 : lecture depuis RAM pour les AUX préchargés.
    uint8_t *ramData = nullptr;
    size_t ramSize = 0;
    size_t ramPos = 0;
    bool fromRam = false;
    bool ramOwned = false;

    // v3.17 : nom fichier mémorisé pour fallback SD automatique.
    char filename[64] = {0};
    bool hasFilename = false;
    bool fallbackToSD = false;

    // Moteur uniquement : resampling simple nearest-neighbor.
    float rate = 1.0f;
    uint32_t stepQ16 = RATE_ONE;
    uint32_t phaseQ16 = 0;
    bool hasCachedFrame = false;
    int16_t cachedL = 0;
    int16_t cachedR = 0;
    bool hasNextFrame = false;
    int16_t nextL = 0;
    int16_t nextR = 0;
  };

  static uint16_t rd16(File &f);
  static uint32_t rd32(File &f);
  static bool readTag(File &f, char tag[5]);

  bool parseWav(File &f, WavInfo &w);
  bool formatAccepted(const WavInfo &w) const;
  bool initI2S(uint32_t sampleRate);
  void setError(const char *err);

  bool openStream(Stream &s, const char *filename, bool loop, uint16_t vol);
  bool getWavInfoFromFile(const char *filename, WavInfo &w);
  bool loadStreamToRam(Stream &s, const char *filename, uint16_t vol);
  void closeStream(Stream &s);
  void stopStream(Stream &s);
  bool refillStream(Stream &s);
  bool readRawStereoFrame(Stream &s, int16_t &l, int16_t &r);
  bool readMotorFrame(int16_t &l, int16_t &r);
  bool ensureMotorFrames();

  void updateMotorStep();

  int16_t applyGainClip(int32_t sample, uint16_t vol) const;
  int16_t finalClip(int32_t sample) const;
  int32_t compressSample(int32_t sample) const;

private:
  int _bclk = 8, _ws = 12, _dout = 5;
  TMRpcmEsp32Format _format = TMRPCM_FMT_AUTO;

  uint16_t _volume = 100;
  float _rate = 1.0f;
  bool _motorInterpolation = true;

  bool _compressorEnabled = true;
  uint16_t _compressorThreshold = 22000;
  float _compressorRatio = 4.0f;
  uint16_t _compressorMakeup = 100;
  bool _autoMixNormalize = true;

  // v3.16: seuil auto RAM/SD pour playAux(slot, filename).
  // 1 Mo par défaut : effets courts en RAM, musiques longues depuis SD.
  uint32_t _autoPreloadMaxBytes = 1024UL * 1024UL;

  bool _begun = false;
  bool _i2sStarted = false;
  uint32_t _i2sSampleRate = 44100;

  Stream _motor;
  Stream _aux[TMRPCM_MAX_AUX_SLOTS];

  const char *_lastError = "OK";

  int16_t _mixBuf[MIX_FRAMES * 2];

  // v4.2-XT : ring buffer + tâche audio dédiée.
  static const size_t XT_RING_SLOTS = 8;
  static const size_t XT_SLOT_FRAMES_MAX = 256;

  int16_t _ring[XT_RING_SLOTS][XT_SLOT_FRAMES_MAX * 2];
  volatile uint8_t _ringRead = 0;
  volatile uint8_t _ringWrite = 0;
  volatile uint8_t _ringCount = 0;
  uint16_t _xtChunkFrames = 128;

  TaskHandle_t _audioTaskHandle = nullptr;
  SemaphoreHandle_t _ringMutex = nullptr;
  volatile bool _taskRunning = false;
  volatile bool _taskStop = false;
  int _audioTaskCore = 0;
  uint8_t _audioTaskPriority = 5;

  static void audioTaskThunk(void *arg);
  void audioTask();
  bool startAudioTask();
  void stopAudioTask();
  bool ringPush(const int16_t *samples, size_t frames);
  bool ringPop(int16_t *samples, size_t frames);
  void fillMixFrames(int16_t *dst, size_t frames);
};
