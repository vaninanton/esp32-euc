#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define CONFIG_NIMBLE_CPP_LOG_LEVEL 3

#include <Arduino.h>
#include <EEPROM.h>
#include <EUC.h>
#include <EncButton.h>
#include <FastLED.h>
#include <FileData.h>
#include <InmotionV2Message.h>
#include <InmotionV2Unpacker.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <TimerMs.h>
#include <esp_log.h>

static const char* LOG_TAG = "ESP32-EUC";
const int LED_PIN = 2;
#define DATA_PIN 13
#define NUM_LEDS 17

CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette = OceanColors_p;
TBlendType currentBlending = LINEARBLEND;
uint8_t brightness = 30;
uint8_t startIndex = 0;
uint8_t maxLeds = NUM_LEDS;

TimerMs timerFastLed(10, 1, false);
TimerMs timerShowEucDebugLog(1000, 1, false);
Button btn(0);

// Работа с EUC
static InmotionV2Unpacker* unpacker = new InmotionV2Unpacker();
static InmotionV2Message* pMessage = new InmotionV2Message();
static NimBLEClient* eucBleClient;
static NimBLEServer* appBleServer;
static const NimBLEUUID uartServiceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static const NimBLEUUID ffe5ServiceUUID("FFE5");
static const NimBLEUUID ffe9CharUUID("FFE9");
static const NimBLEUUID ffe0ServiceCharUUID("FFE0");
static const NimBLEUUID ffe4CharUUID("FFE4");
static NimBLERemoteService* eucService;
static NimBLERemoteCharacteristic* eucRxCharacteristic;
static NimBLERemoteCharacteristic* eucTxCharacteristic;
static NimBLEService* appService;
static NimBLEService* appFfe5;
static NimBLEService* appFfe0;
static NimBLECharacteristic* appTxCharacteristic;
static NimBLECharacteristic* appRxCharacteristic;
static NimBLECharacteristic* appFfe9Characteristic;
static NimBLECharacteristic* appFfe4Characteristic;
static NimBLEAdvertising* appAdvertising;

static boolean doScanEUC = false;
static boolean doConnectToEUC = false;
static boolean doInit = false;
static boolean doWork = false;

struct SettingsStruct {
  uint8_t brightnessButtonIndex = 0;
  uint8_t paletteButtonIndex = 0;
  uint8_t modeButtonIndex = 0;
};
SettingsStruct espSettings;
FileData settingsData(&LittleFS, "/settings.dat", 'B', &espSettings, sizeof(espSettings));

/** @brief Received message from app, proxy it to euc */
static void appNotifyReceived(NimBLECharacteristic* pCharacteristic) {
  if (eucBleClient->isConnected() == false || doWork == false) {
    ESP_LOGW(LOG_TAG, "EUC is not connected, message will be dropped");
    return;
  }

  // Write message from app to euc
  eucBleClient->getService(uartServiceUUID)->getCharacteristic(pCharacteristic->getUUID())->writeValue(pCharacteristic->getValue().data(), pCharacteristic->getDataLength(), false);
}

/** @brief Received message from euc, proxy it to app and parse to EUC object */
static void eucNotifyReceived(NimBLERemoteCharacteristic* eucCharacteristic, uint8_t* data, size_t dataLength, bool isNotify) {
  digitalWrite(LED_PIN, HIGH);

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

  // Send notification from euc to app
  appBleServer->getServiceByUUID(eucCharacteristic->getRemoteService()->getUUID())->getCharacteristic(eucCharacteristic->getUUID())->notify(data, dataLength, true);
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
    ESP_LOGI(LOG_TAG, "Found %s", EUC.bleDevice->getAddress().toString().c_str());
    doConnectToEUC = true;
  }
};

/** @brief Проксирование сообщений от приложения к колесу */
class ProxyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) { appNotifyReceived(pCharacteristic); }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    doWork = subValue > 0;
    if (doWork) {
      ESP_LOGI(LOG_TAG, "App subscribed");
    } else {
      ESP_LOGI(LOG_TAG, "App unsubscribed");
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
}

void initEUC() {
  ESP_LOGD("ESP32-BLE", "Initializing client services...");
  doInit = false;

  NimBLERemoteService* eucService = eucBleClient->getService(uartServiceUUID);
  eucTxCharacteristic = eucService->getCharacteristic(txCharUUID);
  eucRxCharacteristic = eucService->getCharacteristic(rxCharUUID);
  eucRxCharacteristic->subscribe(true, eucNotifyReceived, false);

  appTxCharacteristic->setCallbacks(new ProxyCallbacks());
  appRxCharacteristic->setCallbacks(new ProxyCallbacks());
  appFfe9Characteristic->setCallbacks(new ProxyCallbacks());
  appFfe4Characteristic->setCallbacks(new ProxyCallbacks());
  appAdvertising->start();

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

  LittleFS.begin();
  FDstat_t stat = settingsData.read();

  pinMode(LED_PIN, OUTPUT);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
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
  appFfe5 = appBleServer->createService(ffe5ServiceUUID);
  appFfe0 = appBleServer->createService(ffe0ServiceCharUUID);

  appTxCharacteristic = appService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  appRxCharacteristic = appService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);
  appFfe9Characteristic = appFfe5->createCharacteristic(ffe9CharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  appFfe4Characteristic = appFfe0->createCharacteristic(ffe4CharUUID, NIMBLE_PROPERTY::NOTIFY);

  appService->start();
  appAdvertising = NimBLEDevice::getAdvertising();
  appAdvertising->addServiceUUID(uartServiceUUID);
  appAdvertising->start();
  appAdvertising->stop();
  // /Прокси

  // Запланировали сканирование
  doScanEUC = true;
}

#include <effects.h>

void loop() {
  settingsData.tick();
  btn.tick();

  if (doScanEUC == true) {
    scanEUC();
  } else if (doConnectToEUC == true) {
    connectToEUC();
  } else if (doInit == true) {
    initEUC();
  } else if (doWork == false) {
    // Do nothing
  } else if (doWork == true) {
    if (timerShowEucDebugLog.tick()) {
      // EUC.debug();
    }
  }

  if (btn.click(2)) {
    espSettings.modeButtonIndex++;
    if (espSettings.modeButtonIndex > 1) {
      espSettings.modeButtonIndex = 0;
    }
    ESP_LOGI(LOG_TAG, "Changed mode to %d", espSettings.modeButtonIndex);
    settingsData.update();
  } else if (btn.click(1)) {
    espSettings.paletteButtonIndex++;
    if (espSettings.paletteButtonIndex > 8) {
      espSettings.paletteButtonIndex = 0;
    }
    ESP_LOGI(LOG_TAG, "Changed palette to %d", espSettings.paletteButtonIndex);
    settingsData.update();
  }

  switch (espSettings.paletteButtonIndex) {
    case 0:
      currentPalette = CloudColors_p;
      break;
    case 1:
      currentPalette = LavaColors_p;
      break;
    case 2:
      currentPalette = OceanColors_p;
      break;
    case 3:
      currentPalette = ForestColors_p;
      break;
    case 4:
      fill_solid(currentPalette, 16, CRGB::Black);
      currentPalette[0] = CRGB::White;
      currentPalette[4] = CRGB::White;
      currentPalette[8] = CRGB::White;
      currentPalette[12] = CRGB::White;
      break;
    case 5:
      fill_solid(currentPalette, 16, CRGB::Black);
      currentPalette[0] = CRGB::Green;
      currentPalette[1] = CRGB::Green;

      currentPalette[4] = CRGB::Purple;
      currentPalette[5] = CRGB::Purple;

      currentPalette[8] = CRGB::Green;
      currentPalette[9] = CRGB::Green;

      currentPalette[12] = CRGB::Purple;
      currentPalette[13] = CRGB::Purple;
      break;
    case 6:
      currentPalette = PartyColors_p;
      break;
    case 7:
      currentPalette = HeatColors_p;
      break;
    case 8:
      fill_solid(currentPalette, 16, CRGB::Red);
      // currentPalette[0] = CRGB::Red;
      // currentPalette[1] = CRGB::Red;

      currentPalette[4] = CRGB::Blue;
      currentPalette[5] = CRGB::Blue;

      // currentPalette[8] = CRGB::Red;
      // currentPalette[9] = CRGB::Red;

      currentPalette[12] = CRGB::Blue;
      currentPalette[13] = CRGB::Blue;
      break;
  }

  brightness = EUC.lampState ? 100 : (EUC.decorativeLightState ? 50 : 1);

  if (timerFastLed.tick()) {
    startIndex = startIndex + 1;
    if (EUC.speed != 0) {
      fadeToBlackBy(leds, NUM_LEDS, 20);
      fill_solid(leds, maxLeds, CRGB(255, 0, 0));
    } else if (EUC.speed != 0) {
      fadeToBlackBy(leds, NUM_LEDS, 20);
      maxLeds = map(abs(EUC.speed), 0, 4000, 0, NUM_LEDS);
      FillLEDsFromPaletteColors(startIndex, maxLeds);
    } else if (espSettings.modeButtonIndex == 0) {
      FillLEDsFromPaletteColors(startIndex, NUM_LEDS);
    } else if (espSettings.modeButtonIndex == 1) {
      juggle();
    }
  }
  FastLED.show();
}
