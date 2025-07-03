#pragma once
#include <Arduino.h>
#include <InmotionV2Message.h>
#include <InmotionV2Unpacker.h>
#include <NimBLEDevice.h>

static const NimBLEUUID uartServiceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID ffe5ServiceUUID("FFE5");
static const NimBLEUUID ffe9CharUUID("FFE9");
static const NimBLEUUID ffe0ServiceCharUUID("FFE0");
static const NimBLEUUID ffe4CharUUID("FFE4");

class eucClass {
 public:
  void setup();
  void debug();

  void tick();

  /// @brief Создание BLE сервера для подключения к приложению
  void createAppBleServer();

  /// @brief Создание BLE клиента для подключения к EUC
  void createEucBleClient();

  /// @brief Запуск сканирования BLE устройств
  void startBleScan();

  /// @brief Обработка результата сканирования
  void onScanResult(const NimBLEAdvertisedDevice* advertisedDevice);

  /// @brief Обработка подключения к EUC
  void onEucConnected(NimBLEClient* pClient);

  /// @brief Обработка отключения от EUC
  void onEucDisconnected(NimBLEClient* pClient, int reason);

  /// @brief Подключение к EUC
  void connectToEuc();

  void advertiseApp();

  /// @brief Подписка на характеристики EUC
  void subscribeToEucCharacteristics();

  /// @brief Обработка характеристик EUC
  static void onEucNotifyReceived(NimBLERemoteCharacteristic* pChar,
                                  uint8_t* data,
                                  size_t dataLength,
                                  bool isNotify);

  /// @brief Обработка подключения к приложению
  void onAppConnected(NimBLEServer* pServer, NimBLEConnInfo& connInfo);

  /// @brief Обработка отключения от приложения
  void onAppDisconnected(NimBLEServer* pServer,
                         NimBLEConnInfo& connInfo,
                         int reason);

  /// @brief Обработка события подписки приложения к устройству
  void onAppSubscribe(NimBLECharacteristic* pChar,
                      NimBLEConnInfo& connInfo,
                      uint16_t subValue);

  /// @brief Обработка события отписки приложения от устройства
  void onAppUnsubscribe(NimBLECharacteristic* pChar,
                        NimBLEConnInfo& connInfo,
                        uint16_t subValue);

  /// @brief Проксирование приложения к EUC
  void sendCharacteristicToEuc(NimBLECharacteristic* pChar,
                               NimBLEConnInfo& connInfo);

  unsigned long failedScanCount = 0;
  unsigned long latestScan = 0;

  /// @brief Найденное, но ещё не подключённое устройство
  const NimBLEAdvertisedDevice* advertisedDevice = nullptr;
  NimBLEClient* eucBleClient = nullptr;
  NimBLEServer* appBleServer = nullptr;

  bool eucSubscribed = false;
  bool appSubscribed = false;

  int16_t busVoltage;
  int16_t busCurrent;
  int16_t speed;
  int16_t torque;
  int16_t outputRate;
  int16_t batteryOutputPower;
  int16_t motorOutputPower;
  uint16_t reserve14;
  int16_t pitchAngle;
  int16_t rollAngle;
  int16_t aimPitchAngle;
  int16_t speedingBrakingAngle;
  uint16_t mileage;
  uint16_t reserve26;
  uint16_t batteryPercentage;
  uint16_t batteryPercentageForRide;
  uint16_t estimatedTotalMileage;
  int16_t realtimeSpeedLimit;
  int16_t realtimeCurrentLimit;
  uint16_t reserve38;
  uint16_t reserve40;
  int8_t mosTemperature;
  int8_t motorTemperature;
  int8_t batteryTemperature;
  int8_t boardTemperature;
  int8_t cpuTemperature;
  int8_t imuTemperature;
  int8_t lampTemperature;
  uint8_t envBrightness;
  uint8_t lampBrightness;
  uint8_t reserve51;
  uint8_t reserve52;
  uint8_t reserve53;
  uint8_t reserve54;
  uint8_t reserve55;

  uint8_t HMICRunMode;
  bool MCRunMode;
  bool motorState;
  bool chargeState;
  bool backupBatteryState;
  bool lampState;
  bool decorativeLightState;
  bool liftedState;
  uint8_t tailLightState;
  bool fanState;
  bool brakeState;
  bool slowDownState;
  bool DFUState;
};

extern eucClass EUC;
