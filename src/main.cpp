#include <Arduino.h>
#include <NimBLEDevice.h>

// Пины
const int buttonPin = 0;
const int ledPin = 2;

// Список сервисов и характеристик
static NimBLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

static NimBLEClient *pClient;
static NimBLEAdvertisedDevice *myDevice;
static NimBLERemoteCharacteristic *rxCharacteristic;
static NimBLERemoteCharacteristic *txCharacteristic;

static boolean doScan = false;
static boolean doConnecting = false;
static boolean initialized = false;

static void notifyCallback(NimBLERemoteCharacteristic *pBLERemoteCharacteristic, byte *pData, size_t length, bool isNotify)
{
  Serial.print("[DEBUG] Notify received: "); Serial.println(pData[3], HEX);

  // Гасим светодиод, коллбек обработан
  digitalWrite(ledPin, LOW);
}

/** @brief Коллбеки для управления подключением к EUC */
class EucDeviceCallbacks : public NimBLEClientCallbacks
{
  void onConnect(NimBLEClient *pClient)
  {
    Serial.println("[DEBUG] Connected to EUC, initializing services...");

    NimBLERemoteService *pRemoteService = pClient->getService(serviceUUID);

    txCharacteristic = pRemoteService->getCharacteristic(txCharUUID);

    rxCharacteristic = pRemoteService->getCharacteristic(rxCharUUID);
    rxCharacteristic->subscribe(true, notifyCallback);

    initialized = true;
    Serial.println("[DEBUG]  EUC initialized!");
  }

  void onDisconnect(NimBLEClient *pClient)
  {
    Serial.println("[INFO] Disconnected from EUC");
    initialized = false;
    doConnecting = false;
    doScan = true;
  }
};

/** @brief A callback handler for callbacks associated device scanning. */
class EUCFoundDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
  /** @brief Called when a new scan result is detected. */
  void onResult(NimBLEAdvertisedDevice *advertisedDevice)
  {
    // Пропускаем устройства с пустым именем
    if (advertisedDevice->getName() == "")
      return;

    Serial.print("[DEBUG] Device found: ");
    Serial.println(advertisedDevice->getName().c_str());

    // Пропускаем устройства, которые не поддерживают нужные сервисы
    if (!advertisedDevice->isAdvertisingService(serviceUUID))
      return;

    // Нашли устройство, останавливаем сканирование
    doScan = false;
    NimBLEDevice::getScan()->stop();

    // Выводим название устройства
    myDevice = advertisedDevice;
    doConnecting = true;
    Serial.println("[INFO] Device selected, waiting for connect!");
  }
};

void startBLEScan()
{
  // Сбрасываем переменные
  initialized = false;

  NimBLEScan *pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new EUCFoundDeviceCallbacks());
  pScan->setInterval(100);
  pScan->setWindow(99);
  pScan->setActiveScan(true);
  pScan->start(15);
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("[INFO] Starting ESP32-EUC application...");

  NimBLEDevice::init("ESP32-EUC");
  pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(new EucDeviceCallbacks());

  // Запланировали сканирование
  doScan = true;
}

void loop()
{
  if (doScan == true)
    startBLEScan();
  else if (doConnecting == true)
  {
    doConnecting = false;

    Serial.println("[DEBUG] Connecting to selected device...");
    pClient->connect(myDevice, true);
    Serial.println("[DEBUG] Connected to selected device!");
  }
  else if (initialized == true)
  {
    digitalWrite(ledPin, HIGH);

    byte live[6] = {0xAA, 0xAA, 0x14, 0x01, 0x04, 0x11};
    // byte mute[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2C, 0x00, 0x5B};
    // byte drlOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2D, 0x01, 0x5B};
    // byte drlOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2D, 0x00, 0x5A};

    txCharacteristic->writeValue(live, sizeof(live));
    delay(1000);

    // txCharacteristic->writeValue(drlOn, sizeof(drlOn), false);
    // delay(100);
    // txCharacteristic->writeValue(drlOff, sizeof(drlOff), false);
    // delay(100);
  }

  // delay(1000);
}
