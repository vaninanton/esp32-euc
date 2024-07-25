#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define CONFIG_NIMBLE_CPP_LOG_LEVEL 3

#include <Arduino.h>
#include <EUC.h>
#include <EncButton.h>
#include <FastLED.h>
#include <InmotionV2Message.h>
#include <InmotionV2Unpacker.h>
#include <NimBLEDevice.h>
#include <TimerMs.h>
#include <esp_log.h>

const int LED_PIN = 2;
#define DATA_PIN 12
#define LED_TYPE WS2812B
#define NUM_LEDS 12
CRGB leds[NUM_LEDS];

static const char* LOG_TAG = "ESP32-EUC";

TimerMs tmrFL(10, 1, false);
Button btn(0);

// Список сервисов и характеристик
static const NimBLEUUID uartServiceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID unknownService1UUID("FFE5");
static const NimBLEUUID unknownService1CharUUID("FFE9");
static const NimBLEUUID unknownService2UUID("FFE0");
static const NimBLEUUID unknownService2CharUUID("FFE4");

static NimBLEClient* eucBleClient;
static NimBLEServer* appBleServer;

static NimBLERemoteService* pRemoteService;
static NimBLERemoteCharacteristic* rxCharacteristic;
static NimBLERemoteCharacteristic* txCharacteristic;

static NimBLEService* appService;
static NimBLEService* ffe5;
static NimBLEService* ffe0;
static NimBLECharacteristic* sTxCharacteristic;
static NimBLECharacteristic* sRxCharacteristic;
static NimBLECharacteristic* ffe9Characteristic;
static NimBLECharacteristic* ffe4Characteristic;
static NimBLEAdvertising* sAdvertising;

static InmotionV2Unpacker* unpacker = new InmotionV2Unpacker();
static InmotionV2Message* pMessage = new InmotionV2Message();

static boolean doScanEUC = false;
static boolean doConnectToEUC = false;
static boolean doInit = false;
static boolean doWork = false;

CRGBPalette16 currentPalette = OceanColors_p;
TBlendType currentBlending = LINEARBLEND;
uint8_t brightness = 10;
uint8_t startIndex = 0;

uint8_t palleteButtonIndex = 0;
uint8_t brightnessButtonIndex = 0;

/** @brief Received message from app, proxy it to euc */
static void appMessageReceived(NimBLECharacteristic* pCharacteristic) {
  // ESP_LOGD(LOG_TAG, "A");
  if (eucBleClient->isConnected() == false || doWork == false) {
    ESP_LOGW(LOG_TAG, "EUC is not connected, message will be dropped");
    return;
  }

  eucBleClient->getService(uartServiceUUID)->getCharacteristic(pCharacteristic->getUUID())->writeValue(pCharacteristic->getValue().data(), pCharacteristic->getDataLength(), false);
}

/** @brief Received message from euc, proxy it to app and parse to EUC object */
static void eucNotifyReceived(NimBLERemoteCharacteristic* eucCharacteristic, uint8_t* data, size_t dataLength, bool isNotify) {
  digitalWrite(LED_PIN, HIGH);
  // ESP_LOGD(LOG_TAG, "E");

  for (size_t i = 0; i < dataLength; i++) {
    if (unpacker->addChar(data[i]) == true) {
      uint8_t* buffer = unpacker->getBuffer();
      size_t bufferLength = unpacker->getBufferIndex();

      pMessage->parse(buffer, bufferLength);
    }
  }

  if (doWork == false) {
    ESP_LOGW(LOG_TAG, "Couldn't proxy message to app, because app was not connected");
    digitalWrite(LED_PIN, LOW);
    return;
  }

  NimBLEService* service = appBleServer->getServiceByUUID(eucCharacteristic->getRemoteService()->getUUID());
  NimBLECharacteristic* characteristic = service->getCharacteristic(eucCharacteristic->getUUID());
  characteristic->notify(data, dataLength, true);
  digitalWrite(LED_PIN, LOW);
}

class EUCFoundDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    // Skip if it's not the service we're looking for
    if (advertisedDevice->getName() == "" || !advertisedDevice->isAdvertisingService(uartServiceUUID))
      return;

    // We have found a device, but it's not yet connected
    NimBLEDevice::getScan()->stop();

    EUC.bleDevice = advertisedDevice;
    ESP_LOGI(LOG_TAG, "Found %s", EUC.bleDevice->getName().c_str());
    doConnectToEUC = true;
  }
};

/** @brief Проксирование сообщений от приложения к колесу */
class ProxyCallbacks : public NimBLECharacteristicCallbacks {
  // void onRead(NimBLECharacteristic* pCharacteristic) {}
  void onWrite(NimBLECharacteristic* pCharacteristic) { appMessageReceived(pCharacteristic); }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    doWork = subValue > 0;
    if (doWork) {
      ESP_LOGI(LOG_TAG, "App subscribed");
      currentPalette = PartyColors_p;
      brightness = 30;
    } else {
      ESP_LOGI(LOG_TAG, "App unsubscribed");
      currentPalette = ForestColors_p;
      brightness = 10;
    }
  }
};

void scanEUC() {
  doScanEUC = false;
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new EUCFoundDeviceCallbacks(), false);
  pScan->setInterval(100);
  pScan->setWindow(99);
  pScan->setActiveScan(true);
  pScan->start(15);
}

void connectToEUC() {
  ESP_LOGI("ESP32-BLE", "Connecting to... %s", EUC.bleDevice->getName().c_str());
  doConnectToEUC = false;
  if (eucBleClient->isConnected()) {
    eucBleClient->disconnect();
  }
  bool connected = eucBleClient->connect(EUC.bleDevice, true);
  if (!connected) {
    doConnectToEUC = true;
    return;
  }
  doInit = true;
  currentPalette = ForestColors_p;
}

void initEUC() {
  ESP_LOGD("ESP32-BLE", "Initializing client services...");
  doInit = false;

  NimBLERemoteService* pRemoteService = eucBleClient->getService(uartServiceUUID);
  txCharacteristic = pRemoteService->getCharacteristic(txCharUUID);
  rxCharacteristic = pRemoteService->getCharacteristic(rxCharUUID);
  rxCharacteristic->subscribe(true, eucNotifyReceived, false);

  sTxCharacteristic->setCallbacks(new ProxyCallbacks());
  sRxCharacteristic->setCallbacks(new ProxyCallbacks());
  ffe9Characteristic->setCallbacks(new ProxyCallbacks());
  ffe4Characteristic->setCallbacks(new ProxyCallbacks());
  sAdvertising->start();

  ESP_LOGI("ESP32-BLE", "EUC initialized!");
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

void setup() {
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  esp_log_level_set("NimappBleServer", ESP_LOG_WARN);
  esp_log_level_set("NimBLEAdvertising", ESP_LOG_WARN);
  esp_log_level_set("NimBLEScan", ESP_LOG_WARN);
  esp_log_level_set("NimBLEClient", ESP_LOG_WARN);
  esp_log_level_set("NimBLEService", ESP_LOG_WARN);
  esp_log_level_set("NimBLEDevice", ESP_LOG_WARN);
  esp_log_level_set("NimBLECharacteristic", ESP_LOG_WARN);
  esp_log_level_set("NimBLECharacteristicCallbacks", ESP_LOG_WARN);
  // esp_log_level_set("wifi", ESP_LOG_WARN);
  esp_log_level_set(LOG_TAG, ESP_LOG_INFO);

  pinMode(LED_PIN, OUTPUT);

  FastLED.addLeds<LED_TYPE, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 100);
  // FastLED.setBrightness(10);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Serial.begin(115200);
  ESP_LOGI(LOG_TAG, "Starting ESP32-EUC application...");

  NimBLEDevice::init("V11-ESP32EUC");
  eucBleClient = NimBLEDevice::createClient();
  appBleServer = NimBLEDevice::createServer();

  appService = appBleServer->createService(uartServiceUUID);
  ffe5 = appBleServer->createService(unknownService1UUID);
  ffe0 = appBleServer->createService(unknownService2UUID);

  sTxCharacteristic = appService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  sRxCharacteristic = appService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);
  ffe9Characteristic = ffe5->createCharacteristic(unknownService1CharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  ffe4Characteristic = ffe0->createCharacteristic(unknownService2CharUUID, NIMBLE_PROPERTY::NOTIFY);

  appService->start();
  sAdvertising = NimBLEDevice::getAdvertising();
  sAdvertising->addServiceUUID(uartServiceUUID);
  sAdvertising->start();
  sAdvertising->stop();
  // /Прокси

  // Запланировали сканирование
  doScanEUC = true;
}

#include <effects.h>

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void loop() {
  btn.tick();

  if (doScanEUC == true) {
    currentPalette = OceanColors_p;
    brightness = 1;
    scanEUC();
  } else if (doConnectToEUC == true) {
    currentPalette = OceanColors_p;
    brightness = 5;
    connectToEUC();
  } else if (doInit == true) {
    currentPalette = OceanColors_p;
    brightness = 10;
    initEUC();
  } else if (doWork == false) {

  } else if (doWork == true) {
    // juggle();
  }

  if (btn.click(2)) {
    palleteButtonIndex++;
    if (palleteButtonIndex == 9) {
      palleteButtonIndex = 1;
    }
    switch (palleteButtonIndex) {
      case 1: currentPalette = CloudColors_p; break;
      case 2: currentPalette = LavaColors_p; break;
      case 3: currentPalette = OceanColors_p; break;
      case 4: currentPalette = ForestColors_p; break;
      case 5: currentPalette = RainbowColors_p; break;
      case 6: currentPalette = RainbowStripeColors_p; break;
      case 7: currentPalette = PartyColors_p; break;
      case 8: currentPalette = HeatColors_p; break;
    }
    ESP_LOGI(LOG_TAG, "Changed palette to %d", palleteButtonIndex);
  }

  if (btn.step()) {
    if (btn.stepFor(2000)) {
      brightness += 5;
    } else {
      brightness += 1;
    }
    ESP_LOGI(LOG_TAG, "Changed brightness to %d", brightness);
  }

  if (brightness >= 255) {
    brightness = 1;
  }

  if (tmrFL.tick()) {
    startIndex = startIndex + 1;
    FillLEDsFromPaletteColors(startIndex);
    FastLED.show();
  }
}
