#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define CONFIG_NIMBLE_CPP_LOG_LEVEL 3

#include <Arduino.h>
#include <EUC.h>
#include <EncButton.h>
#include <FastLED.h>
#include <FileData.h>
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

struct SettingsStruct {
  unsigned int paletteIndex = 1;
  unsigned long paletteChangedTime = 0;

  unsigned int modeIndex = 1;
  unsigned long modeChangedTime = 0;
};
SettingsStruct espSettings;

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

  pinMode(LED_PIN, OUTPUT);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 100);
  // FastLED.setBrightness(10);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[0] = CRGB::Red;
  leds[NUM_LEDS - 1] = CRGB::Red;
  FastLED.show();
  ESP_LOGI(LOG_TAG, "Starting ESP32-EUC application...");

  NimBLEDevice::init("V11-ESP32EUC");
}

#include <effects.h>

static void showLedFrame() {
  startIndex = startIndex + 1;
  if (espSettings.modeIndex == 1) {
    if (EUC.lampState) {
      brightness = 200;
    } else if (EUC.decorativeLightState) {
      brightness = 100;
    } else {
      brightness = 50;
    }
    // Заполнение по скорости, иначе просто играем палитрой
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    if (EUC.speed == 0) {
      FillLEDsFromPaletteColors(startIndex, NUM_LEDS);
    } else if (EUC.speed != 0) {
      if (EUC.brakeState) {
        fill_solid(leds, NUM_LEDS, CRGB::Red);
      } else {
        maxLeds = map(abs(EUC.speed), 0, 5000, 0, NUM_LEDS);
        FillLEDsFromPaletteColors(startIndex, maxLeds);
      }
    }
  } else if (espSettings.modeIndex == 2) {
    // Всегда играем палитрой
    FillLEDsFromPaletteColors(startIndex, NUM_LEDS);
  } else if (espSettings.modeIndex == 3) {
    // Ёлочная игрушка
    juggle();
  } else if (espSettings.modeIndex == 4) {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }

  if (espSettings.modeChangedTime != 0 && millis() - espSettings.modeChangedTime <= 3000) {
    fill_solid(leds, espSettings.modeIndex, CRGB::Red);
  }

  if (espSettings.paletteChangedTime != 0 && millis() - espSettings.paletteChangedTime <= 3000) {
    fill_solid(leds, espSettings.paletteIndex, CRGB::Green);
  }

  FastLED.show();
}

void loop() {
  btn.tick();
  EUC.tick();

  if (btn.hold()) {
    espSettings.modeChangedTime = millis();
    espSettings.modeIndex++;
    if (espSettings.modeIndex > 4) {
      espSettings.modeIndex = 1;
    }
    ESP_LOGI(LOG_TAG, "Changed mode to %d", espSettings.modeIndex);
  } else if (btn.click(1)) {
    espSettings.paletteChangedTime = millis();
    espSettings.paletteIndex++;
    if (espSettings.paletteIndex > 9) {
      espSettings.paletteIndex = 1;
    }
    ESP_LOGI(LOG_TAG, "Changed palette to %d", espSettings.paletteIndex);
  }

  switch (espSettings.paletteIndex) {
    case 1:
      currentPalette = CloudColors_p;
      break;
    case 2:
      currentPalette = LavaColors_p;
      break;
    case 3:
      currentPalette = OceanColors_p;
      break;
    case 4:
      currentPalette = ForestColors_p;
      break;
    case 5:
      fill_solid(currentPalette, 16, CRGB::Black);
      currentPalette[0] = CRGB::White;
      currentPalette[4] = CRGB::White;
      currentPalette[8] = CRGB::White;
      currentPalette[12] = CRGB::White;
      break;
    case 6:
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
    case 7:
      currentPalette = PartyColors_p;
      break;
    case 8:
      currentPalette = HeatColors_p;
      break;
    case 9:
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

  if (timerFastLed.tick()) {
    showLedFrame();
  }
}
