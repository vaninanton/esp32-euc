#include "EUC.h"

static const char* LOG_TAG = "ESP32-EUC";
static InmotionV2Unpacker* unpacker = new InmotionV2Unpacker();
static InmotionV2Message* pMessage = new InmotionV2Message();

class EUCFoundDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) { EUC.onScanResult(advertisedDevice); }
};

static void eucNotifyReceived(NimBLERemoteCharacteristic* pChar, uint8_t* data, size_t length, bool isNotify) {
  EUC.onEucNotifyReceived(pChar, data, length, isNotify);
};

class EucConnectCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) { EUC.onEucConnected(pClient); }
  void onDisconnect(NimBLEClient* pClient) { EUC.onEucDisconnected(pClient); }
};

class AppConnectCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) { EUC.onAppConnected(pServer, desc); }
  void onDisconnect(NimBLEServer* pServer) { EUC.onAppDisconnected(pServer); }
};

class AppCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) { EUC.onAppWrite(pChar); }

  void onSubscribe(NimBLECharacteristic* pChar, ble_gap_conn_desc* desc, uint16_t subValue) {
    (subValue > 0) ? EUC.onAppSubscribe(pChar, desc) : EUC.onAppUnsubscribe(pChar, desc);
  }
};

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

void eucClass::setup() {
  NimBLEDevice::init("V11-ESP32EUC");
}

void eucClass::createAppBleServer() {
  if (EUC.appServer != nullptr)
    return;

  ESP_LOGD(LOG_TAG, "Creating BLE server...");
  EUC.appServer = NimBLEDevice::createServer();
  EUC.appServer->setCallbacks(new AppConnectCallbacks());

  NimBLEService* appService = EUC.appServer->createService(uartServiceUUID);
  NimBLEService* appFfe5 = EUC.appServer->createService(ffe5ServiceUUID);
  NimBLEService* appFfe0 = EUC.appServer->createService(ffe0ServiceCharUUID);
  NimBLECharacteristic* appTxCharacteristic = appService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* appRxCharacteristic = appService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* appFfe9Characteristic = appFfe5->createCharacteristic(ffe9CharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* appFfe4Characteristic = appFfe0->createCharacteristic(ffe4CharUUID, NIMBLE_PROPERTY::NOTIFY);
  appService->start();

  NimBLEAdvertising* appAdvertising = NimBLEDevice::getAdvertising();
  appAdvertising->addServiceUUID(uartServiceUUID);
  appTxCharacteristic->setCallbacks(new AppCallbacks());
  appRxCharacteristic->setCallbacks(new AppCallbacks());
  appFfe9Characteristic->setCallbacks(new AppCallbacks());
  appFfe4Characteristic->setCallbacks(new AppCallbacks());
  NimBLEDevice::startAdvertising();
  NimBLEDevice::stopAdvertising();
}
void eucClass::createEucBleClient() {
  if (EUC.eucClient != nullptr)
    return;

  ESP_LOGD(LOG_TAG, "Creating BLE client...");
  EUC.eucClient = NimBLEDevice::createClient();
  EUC.eucClient->setClientCallbacks(new EucConnectCallbacks(), true);
}
void eucClass::startBleScan() {
  if (EUC.advertisedDevice != nullptr)
    return;
  if (EUC.failedScanCount > 5)
    return;

  if (EUC.latestScan > 0 && millis() - EUC.latestScan < 10000)
    return;

  LED.scanStart();
  ESP_LOGD(LOG_TAG, "Scanning for BLE device...");
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new EUCFoundDeviceCallbacks(), false);
  pScan->start(3);
  LED.scanStop();
  ESP_LOGD(LOG_TAG, "Stop scanning for BLE device...");
  EUC.latestScan = millis();
  EUC.failedScanCount++;
}
void eucClass::onScanResult(NimBLEAdvertisedDevice* advertisedDevice) {
  if (advertisedDevice->getName() != "V11-9C07003B")
    return;

  ESP_LOGI(LOG_TAG, "[SCAN] Found BLE device: %s", advertisedDevice->getName().c_str());
  NimBLEDevice::getScan()->stop();
  EUC.advertisedDevice = advertisedDevice;
}
void eucClass::onEucConnected(NimBLEClient* pClient) {
  ESP_LOGD(LOG_TAG, "EUC connected");
  EUC.failedScanCount = 0;
}
void eucClass::onEucDisconnected(NimBLEClient* pClient) {
  ESP_LOGD(LOG_TAG, "EUC disconnected");
  EUC.advertisedDevice = nullptr;
  EUC.eucSubscribed = false;
  EUC.failedScanCount = 0;
}
void eucClass::connectToEuc() {
  if (EUC.eucClient == nullptr)
    return;
  if (EUC.eucClient->isConnected())
    return;
  if (EUC.advertisedDevice == nullptr)
    return;

  ESP_LOGD(LOG_TAG, "Connecting to EUC...");
  // Сбрасываем статус подписки на RX
  EUC.eucSubscribed = false;
  EUC.failedScanCount = 0;
  EUC.eucClient->connect(EUC.advertisedDevice);
}
void eucClass::subscribeToEuc() {
  if (!EUC.eucClient->isConnected() || EUC.eucSubscribed)
    return;

  EUC.failedScanCount =/**
   * @file EUC.cpp
   * @brief Файл, содержащий реализацию класса [eucClass](cci:2://file:///Users/vanton/PlatformIO/Projects/esp32-euc/include/EUC.h:15:0-94:1) и связанные с ним функции и классы.
   */

  #include "EUC.h"

  // Статический экземпляр класса [eucClass](cci:2://file:///Users/vanton/PlatformIO/Projects/esp32-euc/include/EUC.h:15:0-94:1)
  eucClass EUC;

  /**
   * @brief Инициализирует сервер BLE.
   *
   * Эта функция создает сервер BLE и устанавливает обработчики событий подключения и отключения клиентов.
   */
  void eucClass::createAppBleServer() {
    if (EUC.appServer != nullptr)
      return;

    ESP_LOGD(LOG_TAG, "Creating BLE server...");
    EUC.appServer = NimBLEDevice::createServer();
    EUC.appServer->setCallbacks(new AppConnectCallbacks());

    NimBLEService* appService = EUC.appServer->createService(uartServiceUUID);
    NimBLEService* appFfe5 = EUC.appServer->createService(ffe5ServiceUUID);
    NimBLEService* appFfe0 = EUC.appServer->createService(ffe0ServiceCharUUID);
    NimBLECharacteristic* appTxCharacteristic = appService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    NimBLECharacteristic* appRxCharacteristic = appService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* appFfe9Characteristic = appFfe5->createCharacteristic(ffe9CharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    NimBLECharacteristic* appFfe4Characteristic = appFfe0->createCharacteristic(ffe4CharUUID, NIMBLE_PROPERTY::NOTIFY);
    appService->start();

    NimBLEAdvertising* appAdvertising = NimBLEDevice::getAdvertising();
    appAdvertising->addServiceUUID(uartServiceUUID);
    appTxCharacteristic->setCallbacks(new AppCallbacks());
    appRxCharacteristic->setCallbacks(new AppCallbacks());
    appFfe9Characteristic->setCallbacks(new AppCallbacks());
    appFfe4Characteristic->setCallbacks(new AppCallbacks());
    NimBLEDevice::startAdvertising();
    NimBLEDevice::stopAdvertising();
  }

  /**
   * @brief Создает клиент BLE для подключения к устройству EUC.
   *
   * Эта функция создает клиент BLE и устанавливает обработчик событий подключения и отключения.
   */
  void eucClass::createEucBleClient() {
    if (EUC.eucClient != nullptr)
      return;

    ESP_LOGD(LOG_TAG, "Creating BLE client...");
    EUC.eucClient = NimBLEDevice::createClient();
    EUC.eucClient->setClientCallbacks(new EucConnectCallbacks(), true);
  }

  /**
   * @brief Подключается к устройству EUC.
   *
   * Эта функция пытается подключиться к устройству EUC, используя найденное ранее устройство с именем "V11-9C07003B".
   */
  void eucClass::connectToEuc() {
    if (EUC.eucClient == nullptr)
      return;
    if (EUC.eucClient->isConnected())
      return;
    if (EUC.advertisedDevice == nullptr)
      return;

    ESP_LOGD(LOG_TAG, "Connecting to EUC...");
    // Сбрасываем статус подписки на RX
    EUC.eucSub 0;
  ESP_LOGD(LOG_TAG, "Subscribing to EUC characteristics...");
  NimBLERemoteService* eucService = EUC.eucClient->getService(uartServiceUUID);
  NimBLERemoteCharacteristic* eucRxCharacteristic = eucService->getCharacteristic(rxCharUUID);
  eucRxCharacteristic->subscribe(true, eucNotifyReceived, false);
  ESP_LOGD(LOG_TAG, "Subscribed! Starting advertising...");
  NimBLEDevice::startAdvertising();
  EUC.eucSubscribed = true;
}
void eucClass::onEucNotifyReceived(NimBLERemoteCharacteristic* eucCharacteristic, uint8_t* data, size_t dataLength, bool isNotify) {
  // ESP_LOGD(LOG_TAG, "Notify received from EUC");
  for (size_t i = 0; i < dataLength; i++) {
    if (unpacker->addChar(data[i]) == true) {
      uint8_t* buffer = unpacker->getBuffer();
      size_t bufferLength = unpacker->getBufferIndex();
      pMessage->parse(buffer, bufferLength);
    }
  }

  // EUC.appServer->getServiceByUUID(eucCharacteristic->getRemoteService()->getUUID())->getCharacteristic(eucCharacteristic->getUUID())->notify(data, dataLength, true);
  EUC.appServer->getServiceByUUID(uartServiceUUID)->getCharacteristic(eucCharacteristic->getUUID())->notify(data, dataLength, true);
}
void eucClass::onAppConnected(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
  ESP_LOGD(LOG_TAG, "App connected");
}
void eucClass::onAppDisconnected(NimBLEServer* pServer) {
  ESP_LOGD(LOG_TAG, "App disconnected");
}
void eucClass::onAppSubscribe(NimBLECharacteristic* pChar, ble_gap_conn_desc* desc) {
  ESP_LOGD(LOG_TAG, "App subscribed");
  EUC.appSubscribed = true;
}
void eucClass::onAppUnsubscribe(NimBLECharacteristic* pChar, ble_gap_conn_desc* desc) {
  ESP_LOGD(LOG_TAG, "App unsubscribed");
  EUC.appSubscribed = false;
}
void eucClass::onAppWrite(NimBLECharacteristic* pChar) {
  // ESP_LOGD(LOG_TAG, "Notify received from app");
  if (EUC.eucClient->isConnected() == true) {
    // Write message from app to euc
    bool sent = EUC.eucClient->getService(uartServiceUUID)->getCharacteristic(pChar->getUUID())->writeValue(pChar->getValue().data(), pChar->getDataLength(), false);
  }
}

void eucClass::tick() {
  createAppBleServer();
  createEucBleClient();
  startBleScan();
  connectToEuc();
  subscribeToEuc();
}

void eucClass::debug() {
  // ESP_LOGD(LOG_TAG, "Power: %d.%.2dV, (%d.%.2d%%)", (int)(busVoltage / 100), (int)(busVoltage % 100), (int)(batteryPercentage / 100), (int)(batteryPercentage % 100));
  // ESP_LOGD(LOG_TAG, "busCurrent: %d", busCurrent);
  // ESP_LOGD(LOG_TAG, "speed: %d", speed);
  // ESP_LOGD(LOG_TAG, "torque: %d", torque);
  // ESP_LOGD(LOG_TAG, "outputRate: %d", outputRate);
  // ESP_LOGD(LOG_TAG, "batteryOutputPower: %d", batteryOutputPower);
  // ESP_LOGD(LOG_TAG, "motorOutputPower: %d", motorOutputPower);
  // ESP_LOGD(LOG_TAG, "reserve14: %d", reserve14);
  // ESP_LOGD(LOG_TAG, "pitchAngle: %d", pitchAngle);
  // ESP_LOGD(LOG_TAG, "rollAngle: %d", rollAngle);
  // ESP_LOGD(LOG_TAG, "aimPitchAngle: %d", aimPitchAngle);
  // ESP_LOGD(LOG_TAG, "speedingBrakingAngle: %d", speedingBrakingAngle);
  // ESP_LOGD(LOG_TAG, "mileage: %d", mileage);
  // ESP_LOGD(LOG_TAG, "reserve26: %d", reserve26);
  // ESP_LOGD(LOG_TAG, "batteryPercentage: %d", batteryPercentage);
  // ESP_LOGD(LOG_TAG, "batteryPercentageForRide: %d", batteryPercentageForRide);
  // ESP_LOGD(LOG_TAG, "estimatedTotalMileage: %d", estimatedTotalMileage);
  // ESP_LOGD(LOG_TAG, "realtimeSpeedLimit: %d", realtimeSpeedLimit);
  // ESP_LOGD(LOG_TAG, "realtimeCurrentLimit: %d", realtimeCurrentLimit);
  // ESP_LOGD(LOG_TAG, "reserve38: %d", reserve38);
  // ESP_LOGD(LOG_TAG, "reserve40: %d", reserve40);
  // ESP_LOGD(LOG_TAG, "mosTemperature: %d", mosTemperature);
  // ESP_LOGD(LOG_TAG, "motorTemperature: %d", motorTemperature);
  // ESP_LOGD(LOG_TAG, "batteryTemperature: %d", batteryTemperature);
  // ESP_LOGD(LOG_TAG, "boardTemperature: %d", boardTemperature);
  // ESP_LOGD(LOG_TAG, "cpuTemperature: %d", cpuTemperature);
  // ESP_LOGD(LOG_TAG, "imuTemperature: %d", imuTemperature);
  // ESP_LOGD(LOG_TAG, "lampTemperature: %d", lampTemperature);
  // ESP_LOGD(LOG_TAG, "envBrightness: %d", envBrightness);
  // ESP_LOGD(LOG_TAG, "lampBrightness: %d", lampBrightness);
  // ESP_LOGD(LOG_TAG, "reserve51: %d", reserve51);
  // ESP_LOGD(LOG_TAG, "reserve52: %d", reserve52);
  // ESP_LOGD(LOG_TAG, "reserve53: %d", reserve53);
  // ESP_LOGD(LOG_TAG, "reserve54: %d", reserve54);
  // ESP_LOGD(LOG_TAG, "reserve55: %d", reserve55);
  // ESP_LOGD(LOG_TAG, "HMICRunMode: %d", HMICRunMode); // lock, drive, shutdown, idle
  // ESP_LOGD(LOG_TAG, "MCRunMode: %d", MCRunMode);
  // ESP_LOGD(LOG_TAG, "motorState: %d", motorState);
  // ESP_LOGD(LOG_TAG, "chargeState: %d", chargeState);
  // ESP_LOGD(LOG_TAG, "backupBatteryState: %d", backupBatteryState);
  // ESP_LOGD(LOG_TAG, "lampState: %d", lampState);
  // ESP_LOGD(LOG_TAG, "decorativeLightState: %d", decorativeLightState);
  // ESP_LOGD(LOG_TAG, "liftedState: %d", liftedState);
  ESP_LOGD(LOG_TAG, "tailLightState: %d", tailLightState);  // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
  // ESP_LOGD(LOG_TAG, "fanState: %d", fanState);
  // ESP_LOGD(LOG_TAG, "brakeState: %d", brakeState);
  // ESP_LOGD(LOG_TAG, "slowDownState: %d", slowDownState);
  // ESP_LOGD(LOG_TAG, "DFUState: %d", DFUState);
}

eucClass EUC = eucClass();
