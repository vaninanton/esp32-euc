#include <InmotionV2Message.h>

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
  return ((pData[5 + starting + 1] << 8) | (pData[5 + starting] & 0xFF));
}

int InmotionV2Message::shortFromBytesLE(const uint8_t* pData, int starting) {
  return ((pData[5 + starting + 1] & 0xFF) << 8) | (pData[5 + starting] & 0xFF);
}

void InmotionV2Message::parse(const uint8_t* pData, size_t length) {
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

          EUC.busVoltage = shortFromBytesLE(pData, 0);
          EUC.busCurrent = signedShortFromBytesLE(pData, 2);
          EUC.speed = signedShortFromBytesLE(pData, 4);
          EUC.torque = signedShortFromBytesLE(pData, 6);
          EUC.outputRate = signedShortFromBytesLE(pData, 8);
          EUC.batteryOutputPower = signedShortFromBytesLE(pData, 10);
          EUC.motorOutputPower = signedShortFromBytesLE(pData, 12);
          EUC.reserve14 = signedShortFromBytesLE(pData, 14);
          EUC.pitchAngle = signedShortFromBytesLE(pData, 16);
          EUC.rollAngle = signedShortFromBytesLE(pData, 18);
          EUC.aimPitchAngle = signedShortFromBytesLE(pData, 20);
          EUC.speedingBrakingAngle = shortFromBytesLE(pData, 22);
          EUC.mileage = shortFromBytesLE(pData, 24);
          EUC.reserve26 = shortFromBytesLE(pData, 26);
          EUC.batteryPercentage = shortFromBytesLE(pData, 28);
          EUC.batteryPercentageForRide = shortFromBytesLE(pData, 30);
          EUC.estimatedTotalMileage = shortFromBytesLE(pData, 32);
          EUC.realtimeSpeedLimit = shortFromBytesLE(pData, 34);
          EUC.realtimeCurrentLimit = shortFromBytesLE(pData, 36);
          EUC.reserve38 = shortFromBytesLE(pData, 38);
          EUC.reserve40 = shortFromBytesLE(pData, 40);
          EUC.mosTemperature = (pData[5 + 42] & 0xff) + 80 - 256;
          EUC.motorTemperature = (pData[5 + 43] & 0xff) + 80 - 256;
          EUC.batteryTemperature = (pData[5 + 44] & 0xff) + 80 - 256; // 0;
          EUC.boardTemperature = (pData[5 + 45] & 0xff) + 80 - 256;
          EUC.cpuTemperature = (pData[5 + 46] & 0xff) + 80 - 256;
          EUC.imuTemperature = (pData[5 + 47] & 0xff) + 80 - 256;
          EUC.lampTemperature = (pData[5 + 48] & 0xff) + 80 - 256; // 0;
          EUC.envBrightness = pData[5 + 49] & 0xff;
          EUC.lampBrightness = pData[5 + 50] & 0xff;
          EUC.reserve51 = pData[5 + 51] & 0xff;
          EUC.reserve52 = pData[5 + 52] & 0xff;
          EUC.reserve53 = pData[5 + 53] & 0xff;
          EUC.reserve54 = pData[5 + 54] & 0xff;
          EUC.reserve55 = pData[5 + 55] & 0xff;

          EUC.HMICRunMode = (pData[5 + 56] & 0x07); // lock, drive, shutdown, idle
          EUC.MCRunMode = (pData[5 + 56] >> 3) & 0x07;
          EUC.motorState = (pData[5 + 56] >> 6) & 0x01;
          EUC.chargeState = (pData[5 + 56] >> 7) & 0x01;
          EUC.backupBatteryState = (pData[5 + 57]) & 0x01;
          EUC.lampState = (pData[5 + 57] >> 1) & 0x01;
          EUC.decorativeLightState = (pData[5 + 57] >> 2) & 0x01;
          EUC.liftedState = (pData[5 + 57] >> 3) & 0x03;
          EUC.tailLightState = (pData[5 + 57] >> 4) & 0x07; // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
          EUC.fanState = (pData[5 + 57] >> 7) & 0x01;
          EUC.brakeState = (pData[5 + 58]) & 0x01;
          EUC.slowDownState = (pData[5 + 58] >> 2) & 0x01;
          EUC.DFUState = (pData[5 + 58] >> 3) & 0x01;
          // EUC.debug();
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

  // EUC.debug();
  // Serial.println();
}
