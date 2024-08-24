#include <InmotionV2Unpacker.h>

// Возвращает буфер данных
uint8_t* InmotionV2Unpacker::getBuffer() {
  return buffer;
}

size_t InmotionV2Unpacker::getBufferIndex() {
  return bufferIndex;
}

bool InmotionV2Unpacker::addChar(uint8_t c) {
  if (c != 0xA5 || oldc == 0xA5) {
    switch (state) {
      case UnpackerState::collecting:
        buffer[bufferIndex++] = c;
        if (bufferIndex == len + 5) {
          state = UnpackerState::done;
          oldc = 0;
          return true;
        }
        break;

      case UnpackerState::lensearch:
        buffer[bufferIndex++] = c;
        len = c;
        state = UnpackerState::collecting;
        oldc = c;
        break;

      case UnpackerState::flagsearch:
        buffer[bufferIndex++] = c;
        flags = c;
        state = UnpackerState::lensearch;
        oldc = c;
        break;

      default:
        if (c == 0xAA && oldc == 0xAA) {
          bufferIndex = 0;
          buffer[bufferIndex++] = 0xAA;
          buffer[bufferIndex++] = 0xAA;
          state = UnpackerState::flagsearch;
        }
        oldc = c;
    }
  } else {
    oldc = c;
  }
  return false;
}
