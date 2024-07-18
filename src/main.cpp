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

#define MY_PERIOD 500  // период в мс
uint32_t tmr1;         // переменная таймера

// Список сервисов и характеристик
static NimBLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID txCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID rxCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static NimBLEUUID unknownService1UUID("FFE5");
static NimBLEUUID unknownService1CharUUID("FFE9");
static NimBLEUUID unknownService2UUID("FFE0");
static NimBLEUUID unknownService2CharUUID("FFE4");

static NimBLEClient* bleClient;
static NimBLEAdvertisedDevice* selectedDevice;
static NimBLERemoteService* pRemoteService;
static NimBLERemoteCharacteristic* rxCharacteristic;
static NimBLERemoteCharacteristic* txCharacteristic;

static NimBLEServer* bleServer;
static NimBLEService* sService;
static NimBLECharacteristic* sTxCharacteristic;
static NimBLECharacteristic* sRxCharacteristic;

InmotionUnpackerV2* unpacker = new InmotionUnpackerV2();
InmotionV2Message* pMessage = new InmotionV2Message();
static EUC& euc = EUC::getInstance();

static boolean doScan = false;
static boolean doConnect = false;
static boolean doInit = false;
static boolean doWork = false;
static boolean doProxy = false;

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

class ProxyCallbacks : public NimBLECharacteristicCallbacks {
  // onRead
  void onWrite(NimBLECharacteristic* pCharacteristic) {
    digitalWrite(ledPin, HIGH);

    const uint8_t* pData = pCharacteristic->getValue().data();
    uint8_t length = pCharacteristic->getDataLength();

    // Serial.print("👈 ");
    txCharacteristic->writeValue(pData, length, false);

    // for (size_t i = 0; i < length; i++) {
    //   // Serial.printf("0x%02hX ", pData[i]);
    //   if (unpacker->addChar(pData[i]) == true) {
    //     uint8_t* buffer = unpacker->getBuffer();
    //     size_t bufferIndex = unpacker->getBufferIndex();

    //     pMessage->parse(buffer, bufferIndex);
    //   }
    // }
    // Serial.println();
  }
  // onNotify
  // onStatus
  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    Serial.println("Subscribed!");
    doProxy = true;
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
    Serial.printf("[BLE] Device %s selected\n", advertisedDevice->getName().c_str());
  }

  /** Callback to process the results of the completed scan or restart it */
  // void onScanEnd(NimBLEScanResults results) { Serial.println("Scan Ended"); }
};

/** @brief Запуск сканирования */
void startBLEScan() {
  // Serial.println("[BLE] Scanning...");
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
  // Serial.println("[BLE] Connecting...");
  bleClient->connect(selectedDevice, true);

  doScan = false;
  doConnect = false;
  doInit = true;
  doWork = false;
}

/** @brief Коллбек при получении ответа от колеса */
static void rxReceivedCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // Serial.print("👉 ");
  // for (size_t i = 0; i < length; i++) {
  //   Serial.printf("0x%02hX ", pData[i]);
  // }
  // Serial.println();
  if (doProxy == true) {
    if (bleServer->getConnectedCount()) {
      NimBLEService* pSvc = bleServer->getServiceByUUID(serviceUUID);
      if (pSvc != nullptr) {
        NimBLECharacteristic* pChr = pSvc->getCharacteristic(rxCharUUID);
        if (pChr != nullptr) {
          pChr->notify(pData, length, true);

          pMessage->parse(pData, length);
        }
      }
    }
  }

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
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
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
void SetupPurpleAndOrangePalette() {
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB orange = CHSV(HUE_ORANGE, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(orange, orange, black, black, purple, purple, black, black, orange, orange, black, black, purple, purple, black, black);
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM = {CRGB::Red,
                                                           CRGB::Gray,  // 'white' is too bright compared to red and blue
                                                           CRGB::Blue, CRGB::Black,

                                                           CRGB::Red,  CRGB::Gray,  CRGB::Blue, CRGB::Black,

                                                           CRGB::Red,  CRGB::Red,   CRGB::Gray, CRGB::Gray,  CRGB::Blue, CRGB::Blue, CRGB::Black, CRGB::Black};

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
      SetupPurpleAndOrangePalette();
      currentBlending = NOBLEND;
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
  // FastLED.setBrightness(20);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 100);
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  // set_max_power_indicator_LED(13);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.begin(115200);
  Serial.println();
  Serial.println("[INFO] Starting ESP32-EUC application...");

  NimBLEDevice::init("V11-ESP32EUC");
  bleClient = NimBLEDevice::createClient();
  bleClient->setClientCallbacks(new EucDeviceCallbacks());

  // Прокси
  bleServer = NimBLEDevice::createServer();
  NimBLEService* sService = bleServer->createService(serviceUUID);
  NimBLECharacteristic* sTxCharacteristic = sService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  NimBLECharacteristic* sRxCharacteristic = sService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::NOTIFY);

  NimBLEService* ffe5 = bleServer->createService(NimBLEUUID("FFE5"));
  NimBLECharacteristic* ffe9 = sService->createCharacteristic(NimBLEUUID("FFE9"), NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);

  NimBLEService* ffe0 = bleServer->createService(NimBLEUUID("FFE0"));
  NimBLECharacteristic* ffe4 = sService->createCharacteristic(NimBLEUUID("FFE4"), NIMBLE_PROPERTY::NOTIFY);

  sTxCharacteristic->setCallbacks(new ProxyCallbacks());
  sRxCharacteristic->setCallbacks(new ProxyCallbacks());
  ffe9->setCallbacks(new ProxyCallbacks());
  ffe4->setCallbacks(new ProxyCallbacks());

  sService->start();
  NimBLEAdvertising* sAdvertising = NimBLEDevice::getAdvertising();
  sAdvertising->addServiceUUID(serviceUUID);
  sAdvertising->start();
  // /Прокси

  // Запланировали сканирование
  doScan = true;

  // Таймер для команд
  unsigned long start = millis();
}

void loop() {
  // ChangePalettePeriodically();
  // static uint8_t startIndex = 0;
  // startIndex = startIndex + 1; /* motion speed */

  // FillLEDsFromPaletteColors(startIndex);

  // FastLED.show();
  // FastLED.delay(1000 / 100);

  if (doScan == true) {
    startBLEScan();
  } else if (doConnect == true) {
    connectToBleDevice();
  } else if (doInit == true) {
    initBleDevice();
  } else if (doWork == true) {
    int ledsRoll = map(euc.rollAngle, -9000, 9000, 0, 12);
    Serial.println(ledsRoll);
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < ledsRoll) {
        leds[i] = CRGB::PowderBlue;
      } else {
        leds[i] = CRGB::Black;
      }
    }
    FastLED.show();

    // if (millis() - tmr1 >= MY_PERIOD) {
    //   tmr1 = millis();

    //   // Перед отправкой сообщения включаем светодиод на плате
    //   // digitalWrite(ledPin, HIGH);
    //   // Serial.println("[EUC] Sending live packet...");
    //   // uint8_t live[6] = {0xAA, 0xAA, 0x14, 0x01, 0x04, 0x11};
    //   // uint8_t stats[6] = {0xAA, 0xAA, 0x14, 0x01, 0x11, 0x04};
    //   // uint8_t drlOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2D, 0x01, 0x5B};
    //   // uint8_t drlOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2D, 0x00,
    //   0x5A};
    //   // uint8_t lightsOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x50, 0x01,
    //   0x26};
    //   // uint8_t lightsOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x50, 0x00,
    //   0x27};
    //   // uint8_t fanOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x43, 0x01, 0x35};
    //   // uint8_t fanOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x43, 0x00,
    //   0x34};
    //   // uint8_t fanQuietOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x38, 0x01,
    //   0x4E};
    //   // uint8_t fanQuietOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x38, 0x00,
    //   0x4F};
    //   // uint8_t liftOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2E, 0x01,
    //   0x58};
    //   // uint8_t liftOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2E, 0x00,
    //   0x59};
    //   // uint8_t lock[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x31, 0x01, 0x47};
    //   // uint8_t unlock[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x31, 0x00,
    //   0x46};
    //   // uint8_t transportOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x32, 0x01,
    //   0x44};
    //   // uint8_t transportOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x32, 0x00,
    //   0x45};
    //   // uint8_t rideComfort[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x23, 0x00,
    //   0x54};
    //   // uint8_t rideSport[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x23, 0x01,
    //   0x55};
    //   // uint8_t performanceOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x24,
    //   0x01, 0x52};
    //   // uint8_t performanceOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x24,
    //   0x00, 0x53};
    //   // uint8_t remainderReal[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3D,
    //   0x01, 0x4B};
    //   // uint8_t remainderEst[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3D, 0x00,
    //   0x4A};
    //   // uint8_t lowBatLimitOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x37,
    //   0x01, 0x41};
    //   // uint8_t lowBatLimitOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x37,
    //   0x00, 0x40};
    //   // uint8_t usbOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3C, 0x01, 0x4A};
    //   // uint8_t usbOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3C, 0x00,
    //   0x4B};
    //   // uint8_t loadDetectOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x36, 0x01,
    //   0x40};
    //   // uint8_t loadDetectOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x36,
    //   0x00, 0x41};
    //   // uint8_t mute[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2C, 0x00, 0x5B};
    //   // uint8_t unmute[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2C, 0x01,
    //   0x5A};
    //   // uint8_t calibration[10] = {0xAA, 0xAA, 0x14, 0x05, 0x60, 0x42, 0x01,
    //   0x00, 0x01, 0x33};
    //   // txCharacteristic->writeValue(live, sizeof(live), false);
    // }

    // Яркость фары в зависимости от скорости движения
    // brightness = map(euc.speed, 0, 30, 5, 255);
    // brightness = 10;

    // if (euc.speed > 0) {
    //   brightness = 30;
    // }
  }
}
