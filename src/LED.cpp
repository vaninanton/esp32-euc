#include "LED.h"

#define DATA_PIN 4
#define NUM_LEDS 17

static const char* LOG_TAG = "ESP32-EUC";

CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType currentBlending = LINEARBLEND;
uint8_t brightness = 10;
uint8_t startIndex = 0;
TimerMs timerFastLed(10, 1, false);

void ledClass::setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 100);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[0] = CRGB::Red;
  leds[NUM_LEDS - 1] = CRGB::Red;
  FastLED.show();
}

void ledClass::scanStart() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[0] = CRGB::Green;
  leds[NUM_LEDS - 1] = CRGB::Green;
  FastLED.show();
}

void ledClass::scanStop() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
}

void ledClass::tick() {
  if (!timerFastLed.tick())
    return;

  switch (espSettings.paletteIndex) {
    case 1:
      fill_solid(currentPalette, 16, CRGB::Black);
      currentPalette[0] = CRGB::White;
      currentPalette[4] = CRGB::White;
      currentPalette[8] = CRGB::White;
      currentPalette[12] = CRGB::White;
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
      currentPalette = CloudColors_p;
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
      currentPalette[4] = CRGB::Blue;
      currentPalette[5] = CRGB::Blue;
      currentPalette[12] = CRGB::Blue;
      currentPalette[13] = CRGB::Blue;
      break;
  }

  startIndex = startIndex + 1;
  // Заполнение по скорости
  if (espSettings.modeIndex == 1) {
    // int16_t absSpeed = abs(EUC.speed);
    // if (absSpeed != 0) {
    //   long in_max = 3000;
    //   if (absSpeed >= 3000) {
    //     in_max = 4000;
    //   } else if (absSpeed >= 4000) {
    //     in_max = 5000;
    //   } else if (absSpeed >= 5000) {
    //     in_max = 5500;
    //   }
    //   long maxLeds = map(absSpeed, 0, 3000, 0, NUM_LEDS);
    //   fill_gradient_RGB(leds, maxLeds, CRGB::Red, CRGB::White);
    // }

    if (EUC.tailLightState >= 2) {
      fill_solid(leds, NUM_LEDS, CRGB::Red);
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

  if (espSettings.modeChangedTime != 0 && millis() - espSettings.modeChangedTime <= 1000) {
    fill_solid(leds, espSettings.modeIndex, CRGB::Purple);
  }

  if (espSettings.paletteChangedTime != 0 && millis() - espSettings.paletteChangedTime <= 1000) {
    fill_solid(leds, espSettings.paletteIndex, CRGB::Blue);
  }

  FastLED.show();
}

void ledClass::juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, brightness);
    dothue += 32;
  }
}

void ledClass::FillLEDsFromPaletteColors(uint8_t colorIndex, uint8_t maxLed) {
  for (int i = 0; i < maxLed; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

ledClass LED = ledClass();
