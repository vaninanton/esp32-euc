#include <EUC.h>
#include <InmotionV2Message.h>

InmotionV2Message::InmotionV2Message() {}

void InmotionV2Message::printHex(byte* pData, size_t length) {
  for (size_t i = 0; i < length; i++)
    Serial.printf(" 0x%02hX", pData[i]);
}

byte InmotionV2Message::checksum(byte* pData, size_t length) {
  byte xorVal = 0;
  for (size_t i = 0; i < length; i++) {
    xorVal = (xorVal ^ pData[i]) & 0xFF;
  }

  return xorVal;
}

bool InmotionV2Message::verify(byte* pData, size_t length) {
  byte xorVal = checksum(pData, (length - 1));
  bool result = (xorVal == pData[length - 1]);
  if (!result) {
    Serial.printf("[WARNING] Checksum 0x%02hX failed\n", xorVal);
  }

  // Serial.printf("[DEBUG] Checksum 0x%02hX ok\n", xorVal);
  return result;
}

void InmotionV2Message::parse(byte* pData, size_t length) {
  Serial.println("======================================");
  Serial.println("[RX] Parsing InmotionV2Message message");

  // Если префикс не совпадает, то пропускаем
  if (pData[0] != (byte)0xAA && pData[1] != (byte)0xAA) {
    Serial.println("[WARN] BAD PREFIX notify received: ");
    printHex(pData, length);
    Serial.println();
    return;
  }

  if (pData[2] == (byte)0x00) {
    Serial.println("[DEBUG] Packet flag: NoOp");
  } else if (pData[2] == (byte)0x11) {
    Serial.println("[DEBUG] Packet flag: Initial");
    // TODO: Парсинг пакета инициализации
  } else if (pData[2] == (byte)0x14) {
    // Help:
    // NoOp(0) MainVersion(0x01) MainInfo(0x02) Diagnistic(0x03)
    // RealTimeInfo(0x04) BatteryRealTimeInfo(0x05) Something1(0x10)
    // TotalStats(0x11) Settings(0x20) Control(0x60);
    Serial.printf("[DEBUG] Packet flag: Default, len: %d, command: 0x%02hX\n",
                  pData[3], pData[4] & 0x7F);

    if ((pData[4] & 0x7F) == (byte)0x04) {
      EUC& pEUC = EUC::getInstance();
      pEUC.setMVoltage(((pData[6] & 0xFF) << 8) | (pData[5] & 0xFF));
      pEUC.setSpeed(((pData[10] & 0xFF) << 8) | (pData[9] & 0xFF));
      pEUC.setChargeState((pData[61] >> 7) & 0x01);

      pEUC.debug();
    }
  } else {
    Serial.printf("[DEBUG] Unknown 0x%02hX\n", pData[2]);
  }
  Serial.println("======================================");
  Serial.println();
}
