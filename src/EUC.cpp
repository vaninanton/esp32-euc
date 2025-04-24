#include "EUC.h"

static const char* LOG_TAG = "ESP32-EUC";
static InmotionV2Unpacker* unpacker = new InmotionV2Unpacker();
static InmotionV2Message* pMessage = new InmotionV2Message();

class EUCFoundDeviceCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    EUC.onScanResult(advertisedDevice);
  }
} scanCallbacks;

class EucConnectCallbacks : public NimBLEClientCallbacks {
  /**
   * @brief Called after client connects.
   * @param [in] pClient A pointer to the connecting client object.
   */
  void onConnect(NimBLEClient* pClient) override {
    ESP_LOGI(LOG_TAG, "Connected to %s",
             EUC.advertisedDevice->getName().c_str());
  }

  /**
   * @brief Called when a connection attempt fails.
   * @param [in] pClient A pointer to the connecting client object.
   * @param [in] reason Contains the reason code for the connection failure.
   */
  void onConnectFail(NimBLEClient* pClient, int reason) override {
    ESP_LOGW(LOG_TAG, "Failed to connect to %s, reason: %d",
             EUC.advertisedDevice->getName().c_str(), reason);
  };

  /**
   * @brief Called when disconnected from the server.
   * @param [in] pClient A pointer to the calling client object.
   * @param [in] reason Contains the reason code for the disconnection.
   */
  void onDisconnect(NimBLEClient* pClient, int reason) override {
    ESP_LOGI(LOG_TAG, "[%d] Disconnected from %s", reason,
             EUC.advertisedDevice->getName().c_str());

    EUC.advertisedDevice = nullptr;
    EUC.failedScanCount = 0;
  }
  // TODO onConnParamsUpdateRequest
  // TODO onIdentity
  // TODO onMTUChange
} clientCallbacks;

class AppServerCallbacks : public NimBLEServerCallbacks {
  /**
   * @brief Handle a client connection.
   * This is called when a client connects.
   * @param [in] pServer A pointer to the BLE server that received the client
   * connection.
   * @param [in] connInfo A reference to a NimBLEConnInfo instance with
   * information. about the peer connection parameters.
   */
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    ESP_LOGD(LOG_TAG, "App connected to proxy");
  }

  /**
   * @brief Handle a client disconnection.
   * This is called when a client disconnects.
   * @param [in] pServer A pointer to the BLE server that received the client
   * disconnection.
   * @param [in] connInfo A reference to a NimBLEConnInfo instance with
   * information about the peer connection parameters.
   * @param [in] reason The reason code for the disconnection.
   */
  void onDisconnect(NimBLEServer* pServer,
                    NimBLEConnInfo& connInfo,
                    int reason) override {
    ESP_LOGD(LOG_TAG, "App disconnected from proxy");
  }

  /**
   * @brief Called when the connection MTU changes.
   * @param [in] MTU The new MTU value.
   * @param [in] connInfo A reference to a NimBLEConnInfo instance with
   * information about the peer connection parameters.
   */
  void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
    ESP_LOGD(LOG_TAG, "App MTU changed to: %d", MTU);
  }
} appServerCallbacks;

class AppCallbacks : public NimBLECharacteristicCallbacks {
  /**
   * @brief Callback function to support a write request.
   * @param [in] pCharacteristic The characteristic that is the source of the
   * event.
   * @param [in] connInfo A reference to a NimBLEConnInfo instance containing
   * the peer info.
   */
  void onWrite(NimBLECharacteristic* pCharacteristic,
               NimBLEConnInfo& connInfo) override {
    ESP_LOGD(LOG_TAG, "Notify received from app");
    if (EUC.eucBleClient != nullptr &&
        EUC.eucBleClient->isConnected() == true) {
      bool writeValueResult =
          EUC.eucBleClient->getService(uartServiceUUID)
              ->getCharacteristic(pCharacteristic->getUUID())
              ->writeValue(pCharacteristic->getValue().data(),
                           pCharacteristic->getLength(), false);
      if (writeValueResult == false) {
        ESP_LOGE(LOG_TAG, "Failed to send message to EUC");
      }
    }
  }

  /**
   * @brief Callback function called when a client changes subscription status.
   * @param [in] pCharacteristic The characteristic that is the source of the
   * event.
   * @param [in] connInfo A reference to a NimBLEConnInfo instance containing
   * the peer info.
   * @param [in] subValue The subscription status:
   * * 0 = Un-Subscribed
   * * 1 = Notifications
   * * 2 = Indications
   * * 3 = Notifications and Indications
   */
  void onSubscribe(NimBLECharacteristic* pCharacteristic,
                   NimBLEConnInfo& connInfo,
                   uint16_t subValue) override {
    if (subValue == 0) {
      ESP_LOGD(LOG_TAG, "App unsubscribed from ESP");
      EUC.appSubscribed = false;
    } else if (subValue == 1) {
      ESP_LOGD(LOG_TAG, "App subscribed to ESP notifications");
      EUC.appSubscribed = true;
    } else if (subValue == 2) {
      ESP_LOGD(LOG_TAG, "App subscribed to ESP indications");
      EUC.appSubscribed = true;
    } else if (subValue == 3) {
      ESP_LOGD(LOG_TAG, "App subscribed to ESP notifications and indications");
      EUC.appSubscribed = true;
    }
  }
} appChrCallbacks;

static void eucNotifyReceived(
    NimBLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {
  ESP_LOGD(LOG_TAG, "Notify received from EUC");
  for (size_t i = 0; i < length; i++) {
    if (unpacker->addChar(pData[i]) == true) {
      uint8_t* buffer = unpacker->getBuffer();
      size_t bufferLength = unpacker->getBufferIndex();
      pMessage->parse(buffer, bufferLength);
    }
  }

  // EUC.appBleServer->getServiceByUUID(pBLERemoteCharacteristic->getRemoteService()->getUUID())->getCharacteristic(pBLERemoteCharacteristic->getUUID())->notify(pData,
  // length, true);
  bool notifyResult =
      EUC.appBleServer->getServiceByUUID(uartServiceUUID)
          ->getCharacteristic(pBLERemoteCharacteristic->getUUID())
          ->notify(pData, length, BLE_HS_CONN_HANDLE_NONE);
  if (notifyResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to send notification to app");
  }
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

void eucClass::setup() {
  bool initializeResult = NimBLEDevice::init("V11-ESP32EUC");
  if (initializeResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to initialize NimBLE");
    // TODO THROW ERROR
  }
}

void eucClass::createAppBleServer() {
  // exit when appBleServer already initialized
  if (EUC.appBleServer != nullptr)
    return;

  ESP_LOGD(LOG_TAG, "Creating BLE server for app...");
  EUC.appBleServer = NimBLEDevice::createServer();
  EUC.appBleServer->setCallbacks(&appServerCallbacks, true);

  NimBLEService* appService = EUC.appBleServer->createService(uartServiceUUID);
  NimBLEService* appFfe5 = EUC.appBleServer->createService(ffe5ServiceUUID);
  NimBLEService* appFfe0 = EUC.appBleServer->createService(ffe0ServiceCharUUID);
  NimBLECharacteristic* appTxCharacteristic = appService->createCharacteristic(
      txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* appRxCharacteristic =
      appService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* appFfe9Characteristic = appFfe5->createCharacteristic(
      ffe9CharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* appFfe4Characteristic =
      appFfe0->createCharacteristic(ffe4CharUUID, NIMBLE_PROPERTY::NOTIFY);

  ESP_LOGD(LOG_TAG, "Starting BLE server for app...");
  bool appServiceStartedResult = appService->start();
  appTxCharacteristic->setCallbacks(&appChrCallbacks);
  appRxCharacteristic->setCallbacks(&appChrCallbacks);
  appFfe9Characteristic->setCallbacks(&appChrCallbacks);
  appFfe4Characteristic->setCallbacks(&appChrCallbacks);

  if (appServiceStartedResult == false) {
    ESP_LOGE(LOG_TAG, "BLE server for app failed to start, trying to recreate");
    return;
  }

  ESP_LOGD(LOG_TAG, "BLE server for app started successfully");

  // bool serviceFfe5Started = appFfe5->start();
  // if (serviceFfe5Started == false) {
  //   ESP_LOGE(LOG_TAG, "Failed to start service");
  //   return;
  // }
  // ESP_LOGD(LOG_TAG, "appFfe5 service started");

  // bool serviceFfe0Started = appFfe0->start();
  // if (serviceFfe0Started == false) {
  //   ESP_LOGE(LOG_TAG, "Failed to start service");
  //   return;
  // }
  // ESP_LOGD(LOG_TAG, "appFfe0 service started");

  ESP_LOGD(LOG_TAG, "Creating advertising for app server...");

  bool setNameResult = NimBLEDevice::getAdvertising()->setName("V11-ESP32EUC");
  if (setNameResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to set name to advertising");
    return;
  }
  ESP_LOGD(LOG_TAG, "Name setted to advertising successfully");

  bool addServiceUUIDResult =
      NimBLEDevice::getAdvertising()->addServiceUUID(uartServiceUUID);
  if (addServiceUUIDResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to add service to advertising");
    return;
  }
  ESP_LOGD(LOG_TAG, "Service added to advertising");

  bool startAdvertisingResult = NimBLEDevice::startAdvertising();
  if (startAdvertisingResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to start tmp advertising");
  }

  // bool advertisingStopped = NimBLEDevice::stopAdvertising();
  // if (advertisingStopped == false) {
  //   ESP_LOGE(LOG_TAG, "Failed to stop tmp advertising");
  // }
}

void eucClass::createEucBleClient() {
  if (EUC.eucBleClient != nullptr)
    return;

  ESP_LOGI(LOG_TAG, "Creating BLE client for EUC...");
  EUC.eucBleClient = NimBLEDevice::createClient();
  EUC.eucBleClient->setClientCallbacks(&clientCallbacks, true);
}

void eucClass::startBleScan() {
  if (EUC.advertisedDevice != nullptr)
    return;
  // if (EUC.failedScanCount > 5)
  // return;
  // if (EUC.latestScan > 0 && millis() - EUC.latestScan < 10000)
  //   return;

  //   ESP_LOGD(LOG_TAG, "Scanning for BLE devices...");

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(&scanCallbacks, false);
  bool startResult = pScan->start(3000);
  if (startResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to start scan");
  }
  // EUC.latestScan = millis();
  // EUC.failedScanCount++;
}

void eucClass::onScanResult(const NimBLEAdvertisedDevice* advertisedDevice) {
  if (advertisedDevice->getName() != "V11-9C07003B")
    return;

  ESP_LOGI(LOG_TAG, "[SCAN] Found known BLE device: %s",
           advertisedDevice->getName().c_str());

  EUC.advertisedDevice = advertisedDevice;
  // EUC.failedScanCount = 0;

  bool stopResult = NimBLEDevice::getScan()->stop();
  if (stopResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to stop scan");
  }
}

void eucClass::onEucDisconnected(NimBLEClient* pClient, int reason) {
  ESP_LOGI(LOG_TAG, "[%d] Disconnected from %s", reason,
           EUC.advertisedDevice->getName().c_str());

  EUC.advertisedDevice = nullptr;
  EUC.failedScanCount = 0;
}

void eucClass::connectToEuc() {
  if (EUC.eucBleClient == nullptr || EUC.advertisedDevice == nullptr ||
      EUC.eucBleClient->isConnected())
    return;

  ESP_LOGI(LOG_TAG, "Connecting to EUC...");

  bool connectResult = EUC.eucBleClient->connect(EUC.advertisedDevice);
  if (connectResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to connect to EUC");
    EUC.advertisedDevice = nullptr;
    return;
  }

  subscribeToEucCharacteristics();
}

void eucClass::subscribeToEucCharacteristics() {
  if (EUC.eucBleClient == nullptr || EUC.eucBleClient->isConnected() == false)
    return;

  ESP_LOGI(LOG_TAG, "Subscribing to EUC characteristics...");
  delay(3000);

  NimBLERemoteService* eucService =
      EUC.eucBleClient->getService(uartServiceUUID);
  if (eucService == nullptr) {
    ESP_LOGE(LOG_TAG, "eucService equals nullptr");
    return;
  }
  NimBLERemoteCharacteristic* eucTxCharacteristic =
      eucService->getCharacteristic(txCharUUID);
  NimBLERemoteCharacteristic* eucRxCharacteristic =
      eucService->getCharacteristic(rxCharUUID);
  bool subscribeResult =
      eucRxCharacteristic->subscribe(true, eucNotifyReceived, false) == false;
  if (subscribeResult == false) {
    ESP_LOGE(LOG_TAG, "Failed to subscribe to EUC RX characteristic");
    // TODO
    return;
  }

  ESP_LOGI(LOG_TAG, "Subscribed to EUC characteristics!");
  if (NimBLEDevice::startAdvertising() == false) {
    ESP_LOGE(LOG_TAG, "Failed to start advertising");
    return;
  }
  ESP_LOGD(LOG_TAG, "Advertising started");
}

void eucClass::tick() {
  createAppBleServer();
  createEucBleClient();
  startBleScan();
  connectToEuc();
}

void eucClass::debug() {
  ESP_LOGD(LOG_TAG, "Power: %d.%.2dV, (%d.%.2d%%)", (int)(busVoltage / 100),
           (int)(busVoltage % 100), (int)(batteryPercentage / 100),
           (int)(batteryPercentage % 100));
  // ESP_LOGD(LOG_TAG, "busCurrent: %d", busCurrent);
  ESP_LOGD(LOG_TAG, "speed: %d", speed);
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
  // ESP_LOGD(LOG_TAG, "batteryPercentageForRide: %d",
  // batteryPercentageForRide); ESP_LOGD(LOG_TAG, "estimatedTotalMileage: %d",
  // estimatedTotalMileage); ESP_LOGD(LOG_TAG, "realtimeSpeedLimit: %d",
  // realtimeSpeedLimit); ESP_LOGD(LOG_TAG, "realtimeCurrentLimit: %d",
  // realtimeCurrentLimit); ESP_LOGD(LOG_TAG, "reserve38: %d", reserve38);
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
  // ESP_LOGD(LOG_TAG, "HMICRunMode: %d", HMICRunMode); // lock, drive,
  // shutdown, idle ESP_LOGD(LOG_TAG, "MCRunMode: %d", MCRunMode);
  // ESP_LOGD(LOG_TAG, "motorState: %d", motorState);
  // ESP_LOGD(LOG_TAG, "chargeState: %d", chargeState);
  // ESP_LOGD(LOG_TAG, "backupBatteryState: %d", backupBatteryState);
  ESP_LOGD(LOG_TAG, "lampState: %d", lampState);
  ESP_LOGD(LOG_TAG, "decorativeLightState: %d", decorativeLightState);
  ESP_LOGD(LOG_TAG, "liftedState: %d", liftedState);
  ESP_LOGD(LOG_TAG, "tailLightState: %d",
           tailLightState);  // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT,
                             // 3 - BLINKING
  // ESP_LOGD(LOG_TAG, "fanState: %d", fanState);
  ESP_LOGD(LOG_TAG, "brakeState: %d", brakeState);
  ESP_LOGD(LOG_TAG, "slowDownState: %d", slowDownState);
  // ESP_LOGD(LOG_TAG, "DFUState: %d", DFUState);
}

eucClass EUC = eucClass();
