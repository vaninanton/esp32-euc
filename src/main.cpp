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
int brightness = 10;

#define MY_PERIOD 500  // период в мс
uint32_t tmr1;         // переменная таймера

// Список сервисов и характеристик
static NimBLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

static NimBLEClient* bleClient;
static NimBLEAdvertisedDevice* selectedDevice;
static NimBLERemoteCharacteristic* rxCharacteristic;
static NimBLERemoteCharacteristic* txCharacteristic;

static InmotionV2Message* pMessage;
static EUC& euc = EUC::getInstance();

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

// Подсветка
CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness,
                               currentBlending);
    colorIndex += 3;
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette() {
  for (int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV(random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette() {
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette() {
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette =
      CRGBPalette16(green, green, black, black, purple, purple, black, black,
                    green, green, black, black, purple, purple, black, black);
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM = {
    CRGB::Red,
    CRGB::Gray,  // 'white' is too bright compared to red and blue
    CRGB::Blue, CRGB::Black,

    CRGB::Red,  CRGB::Gray,  CRGB::Blue,  CRGB::Black,

    CRGB::Red,  CRGB::Red,   CRGB::Gray,  CRGB::Gray,
    CRGB::Blue, CRGB::Blue,  CRGB::Black, CRGB::Black};

void ChangePalettePeriodically() {
  uint8_t secondHand = (millis() / 1000) % 60;
  static uint8_t lastSecond = 99;

  if (lastSecond != secondHand) {
    lastSecond = secondHand;
    if (secondHand == 0) {
      currentPalette = RainbowColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 10) {
      currentPalette = RainbowStripeColors_p;
      currentBlending = NOBLEND;
    }
    if (secondHand == 15) {
      currentPalette = RainbowStripeColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 20) {
      SetupPurpleAndGreenPalette();
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 25) {
      SetupTotallyRandomPalette();
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 30) {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = NOBLEND;
    }
    if (secondHand == 35) {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 40) {
      currentPalette = CloudColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 45) {
      currentPalette = PartyColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 50) {
      currentPalette = myRedWhiteBluePalette_p;
      currentBlending = NOBLEND;
    }
    if (secondHand == 55) {
      currentPalette = myRedWhiteBluePalette_p;
      currentBlending = LINEARBLEND;
    }
  }
}

//////////////////////////////////
//////////////////////////////////
//////////////////////////////////

void setup() {
  pinMode(ledPin, OUTPUT);

  FastLED.addLeds<LED_TYPE, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 200);
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  // set_max_power_indicator_LED(13);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

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
  ChangePalettePeriodically();
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors(startIndex);

  FastLED.show();
  FastLED.delay(1000 / 100);

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

    // Яркость фары в зависимости от скорости движения
    map(euc.getSpeed(), 0, 30, 5, 255);
  }
}
