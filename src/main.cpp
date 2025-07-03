#include <Arduino.h>
#include <EUC.h>
// #include <LED.h>
#include <esp_log.h>

static const char* LOG_TAG = "ESP32-EUC";

void setup() {
  delay(3000);
  ESP_LOGI(LOG_TAG, "Starting application...");
  EUC.setup();
  ESP_LOGD(LOG_TAG, "Application started!");
}

void loop() {
  EUC.tick();
}
