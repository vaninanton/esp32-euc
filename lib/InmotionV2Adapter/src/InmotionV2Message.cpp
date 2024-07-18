#include <EUC.h>
#include <InmotionV2Message.h>

// Возвращает буфер данных
uint8_t* InmotionUnpackerV2::getBuffer() {
  // for (size_t i = 0; i < bufferIndex; i++)
  //   Serial.printf("%02h ", buffer[i]);
  return buffer;
}

size_t InmotionUnpackerV2::getBufferIndex() {
  return bufferIndex;
}

bool InmotionUnpackerV2::addChar(uint8_t c) {
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

InmotionV2Message::InmotionV2Message() {}

void InmotionV2Message::printHex(const uint8_t* buffer, size_t length) {
  for (size_t i = 0; i < length; i++)
    Serial.printf("%02h ", buffer[i]);
}

uint8_t InmotionV2Message::checksum(const uint8_t* pData, size_t length) {
  uint8_t xorVal = 0;
  for (size_t i = 0; i < length; i++) {
    xorVal = (xorVal ^ pData[i]) & 0xFF;
  }

  return xorVal;
}

bool InmotionV2Message::verify(const uint8_t* buffer, size_t length) {
  uint8_t xorVal = checksum(buffer, (length - 1));
  uint8_t currentChecksum = buffer[length - 1];
  bool checksumVerified = (currentChecksum == xorVal);
  if (!checksumVerified) {
    Serial.printf("[WARNING] Checksum failed! it should be 0x%02hX, but 0x%02hX\n", xorVal, currentChecksum);
    return false;
  }

  // Serial.printf("[DEBUG] Checksum 0x%02hX ok\n", xorVal);
  return checksumVerified;
}

uint InmotionV2Message::signedShortFromBytesLE(const uint8_t* pData, int starting) {
  return ((pData[starting + 5 + 1] << 8) | (pData[starting + 5] & 0xFF));
}

int InmotionV2Message::shortFromBytesLE(const uint8_t* pData, int starting) {
  return ((pData[starting + 5 + 1] & 0xFF) << 8) | (pData[starting + 5] & 0xFF);
}

void InmotionV2Message::parse(const uint8_t* pData, size_t length) {
  EUC& pEUC = EUC::getInstance();
  // Serial.println("================================================== [NEW MESSAGE] ==================================================");

  // Serial.print("🟢 ");
  // for (size_t i = 0; i < length; i++)
  //   Serial.printf("0x%02hX ", pData[i]);
  // Serial.println();

  flag = pData[2];
  dataLength = pData[3];
  command = pData[4] & 0x7F;

  // NoOp(0),
  // Version=0x01,
  // info=0x02,
  // Diagnostic=0x03,
  // live=0x04,
  // bms=0x05,
  // Something1=0x16,
  // stats=0x17,
  // Settings=0x32,
  // control=0x96;

  switch (flag) {
    case (byte)0x00:
      // Serial.println("[DEBUG] Packet flag: 0x00 NoOp");
      break;
    case (byte)0x11:
      // Serial.println("[DEBUG] Packet flag: 0x11 TotalStats");
      break;
    case (byte)0x13:
      // Serial.println("[DEBUG] Packet flag: 0x13 UnknownFlag");
      break;
    case (byte)0x14: {
      // Serial.println("[DEBUG] Packet flag: 0x14 Default");
      switch (command) {
        case (byte)0x00:
          // Serial.println("[DEBUG] Packet command: 0x00 NoOp");
          break;
        case (byte)0x01:
          // Serial.println("[DEBUG] Packet command: 0x01 MainVersion");
          break;
        case (byte)0x02:
          // Serial.println("[DEBUG] Packet command: 0x02 MainInfo");
          break;
        case (byte)0x03:
          // Serial.println("[DEBUG] Packet command: 0x03 Diagnostic");
          break;
        case (byte)0x04: {
          // Serial.println("[DEBUG] Packet command: 0x04 RealTimeInfo");

          pEUC.busVoltage = shortFromBytesLE(pData, 0);
          pEUC.busCurrent = signedShortFromBytesLE(pData, 2);
          pEUC.speed = signedShortFromBytesLE(pData, 4);
          pEUC.torque = signedShortFromBytesLE(pData, 6);
          pEUC.outputRate = signedShortFromBytesLE(pData, 8);
          pEUC.batteryOutputPower = signedShortFromBytesLE(pData, 10);
          pEUC.motorOutputPower = signedShortFromBytesLE(pData, 12);
          pEUC.reserve14 = signedShortFromBytesLE(pData, 14);
          pEUC.pitchAngle = signedShortFromBytesLE(pData, 16);
          pEUC.rollAngle = signedShortFromBytesLE(pData, 18);
          pEUC.aimPitchAngle = signedShortFromBytesLE(pData, 20);
          pEUC.speedingBrakingAngle = shortFromBytesLE(pData, 22);
          pEUC.mileage = shortFromBytesLE(pData, 24);
          pEUC.reserve26 = shortFromBytesLE(pData, 26);
          pEUC.batteryPercentage = shortFromBytesLE(pData, 28);
          pEUC.batteryPercentageForRide = shortFromBytesLE(pData, 30);
          pEUC.estimatedTotalMileage = shortFromBytesLE(pData, 32);
          pEUC.realtimeSpeedLimit = shortFromBytesLE(pData, 34);
          pEUC.realtimeCurrentLimit = shortFromBytesLE(pData, 36);
          pEUC.reserve38 = shortFromBytesLE(pData, 38);
          pEUC.reserve40 = shortFromBytesLE(pData, 40);
          pEUC.mosTemperature = (pData[5 + 42] & 0xff) + 80 - 256;
          pEUC.motorTemperature = (pData[5 + 43] & 0xff) + 80 - 256;
          pEUC.batteryTemperature = (pData[5 + 44] & 0xff) + 80 - 256;  // 0;
          pEUC.boardTemperature = (pData[5 + 45] & 0xff) + 80 - 256;
          pEUC.cpuTemperature = (pData[5 + 46] & 0xff) + 80 - 256;
          pEUC.imuTemperature = (pData[5 + 47] & 0xff) + 80 - 256;
          pEUC.lampTemperature = (pData[5 + 48] & 0xff) + 80 - 256;  // 0;
          pEUC.envBrightness = pData[5 + 49] & 0xff;
          pEUC.lampBrightness = pData[5 + 50] & 0xff;
          pEUC.reserve51 = pData[5 + 51] & 0xff;
          pEUC.reserve52 = pData[5 + 52] & 0xff;
          pEUC.reserve53 = pData[5 + 53] & 0xff;
          pEUC.reserve54 = pData[5 + 54] & 0xff;
          pEUC.reserve55 = pData[5 + 55] & 0xff;

          pEUC.HMICRunMode = (pData[5 + 56] & 0x07);  // lock, drive, shutdown, idle
          pEUC.MCRunMode = (pData[5 + 56] >> 3) & 0x07;
          pEUC.motorState = (pData[5 + 56] >> 6) & 0x01;
          pEUC.chargeState = (pData[5 + 56] >> 7) & 0x01;
          pEUC.backupBatteryState = (pData[5 + 57]) & 0x01;
          pEUC.lampState = (pData[5 + 57] >> 1) & 0x01;
          pEUC.decorativeLightState = (pData[5 + 57] >> 2) & 0x01;
          pEUC.liftedState = (pData[5 + 57] >> 3) & 0x03;
          pEUC.tailLightState = (pData[5 + 57] >> 4) & 0x07; // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
          pEUC.fanState = (pData[5 + 57] >> 7) & 0x01;
          pEUC.brakeState = (pData[5 + 58]) & 0x01;
          pEUC.slowDownState = (pData[5 + 58] >> 2) & 0x01;
          pEUC.DFUState = (pData[5 + 58] >> 3) & 0x01;
          pEUC.debug();
          break;
        }
        case (byte)0x05:
          // Serial.println("[DEBUG] Packet command: 0x05 BatteryRealTimeInfo");
          break;
        case (byte)0x10:
          // Serial.println("[DEBUG] Packet command: 0x10 Something");
          break;
        case (byte)0x20:
          // Serial.println("[DEBUG] Packet command: 0x20 Settings");
          break;
        case (byte)0x60:
          // Serial.println("[DEBUG] Packet command: 0x60 Control");
          break;

        default:
          // Serial.printf("[WARN] Unknown command: 0x%02hX\n", command);
          break;
      }
      break;
    }
    default:
      // Serial.printf("[WARN] Unknown flag: 0x%02hX\n", flag);
      break;
  }

  // pEUC.debug();
  // Serial.println();
}
