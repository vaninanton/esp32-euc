#include "EUC.h"

static const char* LOG_TAG = "EUC";

static const NimBLEUUID uartServiceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID ffe5ServiceUUID("FFE5");
static const NimBLEUUID ffe9CharUUID("FFE9");
static const NimBLEUUID ffe0ServiceCharUUID("FFE0");
static const NimBLEUUID ffe4CharUUID("FFE4");

class EUCFoundDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    if (advertisedDevice->getName() == "") {
      return;
    }

    if (advertisedDevice->getName() == "V11-9C07003B") {
      NimBLEDevice::getScan()->stop();
      ESP_LOGI(LOG_TAG, "[SCAN] Known BLE device: %s", advertisedDevice->getName().c_str());
      EUC.advertisedDevice = advertisedDevice;
      return;
    }

    ESP_LOGD(LOG_TAG, "[SCAN] Found BLE device: %s", advertisedDevice->getName().c_str());
  }
};

static void eucNotifyReceived(NimBLERemoteCharacteristic* eucCharacteristic, uint8_t* data, size_t dataLength, bool isNotify) {
  ESP_LOGD(LOG_TAG, "Notify received from EUC");
  EUC.appServer->getServiceByUUID(uartServiceUUID)->getCharacteristic(eucCharacteristic->getUUID())->notify(data, dataLength, true);
};

class EUCClientCallbacks : public NimBLEClientCallbacks {
 void onConnect(NimBLEClient* pClient) {
  ESP_LOGD(LOG_TAG, "EUC connected");
 }
 void onDisconnect(NimBLEClient* pClient) {
  ESP_LOGD(LOG_TAG, "EUC disconnected");
 }
};

class ProxyConnectCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) { ESP_LOGD(LOG_TAG, "App connected"); }
  void onDisconnect(NimBLEServer* pServer) { ESP_LOGD(LOG_TAG, "App disconnected"); }
};

/** @brief Проксирование сообщений от приложения к колесу */
class ProxyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    ESP_LOGD(LOG_TAG, "Notify received from app");
    if (EUC.eucClient->isConnected() == true) {
      // Write message from app to euc
      bool sent = EUC.eucClient->getService(uartServiceUUID)->getCharacteristic(pCharacteristic->getUUID())->writeValue(pCharacteristic->getValue().data(), pCharacteristic->getDataLength(), false);
    }
  }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    if (subValue > 0) {
      EUC.appSubscribed = true;
      ESP_LOGD(LOG_TAG, "App subscribed");
    } else {
      EUC.appSubscribed = false;
      ESP_LOGD(LOG_TAG, "App unsubscribed");
    }
  }
};

void eucClass::bleConnect() {
  if (EUC.appServer == nullptr) {
    ESP_LOGD(LOG_TAG, "Creating BLE server...");
    EUC.appServer = NimBLEDevice::createServer();
    EUC.appServer->setCallbacks(new ProxyConnectCallbacks());

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
    appTxCharacteristic->setCallbacks(new ProxyCallbacks());
    appRxCharacteristic->setCallbacks(new ProxyCallbacks());
    appFfe9Characteristic->setCallbacks(new ProxyCallbacks());
    appFfe4Characteristic->setCallbacks(new ProxyCallbacks());
    NimBLEDevice::startAdvertising();
    NimBLEDevice::stopAdvertising();
  } else if (EUC.advertisedDevice == nullptr) {
    ESP_LOGD(LOG_TAG, "Scanning for BLE device...");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new EUCFoundDeviceCallbacks(), false);
    pScan->start(5);
  } else if (EUC.eucClient == nullptr) {
    ESP_LOGD(LOG_TAG, "Creating BLE client...");
    EUC.eucClient = NimBLEDevice::createClient();
    EUC.eucClient->setClientCallbacks(new EUCClientCallbacks(), true);
  } else if (EUC.eucClient->isConnected() == false) {
    // Сбрасываем статус подписки на RX
    EUC.eucSubscribed = false;
    ESP_LOGD(LOG_TAG, "Connecting to BLE server...");
    EUC.eucClient->connect(EUC.advertisedDevice);
  } else if (EUC.eucClient->isConnected() == true && EUC.eucSubscribed == false) {
    ESP_LOGD(LOG_TAG, "Subscribing to RX characteristic...");
    NimBLERemoteService* eucService = EUC.eucClient->getService(uartServiceUUID);
    NimBLERemoteCharacteristic* eucTxCharacteristic = eucService->getCharacteristic(txCharUUID);
    NimBLERemoteCharacteristic* eucRxCharacteristic = eucService->getCharacteristic(rxCharUUID);
    eucRxCharacteristic->subscribe(true, eucNotifyReceived, false);
    ESP_LOGD(LOG_TAG, "Subscribed! Starting advertising...");
    NimBLEDevice::startAdvertising();
    EUC.eucSubscribed = true;
  } else if (EUC.eucSubscribed == true) {
    // ESP_LOGD(LOG_TAG, ".");
  }
}

void eucClass::debug() {
  ESP_LOGD(LOG_TAG, "Power: %d.%.2dV, (%d.%.2d%%)", (int)(busVoltage / 100), (int)(busVoltage % 100), (int)(batteryPercentage / 100), (int)(batteryPercentage % 100));
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
  ESP_LOGD(LOG_TAG, "lampState: %d", lampState);
  ESP_LOGD(LOG_TAG, "decorativeLightState: %d", decorativeLightState);
  ESP_LOGD(LOG_TAG, "liftedState: %d", liftedState);
  ESP_LOGD(LOG_TAG, "tailLightState: %d", tailLightState);  // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
  // ESP_LOGD(LOG_TAG, "fanState: %d", fanState);
  ESP_LOGD(LOG_TAG, "brakeState: %d", brakeState);
  ESP_LOGD(LOG_TAG, "slowDownState: %d", slowDownState);
  // ESP_LOGD(LOG_TAG, "DFUState: %d", DFUState);
}

eucClass EUC = eucClass();
