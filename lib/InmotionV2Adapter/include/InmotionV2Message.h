#ifndef InmotionV2Message_h
#define InmotionV2Message_h

#include <Arduino.h>

class InmotionV2Message {
 public:
  InmotionV2Message();
  void printHex(byte* pData, size_t length);
  byte checksum(byte* pData, size_t length);
  bool verify(byte* pData, size_t length);
  void parse(byte* pData, size_t length);

 private:
};

#endif
