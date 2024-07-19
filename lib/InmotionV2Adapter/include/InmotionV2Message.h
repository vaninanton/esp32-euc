#ifndef InmotionV2Message_h
#define InmotionV2Message_h

#include <Arduino.h>
#include <EUC.h>

class InmotionUnpackerV2 {
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

class InmotionV2Message {
 public:
  InmotionV2Message();
  void printHex(const uint8_t* buffer, size_t length);

  uint8_t checksum(const uint8_t* pData, size_t length);
  bool verify(const uint8_t* pData, size_t length);
  void parse(const uint8_t* pData, size_t length);

  uint signedShortFromBytesLE(const uint8_t* pData, int offset);
  int shortFromBytesLE(const uint8_t* pData, int offset);

 private:
  uint8_t flag;
  uint8_t command;
  size_t dataLength;
};

#endif
