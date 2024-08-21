#pragma once
#include <Arduino.h>
#include <EUC.h>
#include <FastLED.h>
#include <TimerMs.h>

struct SettingsStruct {
  unsigned int paletteIndex = 1;
  unsigned long paletteChangedTime = 0;

  unsigned int modeIndex = 1;
  unsigned long modeChangedTime = 0;
};

class ledClass {
 public:
  void setup();
  void scanStart();
  void scanStop();
  void tick();
  void juggle();
  void FillLEDsFromPaletteColors(uint8_t colorIndex, uint8_t maxLed);
  SettingsStruct espSettings;
};

extern ledClass LED;
