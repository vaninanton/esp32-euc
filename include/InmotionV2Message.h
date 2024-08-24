#pragma once
#include <Arduino.h>
#include <EUC.h>

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

