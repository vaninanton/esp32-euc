#include <InmotionV2Message.h>

static const char* LOG_TAG = "ESP32-EUC";

InmotionV2Message::InmotionV2Message() {}

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
static uint8_t prevData[256];
static bool isFirst = true;

void byteToBinary(uint8_t byte, char* str) {
  for (int i = 7; i >= 0; i--) {
      *str++ = (byte & (1 << i)) ? '1' : '0';
  }
  *str = '\0'; // null-terminator
}

void InmotionV2Message::printHex(const uint8_t* pData, size_t length) {
  char buffer[512]; // достаточно большой буфер для вывода (примерно 3 символа на байт с пробелами)
  size_t pos = 0;

  for (size_t i = 0; i < length && pos < sizeof(buffer) - 4; i++) {
      pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%02X:", pData[i]);
  }

  ESP_LOGD("EUC", "%s", buffer);

}

void printHexDiff(const uint8_t* pData, size_t length) {
  char buffer[1024] = {0};
  char* ptr = buffer;
  for (size_t i = 0; i < length && (ptr - buffer) < sizeof(buffer) - 16; i++) {
      // char binStr[9]; // 8 бит + \0
      // byteToBinary(pData[i], binStr);
      if (isFirst || pData[i] != prevData[i]) {
          ptr += sprintf(ptr, ANSI_COLOR_RED "%02X " ANSI_COLOR_RESET, pData[i]);
          // ptr += sprintf(ptr, ANSI_COLOR_RED "%d|%02X|%s " ANSI_COLOR_RESET, (i-5), pData[i], binStr);
      } else {
          ptr += sprintf(ptr, "%02X ", pData[i]);
          // ptr += sprintf(ptr, "%d|%02X|%s ", (i-5), pData[i], binStr);
      }
  }

  isFirst = false;
  memcpy(prevData, pData, length);

  log_printf(
      "[%s] "
      "%s"
      "\r\n",
      "M",
      buffer);
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
    ESP_LOGW(LOG_TAG, "Checksum failed! it should be 0x%02hX, but 0x%02hX", xorVal, currentChecksum);
    return false;
  }

  ESP_LOGD(LOG_TAG, "Checksum 0x%02hX ok", xorVal);
  return checksumVerified;
}

uint InmotionV2Message::signedShortFromBytesLE(const uint8_t* pData, int starting) {
  return ((pData[5 + starting + 1] << 8) | (pData[5 + starting] & 0xFF));
}

int InmotionV2Message::shortFromBytesLE(const uint8_t* pData, int starting) {
  return ((pData[5 + starting + 1] & 0xFF) << 8) | (pData[5 + starting] & 0xFF);
}

void InmotionV2Message::parse(const uint8_t* pData, size_t length) {
  printHex(pData, length);
  flag = pData[2];
  dataLength = pData[3];
  command = pData[4] & 0x7F;

  switch (flag) {
    case (byte)0x00:
      // ESP_LOGD(LOG_TAG, "[DEBUG] Packet flag: %02hX NoOp", flag);
      break;
    case (byte)0x11:
      // ESP_LOGD(LOG_TAG, "[DEBUG] Packet flag: %02hX TotalStats", flag);
      break;
    case (byte)0x13:
      // ESP_LOGD(LOG_TAG, "[DEBUG] Packet flag: %02hX UnknownFlag", flag);
      break;
    case (byte)0x14: {
      // ESP_LOGD(LOG_TAG, "[DEBUG] Packet flag: %02hX Default", flag);
      switch (command) {
        case (byte)0x00:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x00 NoOp");
          break;
        case (byte)0x01:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x01 MainVersion");
          break;
        case (byte)0x02:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x02 MainInfo");
          break;
        case (byte)0x03:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x03 Diagnostic");
          break;
        case (byte)0x04: {
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x04 RealTimeInfo");
          // EUC.busVoltage = shortFromBytesLE(pData, 0);
          // EUC.busCurrent = signedShortFromBytesLE(pData, 2);
          // EUC.speed = signedShortFromBytesLE(pData, 4);
          // EUC.torque = signedShortFromBytesLE(pData, 6);
          // EUC.outputRate = signedShortFromBytesLE(pData, 8);
          // EUC.batteryOutputPower = signedShortFromBytesLE(pData, 10);
          // EUC.motorOutputPower = signedShortFromBytesLE(pData, 12);
          // EUC.reserve14 = signedShortFromBytesLE(pData, 14);
          // EUC.pitchAngle = signedShortFromBytesLE(pData, 16);
          // EUC.rollAngle = signedShortFromBytesLE(pData, 18);
          // EUC.aimPitchAngle = signedShortFromBytesLE(pData, 20);
          // EUC.speedingBrakingAngle = shortFromBytesLE(pData, 22);
          // EUC.mileage = shortFromBytesLE(pData, 24);
          // EUC.reserve26 = shortFromBytesLE(pData, 26);
          // EUC.batteryPercentage = shortFromBytesLE(pData, 28);
          // EUC.batteryPercentageForRide = shortFromBytesLE(pData, 30);
          // EUC.estimatedTotalMileage = shortFromBytesLE(pData, 32);
          // EUC.realtimeSpeedLimit = shortFromBytesLE(pData, 34);
          // EUC.realtimeCurrentLimit = shortFromBytesLE(pData, 36);
          // EUC.reserve38 = shortFromBytesLE(pData, 38);
          // EUC.reserve40 = shortFromBytesLE(pData, 40);
          // EUC.mosTemperature = (pData[5 + 42] & 0xff) + 80 - 256;
          // EUC.motorTemperature = (pData[5 + 43] & 0xff) + 80 - 256;
          // EUC.batteryTemperature = (pData[5 + 44] & 0xff) + 80 - 256; // 0;
          // EUC.boardTemperature = (pData[5 + 45] & 0xff) + 80 - 256;
          // EUC.cpuTemperature = (pData[5 + 46] & 0xff) + 80 - 256;
          // EUC.imuTemperature = (pData[5 + 47] & 0xff) + 80 - 256;
          // EUC.lampTemperature = (pData[5 + 48] & 0xff) + 80 - 256; // 0;
          // EUC.envBrightness = pData[5 + 49] & 0xff;
          // EUC.lampBrightness = pData[5 + 50] & 0xff;
          // EUC.reserve51 = pData[5 + 51] & 0xff;
          // EUC.reserve52 = pData[5 + 52] & 0xff;
          // EUC.reserve53 = pData[5 + 53] & 0xff;
          // EUC.reserve54 = pData[5 + 54] & 0xff;
          // EUC.reserve55 = pData[5 + 55] & 0xff;
          // EUC.HMICRunMode = (pData[5 + 56] & 0x07); // lock, drive, shutdown, idle
          // EUC.MCRunMode = (pData[5 + 56] >> 3) & 0x07;
          // EUC.motorState = (pData[5 + 56] >> 6) & 0x01;
          // EUC.chargeState = (pData[5 + 56] >> 7) & 0x01;
          // EUC.backupBatteryState = (pData[5 + 57]) & 0x01; // unknown
          // EUC.lampState = (pData[5 + 57]) & 0x01;
          // EUC.decorativeLightState = (pData[5 + 57] >> 1) & 0x01;
          // EUC.liftedState = (pData[5 + 57] >> 2) & 0x01;
          // EUC.tailLightState = (pData[5 + 57] >> 3) & 0x03; // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
          // EUC.fanState = (pData[5 + 57] >> 7) & 0x01;
          // EUC.brakeState = (pData[5 + 58]) & 0x01;
          // EUC.slowDownState = (pData[5 + 58] >> 2) & 0x01;
          // EUC.DFUState = (pData[5 + 58] >> 3) & 0x01;

          // ESP_LOGD("EUC", "lamp: %d, dho: %d, lift: %d, tail: %d, fan: %d, brake: %d", EUC.lampState, EUC.decorativeLightState, EUC.liftedState, EUC.tailLightState, EUC.fanState, EUC.brakeState);
          break;
        }
        case (byte)0x05:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x05 BatteryRealTimeInfo");
          break;
        case (byte)0x10:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x10 Something");
          break;
        case (byte)0x16:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x10 NEWAPP");
          // printHex(pDataDiff, length);
          break;
        case (byte)0x20:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x20 Settings");
          break;
        case (byte)0x60:
          // ESP_LOGD(LOG_TAG, "[DEBUG] Packet command: 0x60 Control");
          break;

        default:
          // ESP_LOGW(LOG_TAG, "[WARN] Unknown command: 0x%02hX", command);
          break;
      }
      break;
    }
    case (byte)0x16:
      // ESP_LOGD(LOG_TAG, "[DEBUG] Packet flag: %02hX NewAppData", flag);
      // ESP_LOGD(LOG_TAG, "[WARN] Command: 0x%02hX", command);
      switch (command) {
        case (byte)0x2D:
          // printHex(pDataDiff, length);
          break;
        case (byte)0x1B:
          // ESP_LOGD(LOG_TAG, "[WARN] Command: 0x%02hX", command);
          // printHex(pDataDiff, length);
          break;
        case (byte)0x47:
          // ESP_LOGD(LOG_TAG, "[WARN] Command: 0x%02hX", command);
          // printHex(pDataDiff, length);
          break;
        case (byte)0x21:
          // printHex(pDataDiff, length);
          break;
        default:
          // ESP_LOGW(LOG_TAG, "[WARN] Unknown command: 0x%02hX", command);
          break;
      }
      break;
    default:
      // ESP_LOGW(LOG_TAG, "[WARN] Unknown flag: %02hX", flag);
      break;
  }
  printHex(pData, length);
}
