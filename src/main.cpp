#include <Arduino.h>
#include <EUC.h>
#include <EncButton.h>
#include <LED.h>
#include <TimerMs.h>
#include <esp_log.h>

TimerMs timerShowEucDebugLog(1000, 1, false);
Button btn(9);

static const char* LOG_TAG = "ESP32-EUC";

void setup() {
  ESP_LOGI(LOG_TAG, "Starting application...");
  EUC.setup();
  LED.setup();
  pinMode(8, OUTPUT);
  ESP_LOGD(LOG_TAG, "Application started!");
}

void loop() {
  btn.tick();
  EUC.tick();

  if (timerShowEucDebugLog.tick()) {
    EUC.debug();
  }

  if (EUC.tailLightState == 3) {
    digitalWrite(8, LOW);
  } else {
    digitalWrite(8, HIGH);
  }

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
