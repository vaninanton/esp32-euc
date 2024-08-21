#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define CONFIG_NIMBLE_CPP_LOG_LEVEL 3

#include <Arduino.h>
#include <EUC.h>
#include <EncButton.h>
#include <FastLED.h>
#include <FileData.h>
#include <LED.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <TimerMs.h>
#include <esp_log.h>

static const char* LOG_TAG = "ESP32-EUC";
const int LED_PIN = 2;

TimerMs timerShowEucDebugLog(1000, 1, false);
Button btn(0);

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

  ESP_LOGI(LOG_TAG, "Starting ESP32-EUC application...");
  pinMode(LED_PIN, OUTPUT);

  NimBLEDevice::init("V11-ESP32EUC");
  LED.setup();
}

void loop() {
  btn.tick();
  EUC.tick();

  if (btn.hold()) {
    LED.espSettings.modeChangedTime = millis();
    LED.espSettings.modeIndex++;
    if (LED.espSettings.modeIndex > 4) {
      LED.espSettings.modeIndex = 1;
    }
    ESP_LOGI(LOG_TAG, "Changed mode to %d", LED.espSettings.modeIndex);
  } else if (btn.click(1)) {
    LED.espSettings.paletteChangedTime = millis();
    LED.espSettings.paletteIndex++;
    if (LED.espSettings.paletteIndex > 9) {
      LED.espSettings.paletteIndex = 1;
    }
    ESP_LOGI(LOG_TAG, "Changed palette to %d", LED.espSettings.paletteIndex);
  }

  LED.tick();
}
