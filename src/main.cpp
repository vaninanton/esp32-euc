#define FASTLED_INTERNAL

#include <Arduino.h>
#include <EUC.h>
#include <FastLED.h>
#include <InmotionV2Message.h>
#include <NimBLEDevice.h>

// Пины
const int ledPin = 2;

// FastLED
#define DATA_PIN 12
#define LED_TYPE WS2812B
#define NUM_LEDS 12
CRGB leds[NUM_LEDS];
uint8_t brightness = 10;

// Список сервисов и характеристик
static const NimBLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID unknownService1UUID("FFE5");
static const NimBLEUUID unknownService1CharUUID("FFE9");
static const NimBLEUUID unknownService2UUID("FFE0");
static const NimBLEUUID unknownService2CharUUID("FFE4");

static NimBLEClient* bleClient;
static NimBLEServer* bleServer;

static NimBLERemoteService* pRemoteService;
static NimBLERemoteCharacteristic* rxCharacteristic;
static NimBLERemoteCharacteristic* txCharacteristic;

InmotionUnpackerV2* unpacker = new InmotionUnpackerV2();
InmotionV2Message* pMessage = new InmotionV2Message();

static boolean doScan = false;
static boolean doConnect = false;
static boolean doInit = false;
static boolean doWork = false;
static boolean proxyStarted = false;
static boolean doInitProxy = false;
static boolean subscribed = false;

/**
 * @brief Проксирование сообщений от приложения к колесу
 */
class ProxyCallbacks : public NimBLECharacteristicCallbacks {
  // onRead
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    digitalWrite(ledPin, HIGH);
    if (bleClient->isConnected() && doWork == true) {
      Serial.print(">");
      bleClient->getService(serviceUUID)->getCharacteristic(pCharacteristic->getUUID())->writeValue(pCharacteristic->getValue().data(), pCharacteristic->getDataLength(), false);
    }
    digitalWrite(ledPin, LOW);
  }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    if (subValue == 0) {
      Serial.println("Unsubscribed");
      subscribed = false;
    } else if (subValue == 1) {
      Serial.println("Subscribed to notfications");
      subscribed = true;
    } else if (subValue == 2) {
      Serial.println("Subscribed to indications");
      subscribed = true;
    } else if (subValue == 3) {
      Serial.println("Subscribed to notifications and indications");
      subscribed = true;
    }
  }
};

/** @brief Коллбек при получении ответа от колеса */
static void eucMessageReceived(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  digitalWrite(ledPin, HIGH);
  Serial.print("<");
  for (size_t i = 0; i < length; i++) {
    if (unpacker->addChar(pData[i]) == true) {
      uint8_t* buffer = unpacker->getBuffer();
      size_t bufferIndex = unpacker->getBufferIndex();

      pMessage->parse(buffer, bufferIndex);
    }
  }

  if (subscribed == false) {
    digitalWrite(ledPin, LOW);
    return;
  }

  if (bleServer->getConnectedCount() == 0) {
    digitalWrite(ledPin, LOW);
    return;
  }

  NimBLEService* pSvc = bleServer->getServiceByUUID(serviceUUID);

  if (pSvc == nullptr) {
    digitalWrite(ledPin, LOW);
    return;
  }

  NimBLECharacteristic* pChr = pSvc->getCharacteristic(rxCharUUID);

  if (pChr == nullptr) {
    digitalWrite(ledPin, LOW);
    return;
  }

  pChr->notify(pData, length, true);
  digitalWrite(ledPin, LOW);
}

class EUCFoundDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    if (advertisedDevice->getName() == "" || !advertisedDevice->isAdvertisingService(serviceUUID))
      return;

    // Мы нашли подходящее устройство, останавливаем сканирование
    NimBLEDevice::getScan()->stop();

    EUC.bleDevice = advertisedDevice;
    Serial.printf("[BLE] %s\n", EUC.bleDevice->getName().c_str());
    doConnect = true;
  }
};

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

void setup() {
  pinMode(ledPin, OUTPUT);

  FastLED.addLeds<LED_TYPE, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(20);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 100);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.begin(115200);
  Serial.println();
  Serial.println("[INFO] Starting ESP32-EUC application...");

  NimBLEDevice::init("V11-ESP32EUC");
  bleClient = NimBLEDevice::createClient();
  bleServer = NimBLEDevice::createServer();

  NimBLEService* sService = bleServer->createService(serviceUUID);
  NimBLEService* ffe5 = bleServer->createService(NimBLEUUID("FFE5"));
  NimBLEService* ffe0 = bleServer->createService(NimBLEUUID("FFE0"));

  NimBLECharacteristic* sTxCharacteristic = sService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* sRxCharacteristic = sService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* ffe9Characteristic = ffe5->createCharacteristic(NimBLEUUID("FFE9"), NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* ffe4Characteristic = ffe0->createCharacteristic(NimBLEUUID("FFE4"), NIMBLE_PROPERTY::NOTIFY);

  sTxCharacteristic->setCallbacks(new ProxyCallbacks());
  sRxCharacteristic->setCallbacks(new ProxyCallbacks());
  ffe9Characteristic->setCallbacks(new ProxyCallbacks());
  ffe4Characteristic->setCallbacks(new ProxyCallbacks());

  bool serviceStarted = sService->start();
  if (serviceStarted == false) {
    Serial.println("[BLE] Service failed to start");
    ESP.restart();
  }
  NimBLEAdvertising* sAdvertising = NimBLEDevice::getAdvertising();
  sAdvertising->addServiceUUID(serviceUUID);
  bool advertisingStarted = sAdvertising->start();
  if (advertisingStarted == false) {
    Serial.println("[BLE] Advertising failed to start");
    ESP.restart();
  }
  // /Прокси

  // Запланировали сканирование
  doScan = true;
}

void loop() {
  if (doScan == true) {
    doScan = false;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new EUCFoundDeviceCallbacks(), false);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->setActiveScan(true);
    pScan->start(15);
  } else if (doConnect == true) {
    doConnect = false;
    Serial.printf("[BLE] Connecting to... %s\n", EUC.bleDevice->getName().c_str());
    bool connected = bleClient->connect(EUC.bleDevice, true);
    if (connected) {
      doInit = true;
    }
  } else if (doInit == true) {
    Serial.println("[BLE] Initializing client");
    doInit = false;

    NimBLERemoteService* pRemoteService = bleClient->getService(serviceUUID);
    txCharacteristic = pRemoteService->getCharacteristic(txCharUUID);
    rxCharacteristic = pRemoteService->getCharacteristic(rxCharUUID);
    if (txCharacteristic == nullptr || rxCharacteristic == nullptr) {
      Serial.println("[BLE] RemoteCharacteristics not found!");
      doInit == true;
      return;
    }

    bool subscribed = rxCharacteristic->subscribe(true, eucMessageReceived, false);
    if (subscribed) {
      Serial.println("Subscribed!");
      doInitProxy = true;
    }
    Serial.println("Initialized client!");
  } else if (doInitProxy == true) {
    doInitProxy = false;
    doWork = true;
  } else if (doWork == true) {

    int ledsRoll = map(EUC.pitchAngle, -1500, 1500, 0, 12);
    // Serial.println(ledsRoll);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < ledsRoll) {
        leds[i] = CRGB::PowderBlue;
      } else {
        leds[i] = CRGB::Black;
      }
    }
    FastLED.show();

  }
}
