#include <Arduino.h>
#include <EUC.h>
#include <InmotionV2Message.h>
#include <NimBLEDevice.h>

// Пины
const int ledPin = 2;

#define MY_PERIOD 1000  // период в мс
uint32_t tmr1;          // переменная таймера

// Список сервисов и характеристик
static NimBLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

static NimBLEClient* bleClient;
static NimBLEAdvertisedDevice* selectedDevice;
static NimBLERemoteCharacteristic* rxCharacteristic;
static NimBLERemoteCharacteristic* txCharacteristic;

static InmotionV2Message* pMessage;

static boolean doScan = false;
static boolean doConnect = false;
static boolean doInit = false;
static boolean doWork = false;

/** @brief Коллбеки для управления подключением к EUC */
class EucDeviceCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* bleClient) {
    Serial.println("[BLE] Connected!");

    doScan = false;
    doConnect = false;
    doInit = true;
    doWork = false;
  }

  void onDisconnect(NimBLEClient* bleClient) {
    Serial.println("[BLE] Disconnected from EUC");
    doScan = true;
    doConnect = false;
    doInit = true;
    doWork = false;
  }
};

class EUCFoundDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  /** @brief Запускается при нахождении любого устройства */
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    // Пропускаем устройства с пустым именем
    if (advertisedDevice->getName() == "") {
      return;
    }

    Serial.printf("[BLE] 🔵 %s\n", advertisedDevice->toString().c_str());

    // Пропускаем устройства, которые не поддерживают нужные сервисы
    if (!advertisedDevice->isAdvertisingService(serviceUUID)) {
      return;
    }

    // Мы нашли подходящее устройство, останавливаем сканирование
    NimBLEDevice::getScan()->stop();

    selectedDevice = advertisedDevice;
    doScan = false;
    doConnect = true;
    doInit = false;
    doWork = false;

    // Выводим название устройства
    Serial.printf("[BLE] Device %s selected\n",
                  advertisedDevice->getName().c_str());
  }
};

/** @brief Запуск сканирования */
void startBLEScan() {
  Serial.println("[BLE] Scanning...");
  doScan = false;
  doConnect = false;
  doInit = false;
  doWork = false;

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new EUCFoundDeviceCallbacks());
  pScan->setInterval(100);
  pScan->setWindow(99);
  pScan->setActiveScan(true);
  pScan->start(15);
}

void connectToBleDevice() {
  Serial.println("[BLE] Connecting...");
  bleClient->connect(selectedDevice, true);

  doScan = false;
  doConnect = false;
  doInit = true;
  doWork = false;
}

/** @brief Коллбек при получении сообщения в характеристику RX */
static void rxReceivedCallback(
    NimBLERemoteCharacteristic* pBLERemoteCharacteristic,
    byte* pData,
    size_t length,
    bool isNotify) {
  pMessage->parse(pData, length);
  // После получения и парсинга сообщения гасим светодиод
  digitalWrite(ledPin, LOW);
}

void initBleDevice() {
  Serial.println("[BLE] Initializing services...");
  NimBLERemoteService* pRemoteService = bleClient->getService(serviceUUID);
  txCharacteristic = pRemoteService->getCharacteristic(txCharUUID);
  rxCharacteristic = pRemoteService->getCharacteristic(rxCharUUID);
  rxCharacteristic->subscribe(true, rxReceivedCallback);
  Serial.println("[BLE] Device services initialized successfully");

  doScan = false;
  doConnect = false;
  doInit = false;
  doWork = true;
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

void setup() {
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("[INFO] Starting ESP32-EUC application...");

  NimBLEDevice::init("ESP32-EUC");
  bleClient = NimBLEDevice::createClient();
  bleClient->setClientCallbacks(new EucDeviceCallbacks());

  pMessage = new InmotionV2Message();

  // Запланировали сканирование
  doScan = true;

  // Таймер для команд
  unsigned long start = millis();
}

void loop() {
  if (doScan == true) {
    startBLEScan();
  } else if (doConnect == true) {
    connectToBleDevice();
  } else if (doInit == true) {
    initBleDevice();
  } else if (doWork == true) {
    if (millis() - tmr1 >= MY_PERIOD) {
      tmr1 = millis();

      // Перед отправкой сообщения включаем светодиод на плате
      digitalWrite(ledPin, HIGH);
      Serial.println("[EUC] Sending live packet...");
      byte live[6] = {0xAA, 0xAA, 0x14, 0x01, 0x04, 0x11};
      txCharacteristic->writeValue(live, sizeof(live), false);
    }

  } else {
    Serial.println("[DEBUG] Empty loop");
    delay(1000);
  }
}
