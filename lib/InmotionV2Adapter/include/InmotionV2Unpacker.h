#ifndef InmotionV2Unpacker_h
#define InmotionV2Unpacker_h

#include <Arduino.h>

class InmotionV2Unpacker {
 public:
  enum UnpackerState {
    unknown,
    flagsearch,
    lensearch,
    collecting,
    done,
  };

  uint8_t* getBuffer();
  size_t getBufferIndex();

  bool addChar(uint8_t c);

 private:
  uint8_t buffer[255];
  size_t bufferIndex = 0;

  uint8_t oldc = 0;
  size_t len = 0;
  uint8_t flags = 0;
  UnpackerState state = UnpackerState::unknown;
};

#endif
