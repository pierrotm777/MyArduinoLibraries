#pragma once

#include <Arduino.h>
#include <SD.h>

#ifndef PRO_MICRO_ENGINE_SOUND_AUDIO_BUFFER_SIZE
#define PRO_MICRO_ENGINE_SOUND_AUDIO_BUFFER_SIZE 256
#endif

#ifndef PRO_MICRO_ENGINE_SOUND_SOURCE_BUFFER_SIZE
#define PRO_MICRO_ENGINE_SOUND_SOURCE_BUFFER_SIZE 64
#endif

class ProMicroEngineSoundClass
{
public:
  enum State : uint8_t {
    STOPPED = 0,
    STARTING,
    IDLE
  };

  bool begin(uint8_t sdCsPin,
             const char *startFile = "/ENGINE.STA",
             const char *idleFile  = "/ENGINE.IDL",
             uint16_t sampleRate = 15625);

  void update();
  void stop();
  void startEngine();

  void setThrottleUs(uint16_t us);
  void setThrottleUs(uint16_t us, uint16_t minUs, uint16_t maxUs);
  void setPitchRangeQ8(uint16_t idleQ8, uint16_t fullQ8);
  void setVolume(uint8_t volume);
  void setFiles(const char *startFile, const char *idleFile);

  bool isPlaying() const { return _state != STOPPED; }
  State state() const { return _state; }

  void onSampleTick();

private:
  friend class ProMicroEngineSoundISRHelper;

  bool openStart();
  bool openIdle();
  bool openSoundFile(const char *name, State newState);
  bool parseWavHeader(File &f, uint32_t &dataStart, uint32_t &dataSize, uint16_t &rate);
  uint16_t readLE16(File &f);
  uint32_t readLE32(File &f);
  void closeCurrent();
  void refillAudioBuffer();
  bool readNextSourceByte(uint8_t &v, bool loopFile);
  void pushSample(uint8_t v);
  uint16_t freeAudioSpace() const;
  uint8_t scaleSample(uint8_t v) const;

  static void setupAudioPwmTimer4();
  static void setupSampleTimer1(uint16_t sampleRate);
  static void stopSampleTimer1();

  uint8_t _sdCsPin = 10;
  const char *_startFileName = "/ENGINE.STA";
  const char *_idleFileName  = "/ENGINE.IDL";

  File _file;
  State _state = STOPPED;
  uint16_t _sampleRate = 15625;

  uint16_t _thrMinUs = 1000;
  uint16_t _thrMaxUs = 2000;
  uint16_t _pitchIdleQ8 = 256;  // 1.00x
  uint16_t _pitchFullQ8 = 512;  // 2.00x
  uint16_t _pitchQ8 = 256;
  uint16_t _phaseQ8 = 0;
  uint8_t  _lastSourceSample = 128;
  uint8_t  _volume = 255;

  uint32_t _dataStart = 0;
  uint32_t _dataEnd = 0;
  uint16_t _fileSampleRate = 0;
  bool _currentIsWav = false;

  uint8_t _sourceBuf[PRO_MICRO_ENGINE_SOUND_SOURCE_BUFFER_SIZE];
  uint8_t _sourcePos = 0;
  uint8_t _sourceLen = 0;

  volatile uint8_t _audioBuf[PRO_MICRO_ENGINE_SOUND_AUDIO_BUFFER_SIZE];
  volatile uint8_t _audioHead = 0;
  volatile uint8_t _audioTail = 0;
};

extern ProMicroEngineSoundClass ProMicroEngineSound;
