#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define CONFIG_NIMBLE_CPP_LOG_LEVEL 3

#include <Arduino.h>
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

struct SettingsStruct {
  uint8_t brightnessButtonIndex = 0;
  uint8_t paletteButtonIndex = 0;
  uint8_t modeButtonIndex = 0;
};
SettingsStruct espSettings;
FileData settingsData(&LittleFS, "/settings.dat", 'B', &espSettings, sizeof(espSettings));

void setup() {
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  esp_log_level_set("NimBLEServer", ESP_LOG_WARN);
  esp_log_level_set("NimBLEAdvertising", ESP_LOG_WARN);
  esp_log_level_set("NimBLEScan", ESP_LOG_WARN);
  esp_log_level_set("NimBLEClient", ESP_LOG_WARN);
  esp_log_level_set("NimBLEService", ESP_LOG_WARN);
  esp_log_level_set("NimBLEDevice", ESP_LOG_WARN);
  esp_log_level_set("NimBLECharacteristic", ESP_LOG_WARN);
  esp_log_level_set("NimBLECharacteristicCallbacks", ESP_LOG_WARN);
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
  ESP_LOGI(LOG_TAG, "Starting ESP32-EUC application...");

  NimBLEDevice::init("V11-ESP32EUC");
}

#include <effects.h>

void loop() {
  settingsData.tick();
  btn.tick();

  EUC.bleConnect();

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
