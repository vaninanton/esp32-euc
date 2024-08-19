#include <FastLED.h>

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, brightness);
    dothue += 32;
  }
}

void FillLEDsFromPaletteColors(uint8_t colorIndex, uint8_t maxLed) {
  for (int i = 0; i < maxLed; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

// // This function fills the palette with totally random colors.
// void SetupTotallyRandomPalette() {
//   for (int i = 0; i < 16; i++) {
//     currentPalette[i] = CHSV(random8(), 255, random8());
//   }
// }

// // This function sets up a palette of black and white stripes,
// // using code.  Since the palette is effectively an array of
// // sixteen CRGB colors, the various fill_* functions can be used
// // to set them up.
// void SetupBlackAndWhiteStripedPalette() {
//   // 'black out' all 16 palette entries...
//   fill_solid(currentPalette, 16, CRGB::Black);
//   // and set every fourth one to white.
//   currentPalette[0] = CRGB::White;
//   currentPalette[4] = CRGB::White;
//   currentPalette[8] = CRGB::White;
//   currentPalette[12] = CRGB::White;
// }

// void SetupPurpleAndOrangePalette() {
//   CRGB purple = CHSV(HUE_PURPLE, 255, 255);
//   CRGB orange = CHSV(HUE_ORANGE, 255, 255);
//   CRGB black = CRGB::Black;

//   currentPalette = CRGBPalette16(orange, orange, black, black, purple, purple, black, black, orange, orange, black, black, purple, purple, black, black);
// }

// void ChangePalettePeriodically() {
//   uint8_t secondHand = (millis() / 1000) % 60;
//   static uint8_t lastSecond = 99;

//   if (lastSecond != secondHand) {
//     lastSecond = secondHand;
//     if (secondHand == 0) {
//       currentPalette = PartyColors_p;
//       currentBlending = LINEARBLEND;
//     }
//     if (secondHand == 10) {
//       currentPalette = RainbowStripeColors_p;
//       currentBlending = NOBLEND;
//     }
//     if (secondHand == 15) {
//       currentPalette = RainbowStripeColors_p;
//       currentBlending = LINEARBLEND;
//     }
//     if (secondHand == 20) {
//       SetupPurpleAndOrangePalette();
//       currentBlending = NOBLEND;
//     }
//     if (secondHand == 25) {
//       SetupTotallyRandomPalette();
//       currentBlending = LINEARBLEND;
//     }
//     if (secondHand == 30) {
//       SetupBlackAndWhiteStripedPalette();
//       currentBlending = NOBLEND;
//     }
//     if (secondHand == 35) {
//       SetupBlackAndWhiteStripedPalette();
//       currentBlending = LINEARBLEND;
//     }
//     if (secondHand == 40) {
//       currentPalette = CloudColors_p;
//       currentBlending = LINEARBLEND;
//     }
//     if (secondHand == 45) {
//       currentPalette = PartyColors_p;
//       currentBlending = LINEARBLEND;
//     }
//   }
// }
